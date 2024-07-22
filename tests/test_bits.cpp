#include "test.hpp"

using namespace fluent_tray ;


TEST_CASE("Bits test: ") {
    SUBCASE("bit2mask") {
        CHECK_EQ(util::bit2mask(1),                                 0b1) ;
        CHECK_EQ(util::bit2mask(2),                                0b11) ;
        CHECK_EQ(util::bit2mask(3),                               0b111) ;
        CHECK_EQ(util::bit2mask(4),                              0b1111) ;
        CHECK_EQ(util::bit2mask(5),                             0b11111) ;
        CHECK_EQ(util::bit2mask(6),                            0b111111) ;
        CHECK_EQ(util::bit2mask(7),                           0b1111111) ;
        CHECK_EQ(util::bit2mask(8),                          0b11111111) ;
        CHECK_EQ(util::bit2mask(9),                         0b111111111) ;
        CHECK_EQ(util::bit2mask(10),                       0b1111111111) ;
        CHECK_EQ(util::bit2mask(11),                      0b11111111111) ;
        CHECK_EQ(util::bit2mask(12),                     0b111111111111) ;
        CHECK_EQ(util::bit2mask(13),                    0b1111111111111) ;
        CHECK_EQ(util::bit2mask(14),                   0b11111111111111) ;
        CHECK_EQ(util::bit2mask(15),                  0b111111111111111) ;
        CHECK_EQ(util::bit2mask(16),                 0b1111111111111111) ;
        CHECK_EQ(util::bit2mask(17),                0b11111111111111111) ;
        CHECK_EQ(util::bit2mask(18),               0b111111111111111111) ;
        CHECK_EQ(util::bit2mask(19),              0b1111111111111111111) ;
        CHECK_EQ(util::bit2mask(20),             0b11111111111111111111) ;
        CHECK_EQ(util::bit2mask(21),            0b111111111111111111111) ;
        CHECK_EQ(util::bit2mask(22),           0b1111111111111111111111) ;
        CHECK_EQ(util::bit2mask(23),          0b11111111111111111111111) ;
        CHECK_EQ(util::bit2mask(24),         0b111111111111111111111111) ;
        CHECK_EQ(util::bit2mask(25),        0b1111111111111111111111111) ;
        CHECK_EQ(util::bit2mask(26),       0b11111111111111111111111111) ;
        CHECK_EQ(util::bit2mask(27),      0b111111111111111111111111111) ;
        CHECK_EQ(util::bit2mask(28),     0b1111111111111111111111111111) ;
        CHECK_EQ(util::bit2mask(29),    0b11111111111111111111111111111) ;
        CHECK_EQ(util::bit2mask(30),   0b111111111111111111111111111111) ;
        CHECK_EQ(util::bit2mask(31),  0b1111111111111111111111111111111) ;
        CHECK_EQ(util::bit2mask(32), 0b11111111111111111111111111111111) ;
    }

    SUBCASE("type2bit") {
        CHECK_EQ(util::type2bit<std::uint32_t>(), 32) ;
    }

    SUBCASE("split_bits") {
        std::uint32_t value = 0x11f97892 ;
        std::uint16_t upper, lower ;

        util::split_bits(value, upper, lower) ;

        CHECK_EQ(upper, 0x11f9) ;
        CHECK_EQ(lower, 0x7892) ;
    }

    SUBCASE("concatenate_bits") {
        std::uint16_t upper = 0x112f ;
        std::uint16_t lower = 0x88fc ;
        std::uint32_t value ;

        util::concatenate_bits(upper, lower, value) ;

        CHECK_EQ(value, 0x112f88fc) ;
    }
}
