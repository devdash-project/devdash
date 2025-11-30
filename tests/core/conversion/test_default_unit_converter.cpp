// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
// Test file - numeric literals are test data and self-documenting

#include "core/conversion/DefaultUnitConverter.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace devdash;
using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

TEST_CASE("DefaultUnitConverter - Temperature conversions", "[unit-converter][temperature]") {
    DefaultUnitConverter converter;

    SECTION("Kelvin to Celsius") {
        REQUIRE_THAT(converter.convert(273.15, "K", "C"), WithinAbs(0.0, 0.01));
        REQUIRE_THAT(converter.convert(373.15, "K", "C"), WithinAbs(100.0, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "K", "C"), WithinAbs(-273.15, 0.01));
    }

    SECTION("Celsius to Kelvin") {
        REQUIRE_THAT(converter.convert(0.0, "C", "K"), WithinAbs(273.15, 0.01));
        REQUIRE_THAT(converter.convert(100.0, "C", "K"), WithinAbs(373.15, 0.01));
        REQUIRE_THAT(converter.convert(-273.15, "C", "K"), WithinAbs(0.0, 0.01));
    }

    SECTION("Celsius to Fahrenheit") {
        REQUIRE_THAT(converter.convert(0.0, "C", "F"), WithinAbs(32.0, 0.01));
        REQUIRE_THAT(converter.convert(100.0, "C", "F"), WithinAbs(212.0, 0.01));
        REQUIRE_THAT(converter.convert(-40.0, "C", "F"), WithinAbs(-40.0, 0.01));
    }

    SECTION("Fahrenheit to Celsius") {
        REQUIRE_THAT(converter.convert(32.0, "F", "C"), WithinAbs(0.0, 0.01));
        REQUIRE_THAT(converter.convert(212.0, "F", "C"), WithinAbs(100.0, 0.01));
        REQUIRE_THAT(converter.convert(-40.0, "F", "C"), WithinAbs(-40.0, 0.01));
    }

    SECTION("Kelvin to Fahrenheit") {
        REQUIRE_THAT(converter.convert(273.15, "K", "F"), WithinAbs(32.0, 0.01));
        REQUIRE_THAT(converter.convert(373.15, "K", "F"), WithinAbs(212.0, 0.01));
    }

    SECTION("Fahrenheit to Kelvin") {
        REQUIRE_THAT(converter.convert(32.0, "F", "K"), WithinAbs(273.15, 0.01));
        REQUIRE_THAT(converter.convert(212.0, "F", "K"), WithinAbs(373.15, 0.01));
    }
}

TEST_CASE("DefaultUnitConverter - Pressure conversions", "[unit-converter][pressure]") {
    DefaultUnitConverter converter;

    SECTION("kPa to psi") {
        REQUIRE_THAT(converter.convert(100.0, "kPa", "psi"), WithinRel(14.5038, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "kPa", "psi"), WithinAbs(0.0, 0.01));
    }

    SECTION("psi to kPa") {
        REQUIRE_THAT(converter.convert(14.5038, "psi", "kPa"), WithinRel(100.0, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "psi", "kPa"), WithinAbs(0.0, 0.01));
    }

    SECTION("kPa to bar") {
        REQUIRE_THAT(converter.convert(100.0, "kPa", "bar"), WithinAbs(1.0, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "kPa", "bar"), WithinAbs(0.0, 0.01));
    }

    SECTION("bar to kPa") {
        REQUIRE_THAT(converter.convert(1.0, "bar", "kPa"), WithinAbs(100.0, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "bar", "kPa"), WithinAbs(0.0, 0.01));
    }

    SECTION("kPa to inHg") {
        REQUIRE_THAT(converter.convert(101.325, "kPa", "inHg"), WithinRel(29.92, 0.01));
    }

    SECTION("psi to bar") {
        REQUIRE_THAT(converter.convert(14.5038, "psi", "bar"), WithinRel(1.0, 0.01));
    }

    SECTION("bar to psi") {
        REQUIRE_THAT(converter.convert(1.0, "bar", "psi"), WithinRel(14.5038, 0.01));
    }
}

TEST_CASE("DefaultUnitConverter - Speed conversions", "[unit-converter][speed]") {
    DefaultUnitConverter converter;

    SECTION("km/h to mph") {
        REQUIRE_THAT(converter.convert(100.0, "km/h", "mph"), WithinRel(62.1371, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "km/h", "mph"), WithinAbs(0.0, 0.01));
    }

    SECTION("mph to km/h") {
        REQUIRE_THAT(converter.convert(62.1371, "mph", "km/h"), WithinRel(100.0, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "mph", "km/h"), WithinAbs(0.0, 0.01));
    }

    SECTION("km/h to m/s") {
        REQUIRE_THAT(converter.convert(36.0, "km/h", "m/s"), WithinAbs(10.0, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "km/h", "m/s"), WithinAbs(0.0, 0.01));
    }

    SECTION("m/s to km/h") {
        REQUIRE_THAT(converter.convert(10.0, "m/s", "km/h"), WithinAbs(36.0, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "m/s", "km/h"), WithinAbs(0.0, 0.01));
    }

    SECTION("mph to m/s") {
        REQUIRE_THAT(converter.convert(22.3694, "mph", "m/s"), WithinAbs(10.0, 0.01));
    }

    SECTION("m/s to mph") {
        REQUIRE_THAT(converter.convert(10.0, "m/s", "mph"), WithinRel(22.3694, 0.01));
    }
}

TEST_CASE("DefaultUnitConverter - Distance conversions", "[unit-converter][distance]") {
    DefaultUnitConverter converter;

    SECTION("km to mi") {
        REQUIRE_THAT(converter.convert(10.0, "km", "mi"), WithinRel(6.21371, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "km", "mi"), WithinAbs(0.0, 0.01));
    }

    SECTION("mi to km") {
        REQUIRE_THAT(converter.convert(6.21371, "mi", "km"), WithinRel(10.0, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "mi", "km"), WithinAbs(0.0, 0.01));
    }

    SECTION("m to ft") {
        REQUIRE_THAT(converter.convert(10.0, "m", "ft"), WithinRel(32.8084, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "m", "ft"), WithinAbs(0.0, 0.01));
    }

    SECTION("ft to m") {
        REQUIRE_THAT(converter.convert(32.8084, "ft", "m"), WithinRel(10.0, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "ft", "m"), WithinAbs(0.0, 0.01));
    }

    SECTION("mm to in") {
        REQUIRE_THAT(converter.convert(25.4, "mm", "in"), WithinAbs(1.0, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "mm", "in"), WithinAbs(0.0, 0.01));
    }

    SECTION("in to mm") {
        REQUIRE_THAT(converter.convert(1.0, "in", "mm"), WithinAbs(25.4, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "in", "mm"), WithinAbs(0.0, 0.01));
    }

    SECTION("km to m") {
        REQUIRE_THAT(converter.convert(1.0, "km", "m"), WithinAbs(1000.0, 0.01));
    }

    SECTION("m to km") {
        REQUIRE_THAT(converter.convert(1000.0, "m", "km"), WithinAbs(1.0, 0.01));
    }
}

TEST_CASE("DefaultUnitConverter - Angle conversions", "[unit-converter][angle]") {
    DefaultUnitConverter converter;

    SECTION("rad to deg") {
        REQUIRE_THAT(converter.convert(3.14159, "rad", "deg"), WithinRel(180.0, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "rad", "deg"), WithinAbs(0.0, 0.01));
        REQUIRE_THAT(converter.convert(1.5708, "rad", "deg"), WithinRel(90.0, 0.01));
    }

    SECTION("deg to rad") {
        REQUIRE_THAT(converter.convert(180.0, "deg", "rad"), WithinRel(3.14159, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "deg", "rad"), WithinAbs(0.0, 0.01));
        REQUIRE_THAT(converter.convert(90.0, "deg", "rad"), WithinRel(1.5708, 0.01));
    }
}

TEST_CASE("DefaultUnitConverter - Volume conversions", "[unit-converter][volume]") {
    DefaultUnitConverter converter;

    SECTION("L to gal (US)") {
        REQUIRE_THAT(converter.convert(10.0, "L", "gal"), WithinRel(2.64172, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "L", "gal"), WithinAbs(0.0, 0.01));
    }

    SECTION("gal (US) to L") {
        REQUIRE_THAT(converter.convert(2.64172, "gal", "L"), WithinRel(10.0, 0.001));
        REQUIRE_THAT(converter.convert(0.0, "gal", "L"), WithinAbs(0.0, 0.01));
    }

    SECTION("L to gal_us") {
        REQUIRE_THAT(converter.convert(10.0, "L", "gal_us"), WithinRel(2.64172, 0.001));
    }

    SECTION("L to gal_uk") {
        REQUIRE_THAT(converter.convert(10.0, "L", "gal_uk"), WithinRel(2.19969, 0.001));
    }

    SECTION("gal_uk to L") {
        REQUIRE_THAT(converter.convert(2.19969, "gal_uk", "L"), WithinRel(10.0, 0.001));
    }

    SECTION("L to ml") {
        REQUIRE_THAT(converter.convert(1.0, "L", "ml"), WithinAbs(1000.0, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "L", "ml"), WithinAbs(0.0, 0.01));
    }

    SECTION("ml to L") {
        REQUIRE_THAT(converter.convert(1000.0, "ml", "L"), WithinAbs(1.0, 0.01));
        REQUIRE_THAT(converter.convert(0.0, "ml", "L"), WithinAbs(0.0, 0.01));
    }
}

TEST_CASE("DefaultUnitConverter - Same unit conversions", "[unit-converter][identity]") {
    DefaultUnitConverter converter;

    SECTION("Same unit returns original value") {
        REQUIRE(converter.convert(100.0, "C", "C") == 100.0);
        REQUIRE(converter.convert(50.0, "psi", "psi") == 50.0);
        REQUIRE(converter.convert(120.0, "km/h", "km/h") == 120.0);
        REQUIRE(converter.convert(10.0, "m", "m") == 10.0);
        REQUIRE(converter.convert(45.0, "deg", "deg") == 45.0);
        REQUIRE(converter.convert(5.0, "L", "L") == 5.0);
    }
}

TEST_CASE("DefaultUnitConverter - Invalid conversions", "[unit-converter][error]") {
    const DefaultUnitConverter converter;

    SECTION("Unknown source unit returns original value") {
        REQUIRE(converter.convert(100.0, "unknown", "C") == 100.0);
    }

    SECTION("Unknown target unit returns original value") {
        REQUIRE(converter.convert(100.0, "C", "unknown") == 100.0);
    }

    SECTION("Incompatible units returns original value") {
        // These conversions aren't registered, so should return original value
        REQUIRE(converter.convert(100.0, "C", "psi") == 100.0);
        REQUIRE(converter.convert(50.0, "km/h", "L") == 50.0);
    }
}

TEST_CASE("DefaultUnitConverter - canConvert method", "[unit-converter][capability]") {
    const DefaultUnitConverter converter;

    SECTION("Valid conversions return true") {
        REQUIRE(converter.canConvert("C", "F"));
        REQUIRE(converter.canConvert("F", "C"));
        REQUIRE(converter.canConvert("K", "C"));
        REQUIRE(converter.canConvert("kPa", "psi"));
        REQUIRE(converter.canConvert("km/h", "mph"));
        REQUIRE(converter.canConvert("rad", "deg"));
        REQUIRE(converter.canConvert("L", "gal"));
    }

    SECTION("Same unit returns true") {
        REQUIRE(converter.canConvert("C", "C"));
        REQUIRE(converter.canConvert("psi", "psi"));
        REQUIRE(converter.canConvert("km/h", "km/h"));
    }

    SECTION("Invalid conversions return false") {
        REQUIRE_FALSE(converter.canConvert("C", "psi"));
        REQUIRE_FALSE(converter.canConvert("unknown", "C"));
        REQUIRE_FALSE(converter.canConvert("C", "unknown"));
        REQUIRE_FALSE(converter.canConvert("km/h", "L"));
    }
}

TEST_CASE("DefaultUnitConverter - Edge cases", "[unit-converter][edge-cases]") {
    const DefaultUnitConverter converter;

    SECTION("Negative values") {
        // Temperature can be negative
        REQUIRE_THAT(converter.convert(-40.0, "C", "F"), WithinAbs(-40.0, 0.01));
        REQUIRE_THAT(converter.convert(-273.15, "C", "K"), WithinAbs(0.0, 0.01));
    }

    SECTION("Zero values") {
        REQUIRE(converter.convert(0.0, "C", "K") == 273.15);
        REQUIRE(converter.convert(0.0, "kPa", "psi") == 0.0);
        REQUIRE(converter.convert(0.0, "km/h", "mph") == 0.0);
    }

    SECTION("Large values") {
        REQUIRE_THAT(converter.convert(10000.0, "km", "mi"), WithinRel(6213.71, 0.01));
        REQUIRE_THAT(converter.convert(1000.0, "bar", "kPa"), WithinAbs(100000.0, 1.0));
    }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
