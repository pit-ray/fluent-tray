#include "test.hpp"

using namespace fluent_tray ;

TEST_CASE("Color test: ") {
    auto color = RGB(124, 214, 122) ;
    unsigned char expected_color = 188 ;
    CHECK_EQ(util::rgb2gray(color), expected_color) ;
}
