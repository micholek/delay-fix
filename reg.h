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

template <class T> using Result = std::expected<T, Error>;

Result<HKEY> reg_open_key(HKEY key, std::string subkey_name);

Result<DWORD> reg_get_subkeys_count(HKEY key);

Result<std::string> reg_enum_subkey_names(HKEY key, DWORD idx);

Result<DWORD> reg_get_dword(HKEY key, std::string value_name);

Result<std::vector<DWORD>>
reg_get_dwords(HKEY key, const std::vector<std::string> &value_names);

Result<std::string> reg_get_string(HKEY key, std::string value_name);

Result<std::vector<std::string>>
reg_get_strings(HKEY key, const std::vector<std::string> &value_names);

void print_error(Error err);

} // namespace reg
