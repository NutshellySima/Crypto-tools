// Nut-Crypt.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include<iostream>
#include<exception>
#include<string>
#include<fstream>
#include<cstdlib>
#include<sodium.h>
#include<cstdio>
#include<boost/program_options.hpp>

namespace po = boost::program_options;
using namespace std;

unsigned char salt[crypto_pwhash_SALTBYTES];
unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
constexpr size_t CHUNK_SIZE = 4096;

void Encrypt(string Input, string Output, string Password)
{
	const char* PASSWORD = Password.c_str();
	randombytes_buf(salt, sizeof salt);
	if (crypto_pwhash
	(key, sizeof key, PASSWORD, strlen(PASSWORD), salt,
		crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE,
		crypto_pwhash_ALG_DEFAULT) != 0) {
		cerr << "Out of Memory!\n";
		exit(1);
	}
	ifstream is(Input, ios::binary | ios::in);
	if (!is.is_open())
	{
		cerr << "The input file cannot be opened!\n";
		exit(1);
	}
	ofstream os(Output, ios::binary | ios::trunc | ios::out);
	unsigned char  buf_in[CHUNK_SIZE];
	unsigned char  buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
	unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
	crypto_secretstream_xchacha20poly1305_state st;
	crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
	// Write salt
	os.write(reinterpret_cast<const char*>(salt), crypto_pwhash_SALTBYTES);
	// Write header
	os.write(reinterpret_cast<const char*>(header), crypto_secretstream_xchacha20poly1305_HEADERBYTES);
	bool eof = is.eof();
	unsigned char tag = 0;
	unsigned long long out_len;
	size_t rlen;
	do
	{
		is.read(reinterpret_cast<char*>(buf_in), CHUNK_SIZE);
		rlen = is.gcount();
		eof = is.eof();
		tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
		crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in, rlen,
			NULL, 0, tag);
		os.write(reinterpret_cast<char*>(buf_out), out_len);
	} while (!eof);
	is.close();
	os.close();
}

void Decrypt(string Input, string Output, string Password)
{
	const char* PASSWORD = Password.c_str();
	ifstream is(Input, ios::binary | ios::in);
	if (!is.is_open())
	{
		cerr << "The input file cannot be opened!\n";
		exit(1);
	}
	// Read salt
	is.read(reinterpret_cast<char*>(salt), crypto_pwhash_SALTBYTES);
	if (is.gcount() != crypto_pwhash_SALTBYTES)
	{
		cerr << "The input file is broken!\n";
		exit(1);
	}
	if (crypto_pwhash
	(key, sizeof key, PASSWORD, strlen(PASSWORD), salt,
		crypto_pwhash_OPSLIMIT_MODERATE, crypto_pwhash_MEMLIMIT_MODERATE,
		crypto_pwhash_ALG_DEFAULT) != 0) {
		cerr << "Out of Memory!\n";
		exit(1);
	}
	ofstream os(Output, ios::binary | ios::trunc | ios::out);
	unsigned char  buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
	unsigned char  buf_out[CHUNK_SIZE];
	unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
	crypto_secretstream_xchacha20poly1305_state st;
	// Read header
	is.read(reinterpret_cast<char*>(header), crypto_secretstream_xchacha20poly1305_HEADERBYTES);
	if (is.gcount() != crypto_secretstream_xchacha20poly1305_HEADERBYTES)
	{
		cerr << "The input file is broken!\n";
		exit(1);
	}
	if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key) != 0) {
		// incomplete header
		cerr << "The encryption header is incomplete\n";
		exit(1);
	}
	bool eof = is.eof();
	unsigned char tag = 0;
	unsigned long long out_len;
	size_t rlen;
	do
	{
		is.read(reinterpret_cast<char*>(buf_in), CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES);
		rlen = is.gcount();
		eof = is.eof();
		if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag,
			buf_in, rlen, NULL, 0) != 0) {
			// corrupted chunk
			cerr << "The file is corrupted\n";
			is.close();
			os.close();
			remove(Output.c_str());
			exit(1);
		}
		if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL && !eof) {
			cerr << "End of file reached before the end of the stream\n";
			is.close();
			os.close();
			remove(Output.c_str());
			exit(1); /* premature end (end of file reached before the end of the stream) */
		}
		os.write(reinterpret_cast<char*>(buf_out), out_len);
	} while (!eof);
	is.close();
	os.close();
}

int main(int argc, char** argv)
{
	// Init crypto library
	if (sodium_init() == -1) {
		std::cerr << "libsodium failed to init, exiting...\n";
		return 1;
	}
	// Declare the supported options.
	po::options_description desc("Usage");
	desc.add_options()
		("help", "Show help message")
		("encrypt", "Work in encryption mode")
		("decrypt", "Work in decryption mode")
		("i", po::value<string>(), "Input file")
		("o", po::value<string>(), "Output file")
		("p", po::value<string>(), "Specify the password")
		;
	po::positional_options_description p;
	p.add("i", -1);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
		po::notify(vm);
	}
	catch (exception&e)
	{
		cerr << e.what() << endl;
		return 1;
	}

	// help
	if (vm.count("help")) {
		cout << "Nut-Crypt Version 0.0.1\n\n";
		cout << desc << "\n";
		return 1;
	}

	// encrypt/decrypt
	bool EncryptMode = true;
	if (vm.count("encrypt") || vm.count("decrypt")) {
		if (vm.count("encrypt") && vm.count("decrypt"))
		{
			cerr << "Encryption and decryption cannot happen at the same time\n";
			return 1;
		}
		if (vm.count("decrypt"))
			EncryptMode = false;
	}

	// password
	if (!vm.count("p"))
	{
		cerr << "A password needs to be specified\n";
		return 1;
	}

	// Input-file
	if (!vm.count("i"))
	{
		cerr << "An input file needs to be specified\n";
		return 1;
	}

	string Input = vm["i"].as<string>();
	string Output = Input + (EncryptMode ? ".Encrypted" : ".Decrypted");
	string Password = vm["p"].as<string>();

	// Output-file
	if (vm.count("o"))
		Output = vm["o"].as<string>();

	if (EncryptMode) {
		Encrypt(Input, Output, Password);
	}
	else {
		Decrypt(Input, Output, Password);
	}
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
