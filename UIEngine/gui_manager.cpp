#include "gui_manager.hpp"
#include "raylib.h"

extern "C" {
#include "sodium.h"    // found via api/libsodium/include
}

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

GuiManager::GuiManager() {
    isUnlocked = false;
    isNewVaultMode = false;
    memset(masterPassword, 0, sizeof(masterPassword));
    memset(confirmMasterPassword, 0, sizeof(confirmMasterPassword));
    showMasterPassword = false;
    snprintf(statusMessage, sizeof(statusMessage), "Please enter your master password to unlock.");

    memset(vaultName, 0, sizeof(vaultName));
    memset(searchFilter, 0, sizeof(searchFilter));
    selectedIndex = -1;
    isEditing = false;
    isCreating = false;
    showPasswordInEditor = false;
    memset(&currentEdit, 0, sizeof(currentEdit));

    showGenerator = false;
    genLength = 16;
    genIncludeUpper = 1;
    genIncludeLower = 1;
    genIncludeNumbers = 1;
    genIncludeSymbols = 1;
    memset(generatedPassword, 0, sizeof(generatedPassword));

    onUnlock = nullptr;
    onInitVault = nullptr;
    onLock = nullptr;
    onSaveEntry = nullptr;
    onDeleteEntry = nullptr;

}

GuiManager::~GuiManager() {
    ClearMemory();
}

void GuiManager::ClearMemory() {
    // Securely wipe memory buffer using libsodium
    sodium_memzero(masterPassword, sizeof(masterPassword));
    sodium_memzero(confirmMasterPassword, sizeof(confirmMasterPassword));
    sodium_memzero(&currentEdit, sizeof(currentEdit));
    for (auto &e : entries) {
        sodium_memzero(e.password, sizeof(e.password));
    }
}

void GuiManager::Lock() {
    isUnlocked = false;
    isEditing = false;
    isCreating = false;
    selectedIndex = -1;
    ClearMemory();
    snprintf(statusMessage, sizeof(statusMessage), "Vault locked. Sensitive memory wiped.");
    if (onLock) {
        onLock();
    }
}

void GuiManager::AddEntryToMemory(const CredentialEntry &entry) {
    entries.push_back(entry);
}

void GuiManager::GeneratePassword() {
    const char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *lower = "abcdefghijklmnopqrstuvwxyz";
    const char *digits = "0123456789";
    const char *symbols = "!@#$%^&*()_+-=[]{}|;:,.<>?";

    char charset[128] = { 0 };
    if (genIncludeUpper) strcat(charset, upper);
    if (genIncludeLower) strcat(charset, lower);
    if (genIncludeNumbers) strcat(charset, digits);
    if (genIncludeSymbols) strcat(charset, symbols);

    int charsetLen = strlen(charset);
    if (charsetLen == 0) {
        snprintf(generatedPassword, sizeof(generatedPassword), "Select at least 1 option");
        return;
    }

    if (genLength < 4) genLength = 4;
    if (genLength > 64) genLength = 64;

    // Use libsodium secure random bytes
    for (int i = 0; i < genLength; i++) {
        uint32_t r = randombytes_uniform((uint32_t)charsetLen);
        generatedPassword[i] = charset[r];
    }
    generatedPassword[genLength] = '\0';
}

void GuiManager::ProcessGui(mu_Context *ctx, int screen_w, int screen_h) {
    if (!isUnlocked) {
        DrawLockWindow(ctx, screen_w, screen_h);
    } else {
        DrawVaultWindow(ctx, screen_w, screen_h);
        if (showGenerator) {
            DrawGeneratorWindow(ctx, screen_w, screen_h);
        }
    }
}

void GuiManager::DrawLockWindow(mu_Context *ctx, int screen_w, int screen_h) {
    // Увеличили высоту окна на 50 пикселей (было 360 : 310), чтобы новое поле красиво влезло
    int win_w = 460;
    int win_h = isNewVaultMode ? 410 : 360; 
    int win_x = (screen_w - win_w) / 2;
    int win_y = (screen_h - win_h) / 2;

    if (mu_begin_window(ctx, "Password Papochka - Vault Authentication", mu_rect(win_x, win_y, win_w, win_h))) {
        mu_layout_row(ctx, 1, (int[]){ -1 }, 24);
        mu_label(ctx, isNewVaultMode ? "Initialize New Encrypted Vault" : "Unlock Existing Vault");

        mu_layout_row(ctx, 1, (int[]){ -1 }, 18);
        mu_text(ctx, statusMessage);

        // --- ВВОД ИМЕНИ ФАЙЛА БАЗЫ ДАННЫХ ---
        mu_layout_row(ctx, 1, (int[]){ -1 }, 20);
        mu_label(ctx, "Vault Storage File Name:");
        mu_layout_row(ctx, 1, (int[]){ -1 }, 28);
        mu_textbox(ctx, vaultName, sizeof(vaultName));
        // ------------------------------------

        mu_layout_row(ctx, 1, (int[]){ -1 }, 20);
        mu_label(ctx, "Master Password:");

        mu_layout_row(ctx, 1, (int[]){ -1 }, 28);
        mu_textbox(ctx, masterPassword, sizeof(masterPassword));

        if (isNewVaultMode) {
            mu_layout_row(ctx, 1, (int[]){ -1 }, 20);
            mu_label(ctx, "Confirm Master Password:");
            mu_layout_row(ctx, 1, (int[]){ -1 }, 28);
            mu_textbox(ctx, confirmMasterPassword, sizeof(confirmMasterPassword));
        }

        mu_layout_row(ctx, 2, (int[]){ 180, -1 }, 26);
        int showPwInt = showMasterPassword ? 1 : 0;
        if (mu_checkbox(ctx, "Show Password", &showPwInt)) {
            showMasterPassword = (showPwInt != 0);
        }

        // Toggle unlock vs create mode
        if (mu_button(ctx, isNewVaultMode ? "Switch to: Unlock Existing" : "Switch to: Create New Vault")) {
            isNewVaultMode = !isNewVaultMode;
            snprintf(statusMessage, sizeof(statusMessage), isNewVaultMode ? "Set a strong master password." : "Enter master password to unlock.");
        }

        mu_layout_row(ctx, 1, (int[]){ -1 }, 36);
        if (mu_button(ctx, isNewVaultMode ? "Initialize & Open Vault" : "Unlock Vault")) {
            if (strlen(vaultName) == 0) {
                snprintf(statusMessage, sizeof(statusMessage), "Vault file name cannot be empty!");
            } else if (strlen(masterPassword) == 0) {
                snprintf(statusMessage, sizeof(statusMessage), "Master password cannot be empty!");
            } else if (isNewVaultMode && strcmp(masterPassword, confirmMasterPassword) != 0) {
                snprintf(statusMessage, sizeof(statusMessage), "Master passwords do not match!");
            } else {
                bool success = true;
  
                if (onSetActiveVault) {
                    onSetActiveVault(vaultName);
                }

                if (isNewVaultMode && onInitVault) {
                    success = onInitVault(masterPassword);
                } else if (!isNewVaultMode && onUnlock) {
                    success = onUnlock(masterPassword, entries);
                }

                if (success) {
                    isUnlocked = true;
                    snprintf(statusMessage, sizeof(statusMessage), "Vault successfully unlocked.");
                } else {
                    snprintf(statusMessage, sizeof(statusMessage), "Decryption / Verification Failed!");
                }
            }
        }

        mu_end_window(ctx);
    }
}

void GuiManager::DrawVaultWindow(mu_Context *ctx, int screen_w, int screen_h) {
    // Top Bar Window
    if (mu_begin_window_ex(ctx, "TopBar", mu_rect(0, 0, screen_w, 48), MU_OPT_NOTITLE | MU_OPT_NORESIZE)) {
        mu_layout_row(ctx, 4, (int[]){ 220, 120, 150, -1 }, 32);
        mu_label(ctx, "Password Papochka [AES-GCM / libsodium]");

        if (mu_button(ctx, "+ New Entry")) {
            isCreating = true;
            isEditing = true;
            selectedIndex = -1;
            memset(&currentEdit, 0, sizeof(currentEdit));
            currentEdit.id = (int)entries.size() + 1;
            snprintf(currentEdit.title, sizeof(currentEdit.title), "New Service");
        }

        if (mu_button(ctx, "Key Generator")) {
            showGenerator = !showGenerator;
            if (showGenerator && strlen(generatedPassword) == 0) {
                GeneratePassword();
            }
        }

        if (mu_button(ctx, "Lock Vault (Zero Memory)")) {
            Lock();
        }

        mu_end_window(ctx);
    }

    int left_w = 300;
    int content_y = 50;
    int content_h = screen_h - 52;

    // Left Panel: Entry List & Search
    if (mu_begin_window_ex(ctx, "Vault Accounts", mu_rect(0, content_y, left_w, content_h), MU_OPT_NORESIZE)) {
        mu_layout_row(ctx, 1, (int[]){ -1 }, 22);
        mu_label(ctx, "Search Filter:");
        mu_layout_row(ctx, 1, (int[]){ -1 }, 26);
        mu_textbox(ctx, searchFilter, sizeof(searchFilter));

        mu_layout_row(ctx, 1, (int[]){ -1 }, 18);
        char countStr[48];
        snprintf(countStr, sizeof(countStr), "Total Accounts: %d", (int)entries.size());
        mu_label(ctx, countStr);

        for (size_t i = 0; i < entries.size(); i++) {
            // Filter by search string if not empty
            if (strlen(searchFilter) > 0) {
                std::string titleStr = entries[i].title;
                std::string userStr = entries[i].username;
                std::string query = searchFilter;
                std::transform(titleStr.begin(), titleStr.end(), titleStr.begin(), ::tolower);
                std::transform(userStr.begin(), userStr.end(), userStr.begin(), ::tolower);
                std::transform(query.begin(), query.end(), query.begin(), ::tolower);
                if (titleStr.find(query) == std::string::npos && userStr.find(query) == std::string::npos) {
                    continue;
                }
            }

            mu_layout_row(ctx, 1, (int[]){ -1 }, 32);
            char btnLabel[128];
            snprintf(btnLabel, sizeof(btnLabel), "%s (%s)", entries[i].title, entries[i].username);
            if (mu_button(ctx, btnLabel)) {
                selectedIndex = (int)i;
                isEditing = false;
                isCreating = false;
                currentEdit = entries[i];
            }
        }

        mu_end_window(ctx);
    }

    // Right Panel: Details & Editor View
    int right_x = left_w + 2;
    int right_w = screen_w - right_x;
    if (mu_begin_window_ex(ctx, "Account Details & Editor", mu_rect(right_x, content_y, right_w, content_h), MU_OPT_NORESIZE)) {
        if (selectedIndex >= 0 || isCreating) {
            mu_layout_row(ctx, 1, (int[]){ -1 }, 24);
            mu_label(ctx, isCreating ? "Creating New Account" : (isEditing ? "Editing Account" : "Account Overview"));

            mu_layout_row(ctx, 1, (int[]){ -1 }, 18);
            mu_label(ctx, "Title / Service Name:");
            mu_layout_row(ctx, 1, (int[]){ -1 }, 26);
            if (isEditing) {
                mu_textbox(ctx, currentEdit.title, sizeof(currentEdit.title));
            } else {
                mu_text(ctx, currentEdit.title);
            }

            mu_layout_row(ctx, 1, (int[]){ -1 }, 18);
            mu_label(ctx, "Username / Email / Identifier:");
            mu_layout_row(ctx, 1, (int[]){ -1 }, 26);
            if (isEditing) {
                mu_textbox(ctx, currentEdit.username, sizeof(currentEdit.username));
            } else {
                mu_text(ctx, currentEdit.username);
            }

            mu_layout_row(ctx, 1, (int[]){ -1 }, 18);
            mu_label(ctx, "Password:");

            mu_layout_row(ctx, 3, (int[]){ -1, 100, 110 }, 28);
            if (isEditing) {
                mu_textbox(ctx, currentEdit.password, sizeof(currentEdit.password));
            } else {
                if (showPasswordInEditor) {
                    mu_text(ctx, currentEdit.password);
                } else {
                    mu_text(ctx, "••••••••••••••••");
                }
            }

            if (mu_button(ctx, showPasswordInEditor ? "Hide" : "Reveal")) {
                showPasswordInEditor = !showPasswordInEditor;
            }

            if (mu_button(ctx, "Copy Pass")) {
                SetClipboardText(currentEdit.password);
            }

            mu_layout_row(ctx, 1, (int[]){ -1 }, 18);
            mu_label(ctx, "Website URL:");
            mu_layout_row(ctx, 1, (int[]){ -1 }, 26);
            if (isEditing) {
                mu_textbox(ctx, currentEdit.url, sizeof(currentEdit.url));
            } else {
                mu_text(ctx, currentEdit.url);
            }

            mu_layout_row(ctx, 1, (int[]){ -1 }, 18);
            mu_label(ctx, "Notes / Recovery Info:");
            mu_layout_row(ctx, 1, (int[]){ -1 }, 48);
            if (isEditing) {
                mu_textbox(ctx, currentEdit.notes, sizeof(currentEdit.notes));
            } else {
                mu_text(ctx, currentEdit.notes);
            }

            // Action Buttons
            // Action Buttons
            mu_layout_row(ctx, 3, (int[]){ 110, 110, 110 }, 32);
            if (!isEditing) {
                if (mu_button(ctx, "Edit Details")) {
                    isEditing = true;
                }
                if (mu_button(ctx, "Delete")) {
                    if (selectedIndex >= 0 && selectedIndex < (int)entries.size()) {
                        entries.erase(entries.begin() + selectedIndex); // Удалили из памяти
                        if (onSaveEntry) {
                            onSaveEntry(this->entries); // Перезаписали файл БЕЗ этой записи
                        }
                    }
                    selectedIndex = -1;
                    isEditing = false;
                    isCreating = false;
                }
            } else {
                if (mu_button(ctx, "Save")) {
                    // 1. Сначала пушим/обновляем в памяти GUI
                    if (isCreating) {
                        entries.push_back(currentEdit);
                        selectedIndex = (int)entries.size() - 1;
                        isCreating = false;
                    } else if (selectedIndex >= 0 && selectedIndex < (int)entries.size()) {
                        entries[selectedIndex] = currentEdit;
                    }
                    
                    // 2. И только теперь шифруем актуальный вектор на диск
                    if (onSaveEntry) {
                        onSaveEntry(this->entries);
                    }
                    isEditing = false;
                }
                
                if (mu_button(ctx, "Cancel")) {
                    if (isCreating) {
                        isCreating = false;
                        selectedIndex = -1;
                    } else {
                        currentEdit = entries[selectedIndex];
                        isEditing = false;
                    }
                }
            }
        } else {
            mu_layout_row(ctx, 1, (int[]){ -1 }, 32);
            mu_label(ctx, "Select an account from the left list, or click '+ New Entry'.");
        }

        mu_end_window(ctx); // Закрываем окно "Account Details & Editor"
    }
} // Закрываем саму функцию DrawVaultWindow


void GuiManager::DrawGeneratorWindow(mu_Context *ctx, int screen_w, int screen_h) {
    int win_w = 400;
    int win_h = 320;
    int win_x = (screen_w - win_w) / 2;
    int win_y = (screen_h - win_h) / 2;

    if (mu_begin_window(ctx, "Secure Password Generator", mu_rect(win_x, win_y, win_w, win_h))) {
        mu_layout_row(ctx, 1, (int[]){ -1 }, 20);
        mu_label(ctx, "Generated Password:");

        mu_layout_row(ctx, 2, (int[]){ -1, 100 }, 28);
        mu_textbox(ctx, generatedPassword, sizeof(generatedPassword));
        if (mu_button(ctx, "Copy")) {
            SetClipboardText(generatedPassword);
        }

        mu_layout_row(ctx, 1, (int[]){ -1 }, 20);
        char lenStr[32];
        snprintf(lenStr, sizeof(lenStr), "Length: %d characters", genLength);
        mu_label(ctx, lenStr);

        mu_layout_row(ctx, 2, (int[]){ 120, 120 }, 24);
        if (mu_button(ctx, "- Shorter")) {
            if (genLength > 8) genLength -= 2;
            GeneratePassword();
        }
        if (mu_button(ctx, "+ Longer")) {
            if (genLength < 64) genLength += 2;
            GeneratePassword();
        }

        mu_layout_row(ctx, 2, (int[]){ 180, -1 }, 24);
        mu_checkbox(ctx, "Uppercase (A-Z)", &genIncludeUpper);
        mu_checkbox(ctx, "Lowercase (a-z)", &genIncludeLower);

        mu_layout_row(ctx, 2, (int[]){ 180, -1 }, 24);
        mu_checkbox(ctx, "Numbers (0-9)", &genIncludeNumbers);
        mu_checkbox(ctx, "Symbols (!@#$%)", &genIncludeSymbols);

        mu_layout_row(ctx, 2, (int[]){ 140, 100 }, 32);
        if (mu_button(ctx, "Regenerate")) {
            GeneratePassword();
        }
        if (mu_button(ctx, "Close")) {
            showGenerator = false;
        }

        mu_end_window(ctx);
    }
}
