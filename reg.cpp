#include "reg.h"
#include <format>
#include <windows.h>

namespace {

constexpr std::string system_key_to_path(reg::SystemKey sk) {
    switch (sk) {
    case reg::SystemKey::LocalMachine:
        return "HKEY_LOCAL_MACHINE";
    }
    return "HKEY_LOCAL_MACHINE"; // fallback for now
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

std::string create_msg(std::string desc, std::string param) {
    return std::format("{} '{}'", desc, param);
}

std::string create_path(std::string parent_path,
                        const std::string &subkey_name) {
    if (!subkey_name.empty()) {
        return std::format("{}\\{}", parent_path, subkey_name);
    }
    return parent_path;
}

} // namespace

namespace reg {

Key::Key(SystemKey sk)
    : k_ {system_key_to_key_ptr(sk)}, system_ {true},
      path_ {system_key_to_path(sk)} {}

Key::Key(const Key &k, const std::string &subkey_name)
    : k_ {(uintptr_t) nullptr}, system_ {k.system_ && subkey_name.empty()},
      path_ {create_path(k.path_, subkey_name)} {
    if (!system_) {
        RegOpenKeyExA((HKEY) k.k_, subkey_name.c_str(), 0, KEY_READ | KEY_WRITE,
                      (HKEY *) &k_);
    }
}

Key::Key(Key &&other)
    : k_ {other.k_}, system_ {other.system_}, path_ {std::move(other.path_)} {
    other.k_ = (uintptr_t) nullptr;
}

Key &Key::operator=(Key &&other) {
    if (this != &other) {
        if (!system_ && valid()) {
            RegCloseKey((HKEY) k_);
        }
        k_ = other.k_;
        system_ = other.system_;
        path_ = std::move(other.path_);
        other.k_ = (uintptr_t) nullptr;
    }
    return *this;
}

Key::~Key() {
    if (!system_ && valid()) {
        RegCloseKey((HKEY) k_);
        k_ = (uintptr_t) nullptr;
    }
}

// TODO: Rewrite to some templated function
#define RETURN(Winapi_Result, Expected_Value, Error_Message) \
    do {                                                     \
        if ((Winapi_Result) == ERROR_SUCCESS) {              \
            return Expected_Value;                           \
        }                                                    \
        return std::unexpected(reg::Error {                  \
            .code = (Winapi_Result),                         \
            .msg = (Error_Message),                          \
        });                                                  \
    } while (0)

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

Result<uint32_t> Key::read_u32_value(std::string value_name) const {
    uint32_t value;
    DWORD size = sizeof(value);
    LSTATUS res = RegGetValueA((HKEY) k_, 0, value_name.c_str(), RRF_RT_DWORD,
                               0, &value, &size);
    RETURN(res, value, create_msg("Failed to get u32 value", value_name));
}

Result<std::vector<uint32_t>>
Key::read_u32_values(std::span<const std::string> value_names) const {
    std::vector<uint32_t> values(value_names.size());
    for (size_t i = 0; i < values.size(); i++) {
        auto value_res = read_u32_value(value_names[i]);
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

Result<std::string> Key::read_string_value(std::string value_name) const {
    char value[64];
    DWORD size = sizeof(value);
    LSTATUS res = RegGetValueA((HKEY) k_, 0, value_name.c_str(), RRF_RT_REG_SZ,
                               0, value, &size);
    RETURN(res, std::string {value},
           create_msg("Failed to get string value", value_name));
}

Result<std::vector<std::string>>
Key::read_string_values(std::span<const std::string> value_names) const {
    std::vector<std::string> values(value_names.size());
    for (size_t i = 0; i < values.size(); i++) {
        auto value_res = read_string_value(value_names[i]);
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

Result<void> Key::write_binary_value(const std::string &value_name,
                                     std::span<const uint8_t> data) const {
    return write_subkey_binary_value("", value_name, data);
}

Result<void>
Key::write_subkey_binary_value(const std::string &subkey_name,
                               const std::string &value_name,
                               std::span<const uint8_t> data) const {
    LSTATUS res =
        RegSetKeyValueA((HKEY) k_, subkey_name.c_str(), value_name.c_str(),
                        REG_BINARY, data.data(), (DWORD) data.size_bytes());
    RETURN(res, {}, create_msg("Failed to write binary value", value_name));
}

Result<void> Key::write_u32_value(const std::string &value_name,
                                  uint32_t value) const {
    return write_subkey_u32_value("", value_name, value);
}

Result<void> Key::write_subkey_u32_value(const std::string &subkey_name,
                                         const std::string &value_name,
                                         uint32_t value) const {
    LSTATUS res =
        RegSetKeyValueA((HKEY) k_, subkey_name.c_str(), value_name.c_str(),
                        REG_DWORD, &value, sizeof(value));
    RETURN(res, {}, create_msg("Failed to write u32 value", value_name));
}

bool Key::valid() const {
    return k_ != (uintptr_t) nullptr;
}

std::string Key::path() const {
    return path_;
}

} // namespace reg
