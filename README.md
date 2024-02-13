<p align="center"><h1>fluent-tray</h1></p>
<p align="center"><img src="https://github.com/pit-ray/fluent-tray/blob/main/assets/banner.png?raw=true" width=128/></p>
<p align="center"><b>Fluent Design-based GUI Library for System Tray Applications</b></p>
<p align="center"><b>:fire:Warning: This project is still under development. Do not use it in a production environment.:fire:</b></p>

<p align="center">
    <a href="https://coveralls.io/github/pit-ray/fluent-tray"><img alt="Coveralls" src="https://img.shields.io/coverallsCoverage/github/pit-ray/fluent-tray?style=flat-square"></a>
    <a href="https://github.com/pit-ray/fluent-tray/actions/workflows/test.yml"><img src="https://img.shields.io/github/actions/workflow/status/pit-ray/fluent-tray/test.yml?branch=main&label=test&logo=github&style=flat-square"/></a>
    <br>  
    <a href="https://scan.coverity.com/projects/pit-ray-fluent-tray"><img alt="Coverity Scan Build Status" src="https://img.shields.io/coverity/scan/pit-ray-fluent-tray?style=flat-square" /></a>
    <a href="https://www.codacy.com/gh/pit-ray/fluent-tray/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=pit-ray/fluent-tray&amp;utm_campaign=Badge_Grade"><img src="https://img.shields.io/codacy/grade/8f2e6f2826904efd82019f5888574327?style=flat-square" /></a>
    <a href="https://github.com/pit-ray/fluent-tray/actions/workflows/codeql.yml"><img src="https://img.shields.io/github/actions/workflow/status/pit-ray/fluent-tray/codeql.yml?branch=main&label=CodeQL&logo=github&style=flat-square"/></a>
    <br>  
    <a href="https://github.com/pit-ray/fluent-tray/actions/workflows/coverity.yml"><img src="https://img.shields.io/github/actions/workflow/status/pit-ray/fluent-tray/coverity.yml?branch=main&label=Coverity Build&logo=github&style=flat-square"/></a>
    <a href="https://github.com/pit-ray/fluent-tray/actions/workflows/coveralls.yml"><img src="https://img.shields.io/github/actions/workflow/status/pit-ray/fluent-tray/coveralls.yml?branch=main&label=Coveralls Build&logo=github&style=flat-square"/></a>
    <br>  
    <a href="https://github.com/pit-ray/fluent-tray/actions/workflows/windows.yml"><img src="https://img.shields.io/github/actions/workflow/status/pit-ray/fluent-tray/windows.yml?branch=main&label=MSVC Build&logo=github&style=flat-square"/></a>
    <a href="https://github.com/pit-ray/fluent-tray/actions/workflows/mingw.yml"><img src="https://img.shields.io/github/actions/workflow/status/pit-ray/fluent-tray/mingw.yml?branch=main&label=MinGW Build&logo=github&style=flat-square"/></a>
</p>

## Concept
fluent-tray provides a simple system tray icon and menu to easily create resident applications that do not require complex windows.  
All you have to do is include a single header file since only the native API is used.  
Currently, only Windows is supported.

## Demo

<img src="https://github.com/pit-ray/fluent-tray/blob/main/assets/demo.png?raw=true" />

#### Code
Simply create a `FluentTray` object and add a menu with `.add_menu()`.

```cpp
#include "fluent_tray.hpp"

int main()
{
    using namespace fluent_tray ;

    FluentTray tray{} ;

    // Initialize the tray icon.
    tray.create_tray("demo", "demo/assets/icon.ico") ;

    // Add menus in order from the top.
    tray.add_menu("Home", "demo/assets/fa-home.ico") ;
    tray.add_separator() ;

    tray.add_menu("Download", "demo/assets/fa-download.ico") ;
    tray.add_menu("Insight", "demo/assets/fa-line-chart.ico") ;
    tray.add_separator() ;

    tray.add_menu("Coffee", "demo/assets/fa-coffee.ico", true) ;
    tray.add_menu("Desktop", "demo/assets/fa-desktop.ico", true) ;
    tray.add_separator() ;

    tray.add_menu("Exit", "demo/assets/fa-sign-out.ico") ;

    // Start message loop
    tray.update_with_loop() ;

    return 0 ;
}
```

#### Build
You can build this demo using `cmake` as follows.

```sh
$ cmake -B build demo
$ cmake --build build
$ ./build/Debug/fluent-tray-demo.exe
```

## Test

```sh
$ cmake -B build_test tests
$ cmake --build build_test
$ ctest -C Debug --test-dir build_test --output-on-failure
```

## License
This library is provided by pit-ray under the [MIT License](./LICENSE.txt).
