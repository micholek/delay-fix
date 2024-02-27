#pragma once

#include <expected>
#include <string>
#include <vector>
#include <windows.h>

namespace reg {

struct Error {
    LSTATUS code;
    std::string msg;
};

template <class T> using RegRes = std::expected<T, Error>;

RegRes<HKEY> reg_open_key(HKEY key, std::string subkey_name);

RegRes<DWORD> reg_get_subkeys_count(HKEY key);

RegRes<std::string> reg_enum_subkey_names(HKEY key, DWORD idx);

RegRes<DWORD> reg_get_dword(HKEY key, std::string value_name);

RegRes<std::vector<DWORD>>
reg_get_dwords(HKEY key, const std::vector<std::string> &value_names);

RegRes<std::string> reg_get_string(HKEY key, std::string value_name);

RegRes<std::vector<std::string>>
reg_get_strings(HKEY key, const std::vector<std::string> &value_names);

void print_error(Error err);

} // namespace reg
