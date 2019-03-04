# Crypto-tools

## Crypt

It uses `libsodium` to first derive a secret key from the user provided password, then use the secret key to encrypt the file using AEAD encyption. 

## Send

It uses `libsodium` to exchange keys to derive a shared secret between the server and user. The sender should compare the printed public key to prevent the MITM attack. For files with low entropy, it uses `zstd` library to compress then on the fly.

## Discovery

It uses IPv4/IPv6 multicast to discover nodes on the same local network segment.
