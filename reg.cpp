#include "reg.h"
#include <format>
#include <print>

namespace {

std::string create_msg(std::string desc, std::string param) {
    return std::format("{} '{}'", desc, param);
}

} // namespace

#define RETURN(Winapi_Result, Expected_Value, Error_Message) \
    do {                                                     \
        if ((Winapi_Result) == ERROR_SUCCESS) {              \
            return (Expected_Value);                         \
        }                                                    \
        return std::unexpected(reg::Error {                  \
            .code = (Winapi_Result),                         \
            .msg = (Error_Message),                          \
        });                                                  \
    } while (0)

namespace reg {

Result<Key> Key::open_key(std::string key_name) const {
    HKEY opened_key;
    LSTATUS res = RegOpenKeyExA(k_, key_name.c_str(), 0, KEY_READ | KEY_WRITE,
                                &opened_key);
    RETURN(res, Key(opened_key), create_msg("Failed to open a key", key_name));
}

Result<uint32_t> Key::get_subkeys_count() const {
    DWORD subkeys_count;
    LSTATUS res =
        RegQueryInfoKeyA(k_, 0, 0, 0, &subkeys_count, 0, 0, 0, 0, 0, 0, 0);
    RETURN(res, subkeys_count, "Failed to get subkeys count");
}

Result<std::string> Key::enum_subkey_names(uint32_t index) const {
    char subkey_name[64];
    DWORD size = sizeof(subkey_name);
    LSTATUS res = RegEnumKeyExA(k_, index, subkey_name, &size, 0, 0, 0, 0);
    RETURN(res, std::string {subkey_name},
           create_msg("Failed to get subkey name with index",
                      std::to_string(index)));
}

Result<uint32_t> Key::get_u32(std::string value_name) const {
    uint32_t value;
    DWORD size = sizeof(value);
    LSTATUS res =
        RegGetValueA(k_, 0, value_name.c_str(), RRF_RT_DWORD, 0, &value, &size);
    RETURN(res, value, create_msg("Failed to get u32 value", value_name));
}

Result<std::vector<uint32_t>>
Key::get_u32s(const std::vector<std::string> &value_names) const {
    std::vector<uint32_t> values(value_names.size());
    for (size_t i = 0; i < values.size(); i++) {
        auto value_res = get_u32(value_names[i]);
        if (!value_res.has_value()) {
            Error err = value_res.error();
            return std::unexpected(Error {
                .code = err.code,
                .msg = std::string {"Failed to get multiple u32 values: "} +
                       err.msg,
            });
        }
        values[i] = value_res.value();
    }
    return values;
}

Result<std::string> Key::get_string(std::string value_name) const {
    char value[64];
    DWORD size = sizeof(value);
    LSTATUS res =
        RegGetValueA(k_, 0, value_name.c_str(), RRF_RT_REG_SZ, 0, value, &size);
    RETURN(res, std::string {value},
           create_msg("Failed to get string value", value_name));
}

Result<std::vector<std::string>>
Key::get_strings(const std::vector<std::string> &value_names) const {
    std::vector<std::string> values(value_names.size());
    for (size_t i = 0; i < values.size(); i++) {
        auto value_res = get_string(value_names[i]);
        if (!value_res.has_value()) {
            Error err = value_res.error();
            return std::unexpected(Error {
                .code = err.code,
                .msg = std::string {"Failed to get multiple string values: "} +
                       err.msg,
            });
        }
        values[i] = value_res.value();
    }
    return values;
}

void print_error(Error err) {
    std::println(stderr, "{} (error code: {})", err.msg, err.code);
}

} // namespace reg
