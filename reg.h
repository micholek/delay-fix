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
    Key(const Key &k, std::string subkey_name);

    // Destroys and closes the key
    ~Key();

    // TODO: implement special methods
    // Key(const Key &);
    // Key &operator=(const Key &);
    // Key(Key &&) = delete;
    // Key &operator=(Key &&);

    Result<uint32_t> get_subkeys_count() const;
    Result<std::string> enum_subkey_names(uint32_t idx) const;
    Result<uint32_t> get_u32(std::string value_name) const;
    Result<std::vector<uint32_t>>
    get_u32s(std::span<const std::string> value_names) const;
    Result<std::string> get_string(std::string value_name) const;
    Result<std::vector<std::string>>
    get_strings(std::span<const std::string> value_names) const;

    bool valid() const;
    Error error() const;
    std::string path() const;

  private:
    uintptr_t k_;
    bool system_;
    std::string path_;
    Error err_;

    void update_error_(int32_t res, std::string msg);
};

// System key wrapped in global object for use in client code
static inline const Key LocalMachine(SystemKey::LocalMachine);

} // namespace reg
