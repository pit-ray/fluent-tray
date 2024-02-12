#include "doctest.h"

#include "fluent_tray.hpp"

#include "fff.h"
#include <string>

DEFINE_FFF_GLOBALS ;

FAKE_VALUE_FUNC(
    int, MultiByteToWideChar,
    UINT, DWORD, const char*, int, LPWSTR, int) ;
FAKE_VALUE_FUNC(
    int, WideCharToMultiByte,
    UINT, DWORD, const wchar_t*, int, LPSTR, int, LPCCH, LPBOOL) ;

namespace
{
    std::wstring MultiByteToWideChar_dst{} ;
    int MultiByteToWideChar_custom_fake(
            UINT, DWORD, const char*, int, LPWSTR dst, int) {
        if(dst != NULL) {
            std::wcscpy(dst, MultiByteToWideChar_dst.c_str()) ;
        }
        return static_cast<int>(MultiByteToWideChar_dst.size()) ;
    }

    std::string WideCharToMultiByte_dst{} ;
    int WideCharToMultiByte_custom_fake(
            UINT, DWORD, const wchar_t*, int, LPSTR dst, int, LPCCH, LPBOOL) {
        if(dst != NULL) {
            std::strcpy(dst, WideCharToMultiByte_dst.c_str()) ;
        }
        return static_cast<int>(WideCharToMultiByte_dst.size()) ;
    }

}

using namespace fluent_tray ;

TEST_CASE("string test under Fake Windows API: ") {
    RESET_FAKE(MultiByteToWideChar) ;
    RESET_FAKE(WideCharToMultiByte) ;
    FFF_RESET_HISTORY() ;

    SUBCASE("Empty String") {
        int returns[] = {10, 10} ;
        SET_RETURN_SEQ(MultiByteToWideChar, returns, 2) ;

        std::wstring wstr ;
        CHECK(util::string2wstring(std::string(), wstr)) ;
        CHECK(wstr.empty()) ;
    }

    SUBCASE("Arguments Test") {
        std::string str("Hello") ;

        std::wstring expect(L"Hello") ;
        MultiByteToWideChar_dst = expect ;

        MultiByteToWideChar_fake.custom_fake = MultiByteToWideChar_custom_fake ;

        std::wstring wstr ;
        CHECK(util::string2wstring(str, wstr)) ;
        CHECK_EQ(wstr, expect) ;
        CHECK_EQ(strncmp(MultiByteToWideChar_fake.arg2_val, str.c_str(), str.length()), 0) ;
        CHECK_EQ(MultiByteToWideChar_fake.arg3_val, str.size()) ;
    }

    SUBCASE("The output size is invalid") {
        MultiByteToWideChar_fake.return_val = -1 ;
        std::wstring wstr ;
        CHECK_FALSE(util::string2wstring("Hello", wstr)) ;
    }

    SUBCASE("Failed Conversion") {
        int returns[2] = {10, -1} ;
        SET_RETURN_SEQ(MultiByteToWideChar, returns, 2) ;
        std::wstring wstr ;
        CHECK_FALSE(util::string2wstring("Hello", wstr)) ;
    }

    SUBCASE("The string is empty") {
        int returns[] = {10, 10} ;
        SET_RETURN_SEQ(WideCharToMultiByte, returns, 2) ;
        std::string str ;
        CHECK(util::wstring2string(std::wstring(), str)) ;
        CHECK(str.empty()) ;
    }

    SUBCASE("Arguments Test") {
        std::wstring wstr(L"Hello") ;

        std::string expect("Hello") ;
        WideCharToMultiByte_dst = expect ;

        WideCharToMultiByte_fake.custom_fake = WideCharToMultiByte_custom_fake ;

        std::string str ;
        CHECK(util::wstring2string(wstr, str)) ;
        CHECK_EQ(str, expect) ;

        CHECK_EQ(wcsncmp(WideCharToMultiByte_fake.arg2_val, wstr.c_str(), wstr.length()), 0) ;
        CHECK_EQ(WideCharToMultiByte_fake.arg3_val, wstr.size()) ;
    }

    SUBCASE("The output size is invalid") {
        WideCharToMultiByte_fake.return_val = -1 ; // invalid output size
        std::string str ;
        CHECK_FALSE(util::wstring2string(L"Hello", str)) ;
    }

    SUBCASE("Conversion is failed") {
        // Size is good, but conversion is failed
        int returns[2] = {10, -1} ;
        SET_RETURN_SEQ(WideCharToMultiByte, returns, 2) ;
        std::string str ;
        CHECK_FALSE(util::wstring2string(L"Hello", str)) ;
    }
}
