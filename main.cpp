#include "reg.h"

#include <array>
#include <print>

enum MediaInstanceValue {
    MEDIA_INST_VAL_DRIVER_DESC,
    MEDIA_INST_VAL_DRIVER_VERSION,
    MEDIA_INST_VAL_DRIVER_DATA,
    MEDIA_INST_VAL_PROVIDER_NAME,
    _MEDIA_INST_VAL_COUNT
};

enum PowerSettingsValue {
    POWER_SETTINGS_VAL_CONS_IDLE_TIME,
    POWER_SETTINGS_VAL_PERF_IDLE_TIME,
    POWER_SETTINGS_VAL_IDLE_POWER_STATE,
    _POWER_SETTINGS_VAL_COUNT
};

struct MediaInfo {
    std::string reg_key_path;
    std::string driver_desc;
    std::string driver_version;
    std::string driver_data;
    std::string provider_name;
    DWORD conservation_idle_time;
    DWORD performance_idle_time;
    DWORD idle_power_state;
};

std::string get_key_path(std::string parent, std::string key) {
    return std::string {
               "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\"
               "{4d36e96c-e325-11ce-bfc1-08002be10318}\\"} +
           key;
}

template <typename... Args>
inline void print_error(const std::format_string<Args...> fmt, Args &&...args) {
    std::println(stderr, fmt, args...);
}

int main() {
    const std::string media_path = "SYSTEM\\CurrentControlSet\\Control\\Class\\"
                                   "{4d36e96c-e325-11ce-bfc1-08002be10318}";

    auto mk_res = reg_open_key(HKEY_LOCAL_MACHINE, media_path);
    if (!mk_res.has_value()) {
        print_error("Could not open media key");
        return -1;
    }
    HKEY mk = mk_res.value();

    auto msk_count_res = reg_get_subkeys_count(mk);
    if (!msk_count_res.has_value()) {
        print_error("Could not count media subkeys: {}", msk_count_res.error());
        return -1;
    }
    DWORD msk_count = msk_count_res.value();

    std::vector<MediaInfo> media_infos;
    media_infos.reserve(msk_count);

    for (DWORD i = 0; i < msk_count; i++) {
        auto msk_name_res = reg_enum_subkey_names(mk, i);
        if (!msk_name_res.has_value()) {
            std::println(stderr,
                         "  [{}] Error when enumerating over media keys", i);
            continue;
        }
        std::string msk_name = msk_name_res.value();

        auto msk_res = reg_open_key(mk, msk_name);
        if (!msk_res.has_value()) {
            std::println(stderr, "Could not open media instance key");
            continue;
        }
        HKEY msk = msk_res.value();

        auto psk_res = reg_open_key(msk, "PowerSettings");
        if (!psk_res.has_value()) {
            print_error("Could not open PowerSettings key: {}",
                        psk_res.error());
            continue;
        }
        HKEY psk = psk_res.value();

        const std::array<std::string, _POWER_SETTINGS_VAL_COUNT> ps_names = {
            "ConservationIdleTime",
            "PerformanceIdleTime",
            "IdlePowerState",
        };
        auto ps_values_res =
            reg_get_dwords(psk, std::vector<std::string>(std::begin(ps_names),
                                                         std::end(ps_names)));
        if (!ps_values_res.has_value()) {
            break;
        }
        std::vector<DWORD> ps_values = ps_values_res.value();

        const std::string mi_names[_MEDIA_INST_VAL_COUNT] = {
            "DriverDesc",
            "DriverVersion",
            "DriverDate",
            "ProviderName",
        };
        auto mi_values_res =
            reg_get_strings(msk, std::vector<std::string>(std::begin(mi_names),
                                                          std::end(mi_names)));
        if (!mi_values_res.has_value()) {
            break;
        }
        std::vector<std::string> mi_values = mi_values_res.value();

        MediaInfo mi = {
            .reg_key_path = get_key_path(media_path, msk_name),
            .driver_desc = mi_values[MEDIA_INST_VAL_DRIVER_DESC],
            .driver_version = mi_values[MEDIA_INST_VAL_DRIVER_VERSION],
            .driver_data = mi_values[MEDIA_INST_VAL_DRIVER_DATA],
            .provider_name = mi_values[MEDIA_INST_VAL_PROVIDER_NAME],
            .conservation_idle_time =
                ps_values[POWER_SETTINGS_VAL_CONS_IDLE_TIME],
            .performance_idle_time =
                ps_values[POWER_SETTINGS_VAL_PERF_IDLE_TIME],
            .idle_power_state = ps_values[POWER_SETTINGS_VAL_IDLE_POWER_STATE],
        };
        media_infos.push_back(mi);
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
                     i, mi.driver_desc, mi.driver_version.c_str(),
                     mi.driver_data, mi.provider_name.c_str(), mi.reg_key_path,
                     mi.conservation_idle_time, mi.performance_idle_time,
                     mi.idle_power_state);
    }
    return 0;
}
