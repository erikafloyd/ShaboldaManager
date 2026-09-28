#include "maspass.h"
#include "sodium/core.h"
#include "sodium/crypto_pwhash.h"
#include "sodium/crypto_pwhash_argon2i.h"
#include "sodium/crypto_secretbox.h"
#include "sodium/randombytes.h"
#include "sodium/utils.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sodium.h>
#include <string>

using std::cin;
using std::cout;
using std::endl;
using std::getline;

int initSodium() {
  if (sodium_init() < 0)
    return 1;
  else
    return 0;
}

std::string getMasPass() {
  std::string masPassword; // передавать в LibSodium через masPassword.
  cout << "Type in your masterpass: ";
  getline(cin, masPassword);
  return masPassword;
}

bool write_hex(
    const unsigned char *cipher,
    const unsigned long long
        cipher_len) { // бля крч пратик смори, мы тут поставили *cipher потому
                      // что мы должны сказать нашему коду, что за тип данных
                      // туда прилетит, хотя unsigned char массив когда
                      // вызываешь, он и отдаёт тебе как бы адрес, и тогда
                      // вопрос, почему он блять этого не понимает - яхз, но раз
                      // надо я так сделал крч.

  char hex_output[cipher_len * 2 + 1]; // примечание 1: плиз не надо забывать о
                                       // нулевом операторе прошу, добавляй + 1

  // 4:19 AM: sizeof(out) логично, хотя подсмотрел у ИИ, выглядит красиво так
  // что DINAHU

  // 4:20 AM:
  // Было: size_t hex_outputlen = sizeof(hex_output);
  // Стало: size_t hex_outputlen = cipher_len * 2 + 1;
  // пиши математику руками, а не через sizeof(hex_output).
  // Так как размер массива зависит от переменной (cipher_len), для C++ это
  // dynamic-размер (VLA). Из-за этого sizeof внутри функции вернёт хуйню или
  // ошибку компиляции. Просто дублируем размер напрямую.
  // !!! !!! !!! Блять кароч sizeof() не умеет работать в рантайме, массив
  // переменной длины (VLA) умеет.

  size_t hex_outputlen = cipher_len * 2 + 1;

  sodium_bin2hex(hex_output, hex_outputlen, cipher, cipher_len);

  cout << hex_output;
  std::ofstream masPass("maspass.txt");
  if (masPass.is_open()) {
    masPass << hex_output;
    masPass.close();
    return true;
  } else {
    masPass.close();
    std::cerr << "Couldn't create a file for maspass!" << endl;
    return false;
  }
}

bool createMasterKey() {

  std::string masPassword = ImissShit;

  unsigned long long passwdlen = masPassword.size();

  unsigned char salt[crypto_pwhash_SALTBYTES]; // 16 randombytes

  randombytes_buf(salt, sizeof(salt));

  unsigned char out[32]; // итоговый ключ будет 32 байта

  unsigned long long outlen = 32;
  // unsigned char out[32]; это просто массив, и если мы на него
  // ссылаемся то он передаст адрес, а если сделаем указатель - то на
  // первый байт массива, именно поэтому легче будет создать обычную
  // строку которая указывает на размер массива для LibSodium.

  if (crypto_pwhash(out, outlen, masPassword.c_str(), passwdlen, salt,
                    crypto_pwhash_OPSLIMIT_INTERACTIVE,
                    crypto_pwhash_MEMLIMIT_INTERACTIVE,
                    crypto_pwhash_ALG_DEFAULT) == 0) {
    cout << "crypto_pwhash successful!" << endl;
  } else {
    cout << "wrong key or corrupted data." << endl;
    return false;
  }

  return write_hex(out, sizeof(out));
}
