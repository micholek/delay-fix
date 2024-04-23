#define BOOST_TEST_MODULE key_test_module
#include "reg.h"
#include <boost/test/unit_test.hpp>
// clang-format off
#include <windows.h>
#include <detours.h> // include after windows.h
// clang-format on

LSTATUS WINAPI RegOpenKeyEx_nop(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions,
                                REGSAM samDesired, PHKEY phkResult) {
    (void) hKey;
    (void) lpSubKey;
    (void) ulOptions;
    (void) samDesired;
    (void) phkResult;
    return ERROR_SUCCESS;
}

LSTATUS WINAPI RegOpenKeyEx_success(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions,
                                    REGSAM samDesired, PHKEY phkResult) {
    (void) hKey;
    (void) lpSubKey;
    (void) ulOptions;
    (void) samDesired;
    *phkResult = (HKEY) 123;
    return ERROR_SUCCESS;
}

LSTATUS WINAPI RegCloseKey_success(HKEY hKey) {
    (void) hKey;
    return ERROR_SUCCESS;
}

template <typename T> struct Hook {
    Hook(T func, T detour, const char *name = nullptr)
        : func {func}, detour {detour}, name {name} {
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&func, detour);
        DetourTransactionCommit();
        if (name == nullptr) {
            BOOST_TEST_MESSAGE("  HOOKED @ " << detour);
        } else {
            BOOST_TEST_MESSAGE("  HOOKED " << name << " @ " << detour);
        }
    }
    ~Hook() {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&func, detour);
        DetourTransactionCommit();
        if (name == nullptr) {
            BOOST_TEST_MESSAGE("UNHOOKED @ " << detour);
        } else {
            BOOST_TEST_MESSAGE("UNHOOKED " << name << " @ " << detour);
        }
    }

    T func;
    T detour;
    const char *name;
};

#define _TO_STR(X) #X
#define CREATE_HOOK(Func, Detour) \
    Hook<decltype(&Func)> Detour##_hook(&Func, Detour, _TO_STR(Detour))

BOOST_AUTO_TEST_CASE(predefined_system) {
    BOOST_TEST(reg::LocalMachine.valid());
    BOOST_TEST(reg::LocalMachine.system());
    BOOST_TEST(reg::LocalMachine.path() == "HKEY_LOCAL_MACHINE");
}

BOOST_AUTO_TEST_CASE(constructed_from_system_and_empty_subkey) {
    const auto reg_open_key_nop = [](HKEY hKey, LPCSTR lpSubKey,
                                     DWORD ulOptions, REGSAM samDesired,
                                     PHKEY phkResult) -> LSTATUS {
        (void) hKey;
        (void) lpSubKey;
        (void) ulOptions;
        (void) samDesired;
        (void) phkResult;
        return ERROR_SUCCESS;
    };

    CREATE_HOOK(RegOpenKeyExA, reg_open_key_nop);

    reg::Key key(reg::LocalMachine, "");
    BOOST_TEST(key.valid());
    BOOST_TEST(key.system());
    BOOST_TEST(reg::LocalMachine.path() == "HKEY_LOCAL_MACHINE");
}

BOOST_AUTO_TEST_CASE(constructed_from_system_and_subkey) {
    const auto reg_open_key_success = [](HKEY hKey, LPCSTR lpSubKey,
                                         DWORD ulOptions, REGSAM samDesired,
                                         PHKEY phkResult) -> LSTATUS {
        (void) hKey;
        (void) lpSubKey;
        (void) ulOptions;
        (void) samDesired;
        *phkResult = (HKEY) 123;
        return ERROR_SUCCESS;
    };

    CREATE_HOOK(RegOpenKeyExA, reg_open_key_success);

    reg::Key key(reg::LocalMachine, "subkey");
    BOOST_TEST(key.valid());
    BOOST_TEST(!key.system());
    BOOST_TEST(key.path() == "HKEY_LOCAL_MACHINE\\subkey");
}

BOOST_AUTO_TEST_CASE(constructed_with_move_constructor) {
    CREATE_HOOK(RegOpenKeyExA, RegOpenKeyEx_success);

    reg::Key key(reg::LocalMachine, "subkey");
    reg::Key new_key = std::move(key);
    BOOST_TEST(!key.valid());
    BOOST_TEST(new_key.valid());
    BOOST_TEST(!new_key.system());
    BOOST_TEST(new_key.path() == "HKEY_LOCAL_MACHINE\\subkey");
}

BOOST_AUTO_TEST_CASE(constructed_with_move_assignment) {
    CREATE_HOOK(RegOpenKeyExA, RegOpenKeyEx_success);
    CREATE_HOOK(RegCloseKey, RegCloseKey_success);

    reg::Key key(reg::LocalMachine, "abc");
    reg::Key key2(reg::LocalMachine, "def");
    key2 = std::move(key);
    BOOST_TEST(!key.valid());
    BOOST_TEST(key2.valid());
    BOOST_TEST(!key2.system());
    BOOST_TEST(key2.path() == "HKEY_LOCAL_MACHINE\\abc");
}

BOOST_AUTO_TEST_CASE(subkeys_count_success) {
    const auto reg_query_info_key_success =
        [](HKEY hKey, LPSTR lpClass, LPDWORD lpcchClass, LPDWORD lpReserved,
           LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen,
           LPDWORD lpcbMaxClassLen, LPDWORD lpcValues,
           LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen,
           LPDWORD lpcbSecurityDescriptor,
           PFILETIME lpftLastWriteTime) -> LSTATUS {
        (void) hKey;
        (void) lpClass;
        (void) lpcchClass;
        (void) lpReserved;
        (void) lpcbMaxSubKeyLen;
        (void) lpcbMaxClassLen;
        (void) lpcValues;
        (void) lpcbMaxValueNameLen;
        (void) lpcbMaxValueLen;
        (void) lpcbSecurityDescriptor;
        (void) lpftLastWriteTime;
        *lpcSubKeys = 5;
        return ERROR_SUCCESS;
    };

    CREATE_HOOK(RegQueryInfoKeyA, reg_query_info_key_success);

    const auto subkeys_count_res = reg::LocalMachine.get_subkeys_count();
    BOOST_TEST_REQUIRE(subkeys_count_res.has_value());
    BOOST_TEST(subkeys_count_res.value() == 5U);
}

BOOST_AUTO_TEST_CASE(subkeys_count_failure) {
    const auto reg_query_info_key_failure =
        [](HKEY hKey, LPSTR lpClass, LPDWORD lpcchClass, LPDWORD lpReserved,
           LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen,
           LPDWORD lpcbMaxClassLen, LPDWORD lpcValues,
           LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen,
           LPDWORD lpcbSecurityDescriptor,
           PFILETIME lpftLastWriteTime) -> LSTATUS {
        (void) hKey;
        (void) lpClass;
        (void) lpcchClass;
        (void) lpReserved;
        (void) lpcSubKeys;
        (void) lpcbMaxSubKeyLen;
        (void) lpcbMaxClassLen;
        (void) lpcValues;
        (void) lpcbMaxValueNameLen;
        (void) lpcbMaxValueLen;
        (void) lpcbSecurityDescriptor;
        (void) lpftLastWriteTime;
        return ERROR_NOT_SUPPORTED; // example failure error
    };

    CREATE_HOOK(RegQueryInfoKeyA, reg_query_info_key_failure);

    const auto subkeys_count_res = reg::LocalMachine.get_subkeys_count();
    BOOST_TEST_REQUIRE(!subkeys_count_res.has_value());
    const reg::Error &err = subkeys_count_res.error();
    BOOST_TEST(err.code == ERROR_NOT_SUPPORTED);
    BOOST_TEST(err.msg == "Failed to get subkeys count");
}

BOOST_AUTO_TEST_CASE(enum_subkey_names_success) {
    const auto reg_enum_key_success =
        [](HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcchName,
           LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcchClass,
           PFILETIME lpftLastWriteTime) -> LSTATUS {
        (void) hKey;
        (void) lpReserved;
        (void) lpClass;
        (void) lpcchClass;
        (void) lpftLastWriteTime;
        static const char *subkey_names[] = {"subkey0", "subkey1"};
        strncpy_s(lpName, *lpcchName, subkey_names[dwIndex],
                  strlen(subkey_names[dwIndex]));
        return ERROR_SUCCESS;
    };

    CREATE_HOOK(RegEnumKeyExA, reg_enum_key_success);

    BOOST_TEST(reg::LocalMachine.enum_subkey_names(0).value_or("error0") ==
               "subkey0");
    BOOST_TEST(reg::LocalMachine.enum_subkey_names(1).value_or("error1") ==
               "subkey1");
}

BOOST_AUTO_TEST_CASE(enum_subkey_names_index_out_of_range) {
    const auto reg_enum_key_no_more_items =
        [](HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcchName,
           LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcchClass,
           PFILETIME lpftLastWriteTime) -> LSTATUS {
        (void) hKey;
        (void) dwIndex;
        (void) lpName;
        (void) lpcchName;
        (void) lpReserved;
        (void) lpClass;
        (void) lpcchClass;
        (void) lpftLastWriteTime;
        return ERROR_NO_MORE_ITEMS;
    };

    CREATE_HOOK(RegEnumKeyExA, reg_enum_key_no_more_items);

    const auto enum_subkey_res = reg::LocalMachine.enum_subkey_names(4);
    BOOST_REQUIRE(!enum_subkey_res.has_value());
    const reg::Error &err = enum_subkey_res.error();
    BOOST_TEST(err.code == ERROR_NO_MORE_ITEMS);
    BOOST_TEST(err.msg == "Failed to get subkey name with index '4'");
}
