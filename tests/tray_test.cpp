#include "doctest.h"

#include "fluent_tray.hpp"

#include <string.h>

using namespace fluent_tray ;

TEST_CASE("FluentTray Test: ") {
    SUBCASE("Constructor") {
        CHECK_NOTHROW(FluentTray{}) ;

        FluentTray tray ;
        CHECK_EQ(tray.window_handle(), static_cast<HWND>(NULL)) ;
        CHECK_EQ(tray.status(), TrayStatus::STOPPED) ;
        CHECK_EQ(tray.count_menus(), 0) ;
    }

    SUBCASE("Error Check: add_menu") {
        FluentTray tray ;
        CHECK_FALSE(tray.create_tray("test_failed", "aa.ico")) ;
    }

    SUBCASE("add_menu") {
        FluentTray tray ;
        CHECK(tray.create_tray("test_add_menu", "")) ;

        CHECK_NE(tray.window_handle(), static_cast<HWND>(NULL)) ;
        CHECK_EQ(tray.status(), TrayStatus::RUNNING) ;

        CHECK(tray.add_menu("menu1", "", false, "O")) ;

        bool menu2_flag = false ;
        CHECK(tray.add_menu(
            "menu2", "", false, "O",
            [&menu2_flag] {menu2_flag = true ; return true ;})) ;

        bool menu3_flag = false ;
        CHECK(tray.add_menu(
            "menu3", "", true, "O",
            [&menu3_flag] {menu3_flag = true ; return true ;},
            [&menu3_flag] {menu3_flag = false ; return true ;})) ;

        std::string str1 ;
        CHECK(tray.begin()->get_label(str1)) ;
        CHECK_EQ(str1, "menu1") ;
        CHECK_FALSE(tray.begin()->is_toggleable()) ;
        CHECK(tray.begin()->process_click_event()) ;

        std::string str2 ;
        CHECK((tray.begin() + 1)->get_label(str2)) ;
        CHECK_EQ(str2, "menu2") ;
        CHECK_FALSE((tray.begin() + 1)->is_toggleable()) ;
        CHECK((tray.begin() + 1)->process_click_event()) ;
        CHECK(menu2_flag) ;
        CHECK((tray.begin() + 1)->process_click_event()) ;
        CHECK(menu2_flag) ;

        std::string str3 ;
        CHECK((tray.begin() + 2)->get_label(str3)) ;
        CHECK_EQ(str3, "menu3") ;
        CHECK((tray.begin() + 2)->is_toggleable()) ;
        CHECK((tray.begin() + 2)->process_click_event()) ;
        CHECK(menu3_flag) ;
        CHECK((tray.begin() + 2)->is_checked()) ;
        CHECK((tray.begin() + 2)->process_click_event()) ;
        CHECK_FALSE(menu3_flag) ;
        CHECK_FALSE((tray.begin() + 2)->is_checked()) ;

        (tray.begin() + 2)->check() ;
        CHECK((tray.begin() + 2)->is_checked()) ;

        (tray.begin() + 2)->uncheck() ;
        CHECK_FALSE((tray.begin() + 2)->is_checked()) ;

        (tray.begin() + 2)->check() ;
        CHECK((tray.begin() + 2)->is_checked()) ;
    }

    SUBCASE("status") {
        FluentTray tray ;
        CHECK(tray.create_tray("test_status", "")) ;
        CHECK_EQ(tray.status(), TrayStatus::RUNNING) ;
        tray.stop() ;
        CHECK_EQ(tray.status(), TrayStatus::SHOULD_STOP) ;
        CHECK(tray.update_with_loop()) ;
        CHECK_EQ(tray.status(), TrayStatus::STOPPED) ;
    }

    SUBCASE("iterator") {
        FluentTray tray ;
        CHECK(tray.create_tray("test_iterator", "")) ;

        CHECK(tray.add_menu("menu1")) ;
        CHECK(tray.add_menu("menu2")) ;
        CHECK(tray.add_menu("menu3")) ;

        std::string label11 ;
        CHECK(tray.front().get_label(label11)) ;
        CHECK_EQ(label11, "menu1") ;

        std::string label12 ;
        CHECK(tray.begin()->get_label(label12)) ;
        CHECK_EQ(label12, "menu1") ;

        std::string label13 ;
        CHECK(tray.cbegin()->get_label(label13)) ;
        CHECK_EQ(label13, "menu1") ;

        std::string label21 ;
        CHECK((tray.begin() + 1)->get_label(label21)) ;
        CHECK_EQ(label21, "menu2") ;

        std::string label22 ;
        CHECK((tray.cbegin() + 1)->get_label(label22)) ;
        CHECK_EQ(label22, "menu2") ;

        std::string label23 ;
        CHECK((tray.cend() - 2)->get_label(label23)) ;
        CHECK_EQ(label23, "menu2") ;

        std::string label31 ;
        CHECK(tray.back().get_label(label31)) ;
        CHECK_EQ(label31, "menu3") ;

        std::string label32 ;
        CHECK((tray.cbegin() + 2)->get_label(label32)) ;
        CHECK_EQ(label32, "menu3") ;

        std::string label33 ;
        CHECK((tray.cend() - 1)->get_label(label33)) ;
        CHECK_EQ(label33, "menu3") ;

        std::string label34 ;
        CHECK((tray.end() - 1)->get_label(label34)) ;
        CHECK_EQ(label34, "menu3") ;
    }
}
