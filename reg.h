#pragma once

#include <expected>
#include <span>
#include <string>
#include <vector>

namespace reg {

struct Error {
    int32_t code;
    std::string msg;
};

template <class T> using Result = std::expected<T, Error>;

enum class SystemKey { LocalMachine };

class Key {
  public:
    // Creates a system key. Should not be used in client code.
    Key(SystemKey sk);

    // Creates and opens a new subkey of another key
    Key(const Key &k, const std::string &subkey_name);

    // Destroys and closes the key
    ~Key();

    // Creates a key using move semantics
    Key(Key &&);

    // Closes the key and assigns a new one using move semantics
    Key &operator=(Key &&);

    Key(const Key &) = delete;
    Key &operator=(const Key &) = delete;

    Result<uint32_t> get_subkeys_count() const;
    Result<std::string> enum_subkey_names(uint32_t idx) const;
    Result<uint32_t> read_u32_value(std::string value_name) const;
    Result<std::vector<uint32_t>>
    read_u32_values(std::span<const std::string> value_names) const;
    Result<std::string> read_string_value(std::string value_name) const;
    Result<std::vector<std::string>>
    read_string_values(std::span<const std::string> value_names) const;

    Result<void> write_binary_value(const std::string &value_name,
                                    std::span<const uint8_t> data) const;
    Result<void> write_subkey_binary_value(const std::string &subkey_name,
                                           const std::string &value_name,
                                           std::span<const uint8_t> data) const;
    Result<void> write_u32_value(const std::string &value_name,
                                 uint32_t value) const;
    Result<void> write_subkey_u32_value(const std::string &subkey_name,
                                        const std::string &value_name,
                                        uint32_t value) const;

    bool valid() const;
    std::string path() const;

  private:
    uintptr_t k_;
    bool system_;
    std::string path_;
};

// System key wrapped in global object for use in client code
static inline const Key LocalMachine(SystemKey::LocalMachine);

} // namespace reg
