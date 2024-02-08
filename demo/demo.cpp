#include "fluent_tray.hpp"

#include <iostream>


int main()
{
    using namespace fluent_tray ;
    FluentTray tray("demo", "demo/sample_icon.ico", 20, 500, "Consolas") ;
    if(!tray.create_tray()) {
        std::cout << "Failed Tray Initialization\n" ;
        std::cout << GetLastError() << std::endl ;
        return 1 ;
    }

    if(!tray.add_menu(
            "TextTest 1", "demo/sample_icon.ico", false,
            []{std::cout << "clicked 1!\n" ; return true ;})) {
        std::cout << "Failed Menu 1\n" ;
        return 1 ;
    }
    if(!tray.add_menu("日本語テスト", "demo/sample_icon.ico")) {
        std::cout << "Failed Menu 2\n" ;
        return 1 ;
    }
    if(!tray.add_menu("Button1", "demo/sample_icon.ico")) {
        std::cout << "Failed Menu 3\n" ;
        return 1 ;
    }
    if(!tray.add_menu("Button2", "demo/sample_icon.ico")) {
        std::cout << "Failed Menu 4\n" ;
        return 1 ;
    }
    if(!tray.add_menu("Togglable1", "demo/sample_icon.ico", true)) {
        std::cout << "Failed Menu 4\n" ;
        return 1 ;
    }
    if(!tray.add_menu("Togglable2", "demo/sample_icon.ico", true)) {
        std::cout << "Failed Menu 4\n" ;
        return 1 ;
    }
    if(!tray.add_menu("Exit", "demo/sample_icon.ico", false, []{return false ;})) {
        std::cout << "Failed Menu 4\n" ;
        return 1 ;
    }

    tray.update_parallel() ;

    return 0 ;
}
