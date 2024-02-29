#pragma once

#include <expected>
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
    // Creates and opens a new subkey of a system key
    Key(SystemKey sk, std::string subkey_name);

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
    get_u32s(const std::vector<std::string> &value_names) const;
    Result<std::string> get_string(std::string value_name) const;
    Result<std::vector<std::string>>
    get_strings(const std::vector<std::string> &value_names) const;

    bool is_valid() const;
    Error get_error() const;
    std::string get_path() const;

  private:
    uintptr_t *k_ {nullptr};
    Error err_;
    std::string path_;

    Key(uintptr_t *k, std::string subkey_name, std::string parent_path);
    void update_error_(int32_t res, std::string msg);
};

void print_error(Error err);

} // namespace reg
