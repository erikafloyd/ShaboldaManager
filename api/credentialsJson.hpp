#ifndef CREDENTIALS_JSON_HPP
#define CREDENTIALS_JSON_HPP

#pragma once

#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct CredentialEntry {
  int id;
  char title[64];
  char username[64];
  char password[128];
  char url[128];
  char notes[256];
};

inline void to_json(json &j, const CredentialEntry &e) {
  j = json{{"id", e.id},
           {"title", e.title},
           {"username", e.username},
           {"password", e.password},
           {"url", e.url},
           {"notes", e.notes}};
}

inline void from_json(const json &j, CredentialEntry &e) {
  j.at("id").get_to(e.id);
  auto safe_copy =
      [](const json &node, char *dest,
         size_t max_len) {
        if (node.is_string()) {
          std::string s = node.get<std::string>();
          strncpy(
              dest, s.c_str(),
              max_len - 1);
          dest[max_len - 1] = '\0';
        }
      };
  safe_copy(j.value("title", ""), e.title, sizeof(e.title));
  safe_copy(j.value("username", ""), e.username, sizeof(e.username));
  safe_copy(j.value("password", ""), e.password, sizeof(e.password));
  safe_copy(j.value("url", ""), e.url, sizeof(e.url));
  safe_copy(j.value("notes", ""), e.notes, sizeof(e.notes));
}

#endif // CREDENTIALS_JSON_HPP
