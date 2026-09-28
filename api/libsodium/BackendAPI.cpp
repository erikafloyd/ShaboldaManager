
// 1. Сначала ПЛЮСОВАЯ БАЗА (чистый C++)
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

// 2. Потом СТОРОННИЕ Си-библиотеки
#include "sodium/crypto_box.h"
#include "sodium/crypto_pwhash.h"
#include "sodium/crypto_secretbox.h"
#include "sodium/randombytes.h"
#include "sodium/utils.h"
#include <sodium.h>

// 3. И только в самом конце — ТВОЙ собственный хедер
#include "backend_encryption.h"

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

unsigned char g_masterKey[32];

bool g_isVaultOpen = false;

int intsodium() {
  if (sodium_init() < 0)
    return 1;
}

// для инициализации, когда чел только создаёт базу данных.

bool BackendInitVaultFn(const char *master_password) {
  unsigned char salt[crypto_pwhash_SALTBYTES];
  randombytes_buf(salt, sizeof(salt));

  std::ofstream vault(
      "vault.bin",
      std::ios::binary); // чтобы виндовс открывал в бинарном режиме
  if (vault.is_open()) {
    vault.write((const char *)salt, sizeof(salt));
    vault.close();
  } else {
    return false;
  }
  // начинается подготовка для создания мастерключа
  unsigned long long passwdlen = strlen(
      master_password); // возможно может поломаться типо не тот сайз выдаст

  if (crypto_pwhash(
          g_masterKey, sizeof(g_masterKey), master_password,
          strlen(master_password), salt, crypto_pwhash_OPSLIMIT_INTERACTIVE,
          crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_DEFAULT) == 0) {
    g_isVaultOpen = true;
    return true;
  }
  return false;
}

// для открывания существующей базы

bool BackendUnlockVaultFn(const char *master_password) {
  // 1. Нам больше не нужен огромный db_hash! Нам нужна только соль (16 байт)
  unsigned char salt[crypto_pwhash_SALTBYTES];

  // 2. Открываем файл сейфа в бинарном режиме
  std::ifstream vault("vault.bin", std::ios::binary);
  if (!vault.is_open()) {
    return false;
  }

  // 3. Отцепляем "первый вагон" — читаем ровно 16 байт соли из начала файла
  vault.read((char *)salt, sizeof(salt));

  // Подстраховка: проверяем, что в файле реально было хотя бы 16 байт соли
  if (vault.gcount() != sizeof(salt)) {
    vault.close();
    return false;
  }

  // Здесь можно продолжить читать из файла зашифрованные данные,
  // так как указатель внутри ifstream сдвинулся и стоит СРАЗУ ПОСЛЕ соли.
  // vault.read(куда, сколько);

  vault.close();

  // 4. Считаем длину пароля (здесь strlen работает на 100% миллиард процентов
  // верно)
  unsigned long long passwdlen = strlen(master_password);

  // 5. Генерируем 32-байтный мастер-ключ прямо в наше ОЗУ (в g_masterKey)
  if (crypto_pwhash(g_masterKey,         // КУДА: пишем ключ в глобальный массив
                    sizeof(g_masterKey), // СКОЛЬКО БАЙТ: 32 байта
                    master_password,     // Твой пароль
                    passwdlen,           // Длина пароля
                    salt, // Соль, которую мы только что вытащили из файла
                    crypto_pwhash_OPSLIMIT_INTERACTIVE, // Сложность процессора
                    crypto_pwhash_MEMLIMIT_INTERACTIVE, // Сложность оперативы
                    crypto_pwhash_ALG_DEFAULT) != 0) {
    // Если либсодиум вернул не 0, значит что-то пошло не так (например, не
    // хватило памяти)
    return false;
  }

  // 6. Ключ успешно сгенерирован и лежит в g_masterKey!
  // Теперь ты можешь использовать его для расшифровки остальной части файла.
  g_isVaultOpen = true;
  return true;
}

void BackendLockVaultFn() {
  sodium_memzero(g_masterKey, sizeof(g_masterKey));
  g_isVaultOpen = false;
}

bool SaveEntireVault(std::string &jsonString) {
  if (!g_isVaultOpen)
    return false;

  unsigned char nonce[crypto_box_NONCEBYTES];
  randombytes_buf(nonce, sizeof(nonce));

  std::vector<unsigned char> ciphertext(
      jsonString.size() +
      crypto_secretbox_MACBYTES); // s1mple вахуе с этого синтаксиса

  if (crypto_secretbox_easy(ciphertext.data(),
                            (const unsigned char *)jsonString.c_str(),
                            jsonString.size(), nonce, g_masterKey) != 0)
    return false;
  std::ofstream vault("vault.bin", std::ios::binary);

  if (!vault.is_open())
    return false;
  else {
    vault.write((const char *)ciphertext.data(), ciphertext.size());
    vault.write((const char *)nonce, sizeof(nonce));
  }
}
