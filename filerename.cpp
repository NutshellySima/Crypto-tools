#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <map>
#include <experimental/filesystem>
#include <functional>
#include <stdexcept>
#include <cstdlib>
using namespace std;

namespace fs = std::experimental::filesystem;
string txt_path, dir_path, chapter, class_name;
fs::path dir;

vector<pair<fs::path, fs::path>> files;
map<string, pair<string, bool>> idmap;
vector<string> idname;

void getFiles(string &path)
{
    for (auto &p : fs::directory_iterator(path))
    {
        if (p.status().type() == fs::file_type::regular)
        {
            fs::path name = p.path().filename();
            fs::path ext = p.path().extension();
            files.emplace_back(make_pair(name, ext));
        }
    }
}

void inputInfo()
{
    cout << "指定班级名称: ";
    cin >> class_name;
    cout << "指定作业文件夹路径: ";
    cin >> dir_path;
    cout << "指定姓名-学号文件路径: ";
    cin >> txt_path;
    cout << "指定章节: ";
    cin >> chapter;
    dir = fs::path(dir_path);
}

void getList(string &path)
{
    string name, number;
    ifstream in(path);
    if (!in)
    {
        cerr << "不能打开\"" << path << "\"。" << endl;
        exit(EXIT_FAILURE);
    }
    while (in >> name >> number)
    {
        idmap[name] = make_pair(number, false);
        idname.emplace_back(name);
    }
}

void renameFiles()
{
    for (auto &p : files)
    {
        bool found = false;
        for (auto &pp : idname)
        {
            const auto findstr = p.first.string();
            auto it = std::search(findstr.cbegin(), findstr.cend(), boyer_moore_searcher<string::const_iterator>(pp.cbegin(), pp.cend()));
            if (it != findstr.cend())
            {
                string newname = class_name + "-" + pp + "-" + chapter + "-" + idmap[pp].first + p.second.string();
                try
                {
                    fs::rename(dir / p.first.string(), dir / newname);
                }
                catch (exception &w)
                {
                    cerr << "错误详细信息：" << w.what() << endl
                         << "重命名\"" << p.first.string() << "\"失败" << endl;
                    cerr << "这可能是由于该文件正在被占用或者你想将其命名为非法文件名导致的。" << endl;
                    cerr << "请关闭文件并重试以及检查文件的新名称。" << endl;
                }
                found = true;
                idmap[pp].second = true;
                break;
            }
        }
        if (!found)
            cout << "找不到\"" << p.first.string() << "\"对应的学号" << endl;
    }
}

void showNotFound()
{
    size_t counter = 0;
    for (auto &x : idmap)
    {
        if (x.second.second == false)
        {
            cout << x.first << " " << x.second.first << " 尚未提交作业" << endl;
            ++counter;
        }
    }
    cout << "共" << counter << "位同学尚未提交作业" << endl;
}

int main()
{
    inputInfo();
    getFiles(dir_path);
    getList(txt_path);
    renameFiles();
    showNotFound();
    system("pause");
}
