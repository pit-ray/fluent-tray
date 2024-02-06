#include "fluent_tray.hpp"

#include <iostream>


int main()
{
    using namespace fluent_tray ;
    FluentTray tray("demo", "demo/sample_icon.ico") ;

    tray.add_menu("TextTest 1", "demo/sample_icon.ico") ;
    tray.add_menu("日本語テスト", "demo/sample_icon.ico") ;

    tray.update_parallel() ;

    return 0 ;
}
