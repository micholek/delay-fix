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

RegRes<HKEY> reg_open_key(HKEY key, std::string subkey_name) {
    HKEY opened_key;
    LSTATUS res = RegOpenKeyExA(key, subkey_name.c_str(), 0,
                                KEY_READ | KEY_WRITE, &opened_key);
    RETURN(res, opened_key, create_msg("Failed to open a key", subkey_name));
}

RegRes<DWORD> reg_get_subkeys_count(HKEY key) {
    DWORD subkeys_count;
    LSTATUS res =
        RegQueryInfoKeyA(key, 0, 0, 0, &subkeys_count, 0, 0, 0, 0, 0, 0, 0);
    RETURN(res, subkeys_count, "Failed to get subkeys count");
}

RegRes<std::string> reg_enum_subkey_names(HKEY key, DWORD index) {
    char subkey_name[64];
    DWORD size = sizeof(subkey_name);
    LSTATUS res = RegEnumKeyExA(key, index, subkey_name, &size, 0, 0, 0, 0);
    RETURN(res, std::string {subkey_name},
           create_msg("Failed to get subkey name with index",
                      std::to_string(index)));
}

RegRes<DWORD> reg_get_dword(HKEY key, std::string value_name) {
    DWORD value;
    DWORD size = sizeof(value);
    LSTATUS res = RegGetValueA(key, 0, value_name.c_str(), RRF_RT_DWORD, 0,
                               &value, &size);
    RETURN(res, value, create_msg("Failed to get DWORD value", value_name));
}

RegRes<std::vector<DWORD>>
reg_get_dwords(HKEY key, const std::vector<std::string> &value_names) {
    std::vector<DWORD> values(value_names.size());
    for (size_t i = 0; i < values.size(); i++) {
        auto value_res = reg_get_dword(key, value_names[i]);
        if (!value_res.has_value()) {
            Error err = value_res.error();
            return std::unexpected(Error {
                .code = err.code,
                .msg = std::string {"Failed to get multiple DWORD values: "} +
                       err.msg,
            });
        }
        values[i] = value_res.value();
    }
    return values;
}

RegRes<std::string> reg_get_string(HKEY key, std::string value_name) {
    char value[64];
    DWORD size = sizeof(value);
    LSTATUS res = RegGetValueA(key, 0, value_name.c_str(), RRF_RT_REG_SZ, 0,
                               value, &size);
    RETURN(res, std::string {value},
           create_msg("Failed to get string value", value_name));
}

RegRes<std::vector<std::string>>
reg_get_strings(HKEY key, const std::vector<std::string> &value_names) {
    std::vector<std::string> values(value_names.size());
    for (size_t i = 0; i < values.size(); i++) {
        auto value_res = reg_get_string(key, value_names[i]);
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
