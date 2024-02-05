#include "fluent_tray.hpp"

#include <iostream>


int main()
{
    using namespace fluent_tray ;
    FluentMenu menu1("aa", "demo/sample_icon.ico") ;

    FluentTray tray("demo", "demo/sample_icon.ico") ;
    std::cout << GetLastError() << std::endl ;

    tray.append_menu(menu1) ;

    tray.update() ;
    std::cout << "aa" << std::endl ;

    return 0 ;
}
