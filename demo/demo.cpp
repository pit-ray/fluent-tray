#include "fluent_tray.hpp"

#include <iostream>


int main()
{
    using namespace fluent_tray ;
    FluentTray tray{} ;
    BYTE opacity = 240 ;
    if(!tray.create_tray(
            "demo", "demo/assets/icon.ico",
            5, 5, 10, 5, opacity, true)) {
        return 1 ;
    }

    if(!tray.add_menu(
            "Home", "demo/assets/fa-home.ico", false, "",
            [] {
                std::cout << "Home\n" ;
                return true ;
            })) {
        return 1 ;
    }

    tray.add_line() ;

    if(!tray.add_menu("Download", "demo/assets/fa-download.ico", false, "",
            [] {
                std::cout << "Download\n" ;
                return true ;
            })) {
        return 1 ;
    }

    if(!tray.add_menu(
            "Insight", "demo/assets/fa-line-chart.ico", false, "",
            [] {
                std::cout << "Insight\n" ;
                return true ;
            })) {
        return 1 ;
    }

    tray.add_line() ;

    if(!tray.add_menu("Coffee", "demo/assets/fa-coffee.ico", true, "✓",
            [] {
                std::cout << "I like coffee\n" ;
                return true ;
            },
            [] {
                std::cout << "I don't like coffe\n" ;
                return true ;
            })) {
        return 1 ;
    }
    if(!tray.add_menu("Desktop", "demo/assets/fa-desktop.ico", true, "✓",
            [] {
                std::cout << "Connect to Desktop\n" ;
                return true ;
            },
            [] {
                std::cout << "Disconnect from desktop\n" ;
                return true ;
            })) {
        return 1 ;
    }

    tray.add_line() ;

    if(!tray.add_menu(
            "Exit", "demo/assets/fa-sign-out.ico", false, "",
            [] {
                return false ;
            })) {
        return 1 ;
    }

    if(!tray.update_with_loop()) {
        return 1 ;
    }

    return 0 ;
}
