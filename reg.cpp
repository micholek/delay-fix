#include "reg.h"
#include <format>
#include <print>
#include <windows.h>

namespace {

std::string create_msg(std::string desc, std::string param) {
    return std::format("{} '{}'", desc, param);
}

constexpr std::string system_key_to_path(reg::SystemKey sk) {
    switch (sk) {
    case reg::SystemKey::LocalMachine:
        return "HKEY_LOCAL_MACHINE";
    }
    return "HKEY_LOCAL_MACHINE"; // fallback for now
}

std::string create_path(std::string parent_path, std::string subkey_name) {
    if (!subkey_name.empty()) {
        return std::format("{}\\{}", parent_path, subkey_name);
    }
    return parent_path;
}

constexpr uintptr_t system_key_to_key_ptr(reg::SystemKey sk) {
    HKEY k;
    switch (sk) {
    case reg::SystemKey::LocalMachine:
        k = HKEY_LOCAL_MACHINE;
        break;
    default:
        k = HKEY_LOCAL_MACHINE; // fallback for now
    }
    return (uintptr_t) k;
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

Key::Key(SystemKey sk)
    : k_ {system_key_to_key_ptr(sk)}, system_ {true},
      path_ {system_key_to_path(sk)}, err_ {Error {}} {}

Key::Key(const Key &k, std::string subkey_name)
    : system_ {k.system_ && subkey_name.empty()} {
    if (system_) {
        k_ = k.k_;
        err_ = Error {};
        path_ = k.path_;
    } else {
        LSTATUS res = RegOpenKeyExA((HKEY) k.k_, subkey_name.c_str(), 0,
                                    KEY_READ | KEY_WRITE, (HKEY *) &k_);
        update_error_(res, create_msg("Could not open a key", subkey_name));
        path_ = create_path(k.path_, subkey_name);
    }
}

bool Key::valid() const {
    return k_ != (uintptr_t) nullptr;
}

Error Key::error() const {
    return err_;
}

Key::~Key() {
    if (!system_ && valid()) {
        LSTATUS res = RegCloseKey((HKEY) k_);
        update_error_(res, "Could not close the key");
        k_ = (uintptr_t) nullptr;
    }
}

Result<uint32_t> Key::get_subkeys_count() const {
    DWORD subkeys_count;
    LSTATUS res = RegQueryInfoKeyA((HKEY) k_, 0, 0, 0, &subkeys_count, 0, 0, 0,
                                   0, 0, 0, 0);
    RETURN(res, subkeys_count, "Failed to get subkeys count");
}

Result<std::string> Key::enum_subkey_names(uint32_t index) const {
    char subkey_name[64];
    DWORD size = sizeof(subkey_name);
    LSTATUS res =
        RegEnumKeyExA((HKEY) k_, index, subkey_name, &size, 0, 0, 0, 0);
    RETURN(res, std::string {subkey_name},
           create_msg("Failed to get subkey name with index",
                      std::to_string(index)));
}

Result<uint32_t> Key::get_u32(std::string value_name) const {
    uint32_t value;
    DWORD size = sizeof(value);
    LSTATUS res = RegGetValueA((HKEY) k_, 0, value_name.c_str(), RRF_RT_DWORD,
                               0, &value, &size);
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
    LSTATUS res = RegGetValueA((HKEY) k_, 0, value_name.c_str(), RRF_RT_REG_SZ,
                               0, value, &size);
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

std::string Key::path() const {
    return path_;
}

void Key::update_error_(int32_t res, std::string err_msg) {
    err_ = {
        .code = res,
        .msg = (res != ERROR_SUCCESS) ? err_msg : "",
    };
}

void print_error(Error err) {
    std::println(stderr, "{} (error code: {})", err.msg, err.code);
}

} // namespace reg
