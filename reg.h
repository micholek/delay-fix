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

class Key {
    HKEY k_;

  public:
    // TODO: create global predefined objects in reg namespace instead (how?)
    // TODO: create methods for the rest of predefined keys
    static Key &LOCAL_MACHINE() {
        static Key key(HKEY_LOCAL_MACHINE);
        return key;
    }

    // TODO: implement special methods
    // ~Key();
    // Key(const Key &);
    // Key &operator=(const Key &);
    // Key(Key &&) = delete;
    // Key &operator=(Key &&);

    Result<Key> open_key(std::string key_name) const;
    Result<uint32_t> get_subkeys_count() const;
    Result<std::string> enum_subkey_names(uint32_t idx) const;
    Result<uint32_t> get_u32(std::string value_name) const;
    Result<std::vector<uint32_t>>
    get_u32s(const std::vector<std::string> &value_names) const;
    Result<std::string> get_string(std::string value_name) const;
    Result<std::vector<std::string>>
    get_strings(const std::vector<std::string> &value_names) const;

  private:
    // Keys have to be created with Key::open_key
    explicit Key(HKEY k) : k_ {k} {};
};

void print_error(Error err);

} // namespace reg
