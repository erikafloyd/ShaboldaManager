#pragma once
#include <string>

int initSodium();

std::string getMasPass();

bool write_hex(
    const unsigned char *cipher,
    const unsigned long long
        cipher_len);

bool createMasterKey();