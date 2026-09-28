#ifndef VAULT_BACKEND_H
#define VAULT_BACKEND_H

#include "api/credentialsJson.hpp"
#include <sodium.h>
#include <string>
#include <vector>

void SetActiveVault(const std::string &name);

extern unsigned char g_masterKey[crypto_secretbox_KEYBYTES];
extern std::string masPassword;

bool initVault(const char *master_password);
bool UnlockVault(const char *master_password,
                 std::vector<CredentialEntry> &out_entries);
bool SaveEntry(const std::vector<CredentialEntry> &entries);

#endif // VAULT_BACKEND_H
