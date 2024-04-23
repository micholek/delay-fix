#include "reg.h"

#include <array>
#include <iostream>
#include <print>

enum class DriverValue : uint8_t {
    Desc,
    Version,
    Date,
    ProviderName,
    _Count,
};

enum class PowerSettingsValue : uint8_t {
    ConsIdleTime,
    PerfIdleTime,
    IdlePowerState,
    _Count,
};

struct Driver {
    std::string desc;
    std::string version;
    std::string date;
    std::string provider_name;

    using enum DriverValue;

    Driver(std::span<const std::string> data)
        : desc {data[std::to_underlying(Desc)]},
          version {data[std::to_underlying(Version)]},
          date {data[std::to_underlying(Date)]},
          provider_name {data[std::to_underlying(ProviderName)]} {}

    static auto create_value_names() {
        std::array<std::string, std::to_underlying(_Count)> arr;
        arr[std::to_underlying(Desc)] = "DriverDesc";
        arr[std::to_underlying(Version)] = "DriverVersion";
        arr[std::to_underlying(Date)] = "DriverDate";
        arr[std::to_underlying(ProviderName)] = "ProviderName";
        return arr;
    }
};

struct PowerSettings {
    uint32_t cons_idle_time;
    uint32_t perf_idle_time;
    uint32_t idle_power_state;

    using enum PowerSettingsValue;

    constexpr PowerSettings(std::span<const uint32_t> data)
        : PowerSettings(data[std::to_underlying(ConsIdleTime)],
                        data[std::to_underlying(PerfIdleTime)],
                        data[std::to_underlying(IdlePowerState)]) {}

    constexpr PowerSettings(uint32_t cons_idle_time, uint32_t perf_idle_time,
                            uint32_t idle_power_state)
        : cons_idle_time {cons_idle_time}, perf_idle_time {perf_idle_time},
          idle_power_state {idle_power_state} {}

    static auto create_value_names() {
        std::array<std::string, std::to_underlying(_Count)> arr;
        arr[std::to_underlying(ConsIdleTime)] = "ConservationIdleTime";
        arr[std::to_underlying(PerfIdleTime)] = "PerformanceIdleTime";
        arr[std::to_underlying(IdlePowerState)] = "IdlePowerState";
        return arr;
    }
};

struct MediaInfo {
    size_t id;
    reg::Key main_key;
    reg::Key ps_key;
    Driver drv;
    PowerSettings ps;

    std::string description() const {
        return std::format(
            "#{} {} | version: {} | date: {} | provider name: {}\n"
            "(registry key path: {})",
            id, drv.desc, drv.version.c_str(), drv.date.c_str(),
            drv.provider_name.c_str(), main_key.path());
    }
};

void print_error(const reg::Error &err) {
    std::println(stderr, "{} (error code: {})", err.msg, err.code);
}

bool get_input(std::string &input) {
    if (std::getline(std::cin, input).fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }
    return true;
}

constexpr auto create_update_ps_values() {
    using enum PowerSettingsValue;
    std::array<uint32_t, std::to_underlying(_Count)> arr;
    arr[std::to_underlying(ConsIdleTime)] = 0xffffffff;
    arr[std::to_underlying(PerfIdleTime)] = 0xffffffff;
    arr[std::to_underlying(IdlePowerState)] = 0x3;
    return arr;
}

int main() {
    const std::string media_path = "SYSTEM\\CurrentControlSet\\Control\\Class\\"
                                   "{4d36e96c-e325-11ce-bfc1-08002be10318}";

    const reg::Key mk(reg::LocalMachine, media_path);
    if (!mk.valid()) {
        std::println(stderr, "Could not open a key {}", mk.path());
        return -1;
    }

    const auto msk_count_res = mk.get_subkeys_count();
    if (!msk_count_res.has_value()) {
        print_error(msk_count_res.error());
        return -1;
    }
    const uint32_t msk_count = msk_count_res.value();

    std::vector<MediaInfo> media_infos;
    media_infos.reserve(msk_count);

    for (uint32_t i = 0; i < msk_count; i++) {
        const auto msk_name_res = mk.enum_subkey_names(i);
        if (!msk_name_res.has_value()) {
            print_error(msk_name_res.error());
            continue;
        }
        const std::string msk_name = msk_name_res.value();

        reg::Key msk(mk, msk_name);
        if (!msk.valid()) {
            std::println(stderr, "Could not open a key '{}'", msk.path());
            continue;
        }

        reg::Key psk(msk, "PowerSettings");
        if (!psk.valid()) {
            std::println(stderr, "Could not open a key '{}'", psk.path());
            continue;
        }

        const std::array ps_value_names = PowerSettings::create_value_names();
        const auto ps_values_res = psk.read_u32_values(ps_value_names);
        if (!ps_values_res.has_value()) {
            print_error(ps_values_res.error());
            continue;
        }
        const std::vector<uint32_t> ps_values = ps_values_res.value();

        const std::array drv_value_names = Driver::create_value_names();
        const auto drv_values_res = msk.read_string_values(drv_value_names);
        if (!drv_values_res.has_value()) {
            print_error(drv_values_res.error());
            continue;
        }
        const std::vector<std::string> drv_values = drv_values_res.value();

        media_infos.push_back(MediaInfo {
            .id = media_infos.size(),
            .main_key = std::move(msk),
            .ps_key = std::move(psk),
            .drv = Driver(drv_values),
            .ps = PowerSettings(ps_values),
        });
    }

    const size_t mi_size = media_infos.size();
    if (!mi_size) {
        std::println(stderr, "No media instances found!");
        return 0;
    }

    std::print("Found {} media instances:\n\n", mi_size);
    for (size_t i = 0; i < mi_size; i++) {
        const auto &mi = media_infos[i];
        std::println("{}\n"
                     "Conservation Idle Time = {:#010x}\n"
                     " Performance Idle Time = {:#010x}\n"
                     "      Idle Power State = {:#010x}\n\n",
                     mi.description(), mi.ps.cons_idle_time,
                     mi.ps.perf_idle_time, mi.ps.idle_power_state);
    }

    size_t choice;
    for (;;) {
        std::print("Select media instance (0-{}) >> ", mi_size - 1);
        std::string input;
        if (get_input(input)) {
            const char *input_end = input.data() + input.size();
            const auto [conv_end, err] =
                std::from_chars(input.data(), input_end, choice);
            if (err == std::errc {} && conv_end == input_end &&
                choice < mi_size) {
                break;
            }
        }
    }

    const MediaInfo &mi = media_infos[choice];
    static constexpr const std::array update_ps_values =
        create_update_ps_values();
    static constexpr const PowerSettings update_ps(update_ps_values);

    std::print("Selected {}\n"
               "The program is about to update device's power settings to "
               "the following values:\n"
               "Conservation Idle Time = {:#010x} -> {:#010x}\n"
               " Performance Idle Time = {:#010x} -> {:#010x}\n"
               "      Idle Power State = {:#010x} -> {:#010x}\n\n",
               mi.description(), mi.ps.cons_idle_time, update_ps.cons_idle_time,
               mi.ps.perf_idle_time, update_ps.perf_idle_time,
               mi.ps.idle_power_state, update_ps.idle_power_state);

    bool update;
    for (;;) {
        std::print("Do you want to proceed? (y/n) >> ");
        std::string input;
        if (get_input(input) && (input == "y" || input == "n")) {
            update = (input == "y");
            break;
        }
    }

    if (update) {
        const std::array ps_value_names = PowerSettings::create_value_names();
        for (size_t i = 0; i < update_ps_values.size(); i++) {
            const uint32_t &value = update_ps_values[i];
            const auto write_res = mi.ps_key.write_binary_value(
                ps_value_names[i],
                std::span((uint8_t *) &value, sizeof(value)));
            if (write_res.fail) {
                print_error(write_res.error);
            }
        }
        std::println("Settings have been updated");
    } else {
        std::println("Aborting");
    }
    return 0;
}
