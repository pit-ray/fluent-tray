#include "test.hpp"

using namespace fluent_tray ;

TEST_CASE("string test under Fake Windows API: ") {
    SUBCASE("Empty String") {
        std::wstring wstr ;
        CHECK(util::string2wstring(std::string(), wstr)) ;
        CHECK(wstr.empty()) ;
    }

    SUBCASE("Arguments Test") {
        std::string str("Hello") ;
        std::wstring expect(L"Hello") ;

        std::wstring wstr ;
        CHECK(util::string2wstring(str, wstr)) ;
        CHECK_EQ(wstr, expect) ;
    }


    SUBCASE("Empty String") {
        std::string str ;
        CHECK(util::wstring2string(std::wstring(), str)) ;
        CHECK(str.empty()) ;
    }

    SUBCASE("Arguments Test") {
        std::wstring wstr(L"Hello") ;
        std::string expect("Hello") ;

        std::string str ;
        CHECK(util::wstring2string(wstr, str)) ;
        CHECK_EQ(str, expect) ;
    }
}
