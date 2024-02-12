#include "doctest.h"

#include "fluent_tray.hpp"

using namespace fluent_tray ;

TEST_CASE("FluentMenu Test: ") {
    SUBCASE("Constructor") {
        CHECK_NOTHROW(FluentMenu{}) ;

        FluentMenu menu ;
        CHECK_EQ(menu.window_handle(), static_cast<HWND>(NULL)) ;
        CHECK_EQ(menu.menu_handle(), static_cast<HMENU>(NULL)) ;
        CHECK_EQ(menu.id(), 0) ;

        std::string str{} ;
        CHECK(menu.get_label(str)) ;
        CHECK(str.empty()) ;
    }

    SUBCASE("Error Check: create_menu") {
        FluentMenu menu ;
        CHECK_FALSE(menu.create_menu(NULL, NULL, 1)) ;
    }

    SUBCASE("toggle") {
        FluentMenu menu1{true} ;
        CHECK(menu1.is_toggleable()) ;

        CHECK_FALSE(menu1.is_checked()) ;
        menu1.check() ;
        CHECK(menu1.is_checked()) ;
        menu1.uncheck() ;
        CHECK_FALSE(menu1.is_checked()) ;

        FluentMenu menu2{false} ;
        CHECK_FALSE(menu2.is_toggleable()) ;

        CHECK_FALSE(menu2.is_checked()) ;
        menu2.check() ;
        CHECK_FALSE(menu2.is_checked()) ;
        menu2.uncheck() ;
        CHECK_FALSE(menu2.is_checked()) ;
    }
}
