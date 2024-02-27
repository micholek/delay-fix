#include "reg.h"
#include <format>

namespace {

reg::ErrorMessage get_error_message(LSTATUS error_status) {
    char msg_buf[128];
    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, error_status, 0, msg_buf,
                        sizeof(msg_buf), 0)) {
        return std::vformat("[{}] Some error occured",
                            std::make_format_args(error_status));
    }
    return std::vformat("[{}] {}",
                        std::make_format_args(error_status, msg_buf));
}

} // namespace

#define RETURN(Result, Expected_Value)                           \
    do {                                                         \
        if ((Result) != ERROR_SUCCESS) {                         \
            return std::unexpected(get_error_message((Result))); \
        }                                                        \
        return (Expected_Value);                                 \
    } while (0)

namespace reg {

RegRes<HKEY> reg_open_key(HKEY key, std::string subkey_name) {
    HKEY opened_key;
    LSTATUS res = RegOpenKeyExA(key, subkey_name.c_str(), 0,
                                KEY_READ | KEY_WRITE, &opened_key);
    RETURN(res, opened_key);
}

RegRes<DWORD> reg_get_subkeys_count(HKEY key) {
    DWORD subkeys_count;
    LSTATUS res =
        RegQueryInfoKeyA(key, 0, 0, 0, &subkeys_count, 0, 0, 0, 0, 0, 0, 0);
    RETURN(res, subkeys_count);
}

RegRes<std::string> reg_enum_subkey_names(HKEY key, DWORD index) {
    char subkey_name[64];
    DWORD size = sizeof(subkey_name);
    LSTATUS res = RegEnumKeyExA(key, index, subkey_name, &size, 0, 0, 0, 0);
    RETURN(res, std::string {subkey_name});
}

RegRes<DWORD> reg_get_dword(HKEY key, std::string value_name) {
    DWORD value;
    DWORD size = sizeof(value);
    LSTATUS res = RegGetValueA(key, 0, value_name.c_str(), RRF_RT_DWORD, 0,
                               &value, &size);
    RETURN(res, value);
}

RegRes<std::vector<DWORD>>
reg_get_dwords(HKEY key, const std::vector<std::string> &value_names) {
    std::vector<DWORD> values(value_names.size());
    for (size_t i = 0; i < values.size(); i++) {
        auto value_res = reg_get_dword(key, value_names[i]);
        if (!value_res.has_value()) {
            return std::unexpected(value_res.error());
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
    RETURN(res, std::string {value});
}

RegRes<std::vector<std::string>>
reg_get_strings(HKEY key, const std::vector<std::string> &value_names) {
    std::vector<std::string> values(value_names.size());
    for (size_t i = 0; i < values.size(); i++) {
        auto value_res = reg_get_string(key, value_names[i]);
        if (!value_res.has_value()) {
            return std::unexpected(value_res.error());
        }
        values[i] = value_res.value();
    }
    return values;
}

} // namespace reg
