#include "vault_backend.h"
#include "credentialsJson.hpp"
#include "sodium/crypto_pwhash.h"
#include "sodium/randombytes.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

// для unsigned char мы юзаем sizeof()
// для char мы юзаем strlen();
// ПРИ УЧЁТЕ ЧТО У ОБОИХ ДО КОМПИЛЯЦИИ ИЗВЕСТЕН РАЗМЕР МАССИВА
// а именно: char piska[32], это правильно.
// int hueglot = 32 + 16;
// char piska[hueglot] неправильно нихуя. hueglot высчитывается во время
// исполнения кода, поэтму мы не можем задать его как размер массива впринципе.

// ИИшная версия:
// 1. Для unsigned char мы юзаем sizeof() - 1, ЕСЛИ это строка в кавычках
// ("123")! Потому что "123" в памяти — это [1, 2, 3, \0]. sizeof вернет 4, а
// для crypto_pwhash нужно 3.
// 2. Для char мы юзаем strlen(), ОН САМ игнорирует \0, тут вычитать единицу НЕ
// НАДО. ПРИ УЧЁТЕ ЧТО У ОБОИХ ДО КОМПИЛЯЦИИ ИЗВЕСТЕН РАЗМЕР МАССИВА а именно:
// char piska[32], это правильно.

// int hueglot = 32 + 16;
// char piska[hueglot] — неправильно нихуя. hueglot высчитывается во время
// исполнения кода, поэтому мы не можем задать его как размер статического
// массива в принципе. ЧТОБЫ ЗАРАБОТАЛО: нужно дописать const (а лучше
// constexpr): constexpr int hueglot = 32 + 16; char piska[hueglot]; // Вот
// теперь компилятор счастлив, размер известен до компиляции.

// Блять кароч sizeof() не умеет работать в рантайме, массив переменной длины
// (VLA) умеет. плиз не надо забывать о нулевом операторе прошу, добавляй + 1

std::string g_currentVaultPath = "";

using json = nlohmann::json;

unsigned char g_masterKey[crypto_secretbox_KEYBYTES];
std::string masPassword;

void SetActiveVault(const std::string &name) {
  g_currentVaultPath = name + ".bin";
}

bool initVault(const char *master_password) {
  if (!master_password)
    return false;

  std::ofstream vault(
      g_currentVaultPath,
      std::ios::binary); // чтобы виндовс открывал в бинарном режиме
  if (!vault.is_open())
    return false;

  unsigned char salt[crypto_pwhash_SALTBYTES];
  randombytes_buf(salt, sizeof(salt));

  masPassword = master_password;

  // начинается подготовка для создания мастерключа
  // возможно может поломаться типо не тот сайз выдаст
  // (upd1.0) если оно поломается у безоса сдохнет мать

  if (crypto_pwhash(
          g_masterKey, sizeof(g_masterKey), masPassword.data(),
          masPassword.size(), salt, crypto_pwhash_OPSLIMIT_INTERACTIVE,
          crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_DEFAULT) != 0) {
    vault.close();
    return false;
  }
  sodium_memzero((char *)masPassword.data(), masPassword.size());

  vault.write((const char *)salt, sizeof(salt));

  std::string empty_json = "[]";
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  randombytes_buf(nonce, sizeof(nonce));

  vault.write((const char *)nonce, sizeof(nonce));

  std::vector<unsigned char> cipher_text(empty_json.size() +
                                         crypto_secretbox_MACBYTES);

  if (crypto_secretbox_easy(cipher_text.data(),
                            (const unsigned char *)empty_json.data(),
                            empty_json.size(), nonce, g_masterKey) == 0) {
    vault.write((const char *)cipher_text.data(), cipher_text.size());
    vault.close();
    return true;
  }

  vault.close();
  return false;
}

bool UnlockVault(const char *master_password,
                 std::vector<CredentialEntry> &out_entries) {
  if (!master_password)
    return false;

  std::ifstream vault(g_currentVaultPath, std::ios::binary | std::ios::ate);
  if (!vault.is_open())
    return false;

  std::streamsize fileSize = vault.tellg();
  vault.seekg(0, std::ios::beg);
  if (fileSize <
      (std::streamsize)(crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES +
                        crypto_secretbox_MACBYTES)) {
    vault.close();
    return false;
  }
  size_t cipher_size =
      fileSize - crypto_pwhash_SALTBYTES - crypto_secretbox_NONCEBYTES;
  std::vector<unsigned char> cipher_text(cipher_size);

  unsigned char salt[crypto_pwhash_SALTBYTES];
  unsigned char nonce[crypto_secretbox_NONCEBYTES];

  vault.read((char *)salt, crypto_pwhash_SALTBYTES);
  vault.read((char *)nonce, crypto_secretbox_NONCEBYTES);
  vault.read((char *)cipher_text.data(), cipher_size);
  vault.close();

  masPassword = master_password;

  if (crypto_pwhash(
          g_masterKey, sizeof(g_masterKey), masPassword.data(),
          masPassword.size(), salt, crypto_pwhash_OPSLIMIT_INTERACTIVE,
          crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_DEFAULT) != 0) {
    return false;
  }

  sodium_memzero((char *)masPassword.data(), masPassword.size());

  size_t plain_size = cipher_size - crypto_secretbox_MACBYTES;
  std::string json_str(plain_size, '\0');

  if (crypto_secretbox_open_easy(
          (unsigned char *)json_str.data(), cipher_text.data(),
          (unsigned long long)cipher_size, nonce, g_masterKey) != 0) {
    return false;
  }

  try { // вот это всё я вообще не ебу это ИИ сделал
    out_entries = json::parse(json_str).get<std::vector<CredentialEntry>>();
  } catch (const std::exception &e) {
    std::cerr << "JSON parse error during unlock: " << e.what() << std::endl;
    return false;
  }
  return true;
}

// אבא שלי היה טייס – בבקשה, אני מתחנן, תשיג לי עבודה.
// (ואם אתה קורא את זה, תגיד לו
//  שאני עושה אדריכלות מדהימה בטירוף, בבקשה. אני עני, אני גר בישראל, ואין לי כסף לאוכל. הגעתי לרמה C2 בעברית; אני יהודי, דובר-שפת-אם.
//  בבקשה, אני ממש רוצה עבודה – לאמא שלי ולי אין מה 

bool SaveEntry(const std::vector<CredentialEntry> &entries) {
  unsigned char salt[crypto_pwhash_SALTBYTES];
  std::ifstream vault_read(g_currentVaultPath, std::ios::binary);
  if (!vault_read.is_open())
    return false;
  vault_read.read((char *)salt, sizeof(salt));
  vault_read.close();

  std::string json_str = json(entries).dump();

  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  randombytes_buf(nonce, sizeof(nonce));

  // macbytes, сука всегда запомни блять нужно к векторам шифротекста
  // добавлять 16 байт долбоеб ебаный нахуй сука
  // ВАЖНО: Третьим параметром передаем именно РАЗМЕР БИНАРНОГО ШИФРА (длина +
  // 16) ПОЧЕМУ: Если передать неверный размер, дешифратор подумает, что
  // бинарный шифр длиннее, уйдет читать чужую память (системного мусора),
  // упрется в несовпадение защитной печати (MAC) и вернет ошибку -1. (это я с
  // помощью ИИ вывел, пздц я даун)

  std::vector<unsigned char> cipher_text(json_str.size() +
                                         crypto_secretbox_MACBYTES);

  if (crypto_secretbox_easy(cipher_text.data(),
                            (const unsigned char *)json_str.data(),
                            json_str.size(), nonce, g_masterKey) != 0)
    return false;

  // бля крч пратик смори, мы тут ставим звездочку перед указателем (например,
  // const unsigned char *cipher) потому что мы должны сказать нашему коду, что
  // за тип данных туда прилетит, хотя unsigned char массив когда вызываешь, он
  // и отдаёт тебе как бы адрес, и тогда вопрос, почему он блять этого не
  // понимает - яхз, но раз надо я так сделал крч. (upd1.0) нихуя мы не должны
  // пошёл в пизду вась

  std::ofstream vault(g_currentVaultPath, std::ios::binary);
  if (!vault.is_open())
    return false;
  vault.write((const char *)salt, sizeof(salt));
  vault.write((const char *)nonce, sizeof(nonce));
  vault.write((const char *)cipher_text.data(), cipher_text.size());
  return true;
}
