#pragma once

#include <string>
#include <vector>
#include "api/credentialsJson.hpp"

extern "C" {
#include "microui.h"
}

// Function pointer types for easy binding with your backend_encryption.h
typedef bool (*BackendUnlockVaultFn)(const char *master_password, std::vector<CredentialEntry> &out_entries);
typedef bool (*BackendInitVaultFn)(const char *master_password);
typedef void (*BackendLockVaultFn)();
typedef bool (*BackendSaveEntryFn)(const std::vector<CredentialEntry> &entries);
typedef bool (*BackendDeleteEntryFn)(int entry_id);
typedef void (*BackendSetActiveVaultFn)(const std::string &name);


class GuiManager {
public:
  GuiManager();
  ~GuiManager();

  // Call this inside your main loop to render the microui windows
  void ProcessGui(mu_Context *ctx, int screen_width, int screen_height);

  // Callbacks setup (optional: you can connect your backend functions here)
  void SetUnlockCallback(BackendUnlockVaultFn fn) { onUnlock = fn; }
  void SetInitVaultCallback(BackendInitVaultFn fn) { onInitVault = fn; }
  void SetLockCallback(BackendLockVaultFn fn) { onLock = fn; }
  void SetSaveEntryCallback(BackendSaveEntryFn fn) { onSaveEntry = fn; }
  void SetDeleteEntryCallback(BackendDeleteEntryFn fn) { onDeleteEntry = fn; }
  void SetActiveVaultCallback(BackendSetActiveVaultFn fn) { onSetActiveVault = fn; }

  // Helpers to populate/clear credentials in memory
  void AddEntryToMemory(const CredentialEntry &entry);
  void ClearMemory();
  bool IsVaultUnlocked() const { return isUnlocked; }
  const std::vector<CredentialEntry> &GetEntries() const { return entries; }

  void Lock();

private:
  // UI Panels
  void DrawLockWindow(mu_Context *ctx, int screen_w, int screen_h);
  void DrawVaultWindow(mu_Context *ctx, int screen_w, int screen_h);
  void DrawGeneratorWindow(mu_Context *ctx, int screen_w, int screen_h);

  // Internal actions
  void GeneratePassword();

  // State variables
  bool isUnlocked;
  bool isNewVaultMode;
  char masterPassword[128];
  char confirmMasterPassword[128];
  char vaultName[64];
  bool showMasterPassword;
  char statusMessage[128];

  // Search and selection
  char searchFilter[64];
  std::vector<CredentialEntry> entries;
  int selectedIndex;

  // Edit/View buffer
  bool isEditing;
  bool isCreating;
  CredentialEntry currentEdit;
  bool showPasswordInEditor;

  // Password generator modal/window state
  bool showGenerator;
  int genLength;
  int genIncludeUpper;
  int genIncludeLower;
  int genIncludeNumbers;
  int genIncludeSymbols;
  char generatedPassword[128];

  // Attached backend callbacks
  BackendUnlockVaultFn onUnlock;
  BackendInitVaultFn onInitVault;
  BackendLockVaultFn onLock;
  BackendSaveEntryFn onSaveEntry;
  BackendDeleteEntryFn onDeleteEntry;
  BackendSetActiveVaultFn onSetActiveVault;
};
