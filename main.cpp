#include "reg.h"

#include <array>
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

    PowerSettings(std::span<const uint32_t> data)
        : cons_idle_time {data[std::to_underlying(ConsIdleTime)]},
          perf_idle_time {data[std::to_underlying(PerfIdleTime)]},
          idle_power_state {data[std::to_underlying(IdlePowerState)]} {}

    static auto create_value_names() {
        std::array<std::string, std::to_underlying(_Count)> arr;
        arr[std::to_underlying(ConsIdleTime)] = "ConservationIdleTime";
        arr[std::to_underlying(PerfIdleTime)] = "PerformanceIdleTime";
        arr[std::to_underlying(IdlePowerState)] = "IdlePowerState";
        return arr;
    }
};

struct MediaInfo {
    std::string reg_key_path;
    Driver drv;
    PowerSettings ps;
};

void print_error(const reg::Error &err) {
    std::println(stderr, "{} (error code: {})", err.msg, err.code);
}

int main() {
    const std::string media_path = "SYSTEM\\CurrentControlSet\\Control\\Class\\"
                                   "{4d36e96c-e325-11ce-bfc1-08002be10318}";

    reg::Key mk(reg::LocalMachine, media_path);
    if (!mk.valid()) {
        print_error(mk.error());
        return -1;
    }

    auto msk_count_res = mk.get_subkeys_count();
    if (!msk_count_res.has_value()) {
        print_error(msk_count_res.error());
        return -1;
    }
    uint32_t msk_count = msk_count_res.value();

    std::vector<MediaInfo> media_infos;
    media_infos.reserve(msk_count);

    for (uint32_t i = 0; i < msk_count; i++) {
        auto msk_name_res = mk.enum_subkey_names(i);
        if (!msk_name_res.has_value()) {
            print_error(msk_name_res.error());
            continue;
        }
        std::string msk_name = msk_name_res.value();

        reg::Key msk(mk, msk_name);
        if (!msk.valid()) {
            print_error(msk.error());
            continue;
        }

        reg::Key psk(msk, "PowerSettings");
        if (!psk.valid()) {
            print_error(psk.error());
            continue;
        }

        const std::array ps_value_names = PowerSettings::create_value_names();
        auto ps_values_res = psk.get_u32s(ps_value_names);
        if (!ps_values_res.has_value()) {
            print_error(ps_values_res.error());
            continue;
        }
        std::vector<uint32_t> ps_values = ps_values_res.value();

        const std::array drv_value_names = Driver::create_value_names();
        auto drv_values_res = msk.get_strings(drv_value_names);
        if (!drv_values_res.has_value()) {
            print_error(drv_values_res.error());
            continue;
        }
        std::vector<std::string> drv_values = drv_values_res.value();

        media_infos.push_back(MediaInfo {
            .reg_key_path = msk.path(),
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
        std::println("#{} {} | version: {} | date: {} | provider name: {}\n"
                     "(registry key path: {}\n"
                     "Conservation Idle Time = {:#010x}\n"
                     " Performance Idle Time = {:#010x}\n"
                     "      Idle Power State = {:#010x}\n\n",
                     i, mi.drv.desc, mi.drv.version.c_str(), mi.drv.date,
                     mi.drv.provider_name.c_str(), mi.reg_key_path,
                     mi.ps.cons_idle_time, mi.ps.perf_idle_time,
                     mi.ps.idle_power_state);
    }
    return 0;
}
