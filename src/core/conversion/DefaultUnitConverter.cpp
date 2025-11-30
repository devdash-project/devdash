#include "DefaultUnitConverter.h"
#include <cmath>

namespace devdash {

/**
 * @brief Physical conversion constants.
 *
 * These constants define the mathematical relationships between different
 * units of measurement. All values are based on standard international
 * unit definitions (SI, imperial, US customary).
 */
namespace {
// Temperature conversion constants
constexpr double KELVIN_TO_CELSIUS_OFFSET = 273.15;  // K to °C offset
constexpr double FAHRENHEIT_OFFSET = 32.0;           // Freezing point of water in °F
constexpr double CELSIUS_TO_FAHRENHEIT_RATIO = 9.0 / 5.0;  // °C to °F multiplier
constexpr double FAHRENHEIT_TO_CELSIUS_RATIO = 5.0 / 9.0;  // °F to °C multiplier

// Pressure conversion factors
constexpr double KPA_TO_PSI = 0.145038;    // Kilopascals to pounds per square inch
constexpr double KPA_TO_BAR = 0.01;        // Kilopascals to bar
constexpr double KPA_TO_INHG = 0.2953;     // Kilopascals to inches of mercury
constexpr double PSI_TO_BAR = 0.0689476;   // PSI to bar
constexpr double PSI_TO_INHG = 2.03602;    // PSI to inches of mercury
constexpr double BAR_TO_INHG = 29.53;      // Bar to inches of mercury

// Speed conversion factors
constexpr double KMH_TO_MPH = 0.621371;    // Kilometers per hour to miles per hour
constexpr double KMH_TO_MS = 1.0 / 3.6;    // Kilometers per hour to meters per second
constexpr double MPH_TO_MS = 0.44704;      // Miles per hour to meters per second

// Distance conversion factors
constexpr double KM_TO_MI = 0.621371;      // Kilometers to miles
constexpr double KM_TO_M = 1000.0;         // Kilometers to meters
constexpr double KM_TO_FT = 3280.84;       // Kilometers to feet
constexpr double MI_TO_M = 1609.34;        // Miles to meters
constexpr double MI_TO_FT = 5280.0;        // Miles to feet
constexpr double M_TO_FT = 3.28084;        // Meters to feet
constexpr double M_TO_MM = 1000.0;         // Meters to millimeters
constexpr double M_TO_IN = 39.3701;        // Meters to inches
constexpr double FT_TO_IN = 12.0;          // Feet to inches
constexpr double MM_TO_IN = 1.0 / 25.4;    // Millimeters to inches
constexpr double IN_TO_MM = 25.4;          // Inches to millimeters

// Angle conversion factors
constexpr double RAD_TO_DEG = 180.0 / M_PI;  // Radians to degrees
constexpr double DEG_TO_RAD = M_PI / 180.0;  // Degrees to radians

// Volume conversion factors
constexpr double L_TO_GAL_US = 0.264172;   // Liters to US gallons
constexpr double L_TO_GAL_UK = 0.219969;   // Liters to UK (imperial) gallons
constexpr double L_TO_ML = 1000.0;         // Liters to milliliters
} // namespace

/**
 * @brief Initialize the converter by registering all conversion functions.
 *
 * Populates the conversion lookup table with bidirectional conversion
 * functions for all supported unit types. This is done once at construction
 * time for O(1) lookup during runtime conversions.
 */
DefaultUnitConverter::DefaultUnitConverter() {
    registerTemperatureConversions();
    registerPressureConversions();
    registerSpeedConversions();
    registerDistanceConversions();
    registerAngleConversions();
    registerVolumeConversions();
}

/**
 * @brief Convert a value between two units.
 *
 * Looks up the conversion function in the pre-built table and applies it.
 * If no conversion is registered (either unsupported or same unit), returns
 * the original value unchanged.
 *
 * @param value The numeric value to convert
 * @param fromUnit Source unit (case-sensitive, e.g., "K", "kPa")
 * @param toUnit Target unit (case-sensitive, e.g., "C", "psi")
 * @return Converted value, or original value if conversion not available
 */
double DefaultUnitConverter::convert(double value, const QString& fromUnit,
                                     const QString& toUnit) const {
    // Same unit - no conversion needed
    if (fromUnit == toUnit) {
        return value;
    }

    ConversionKey key{fromUnit, toUnit};
    auto it = m_conversions.find(key);

    if (it != m_conversions.end()) {
        return it.value()(value);
    }

    // Conversion not found - return original value
    return value;
}

/**
 * @brief Check if a conversion is supported.
 *
 * @param fromUnit Source unit
 * @param toUnit Target unit
 * @return true if conversion is registered, false otherwise
 */
bool DefaultUnitConverter::canConvert(const QString& fromUnit,
                                      const QString& toUnit) const {
    if (fromUnit == toUnit) {
        return true;
    }

    ConversionKey key{fromUnit, toUnit};
    return m_conversions.contains(key);
}

/**
 * @brief Register bidirectional temperature conversion functions.
 *
 * Registers conversions between Kelvin (K), Celsius (C), and Fahrenheit (F).
 * Uses standard formulas:
 * - K to C: subtract 273.15
 * - C to F: multiply by 9/5 and add 32
 * - F to C: subtract 32 and multiply by 5/9
 */
void DefaultUnitConverter::registerTemperatureConversions() {
    // Kelvin conversions
    m_conversions[{"K", "C"}] = [](double k) { return k - KELVIN_TO_CELSIUS_OFFSET; };
    m_conversions[{"K", "F"}] = [](double k) {
        return (k - KELVIN_TO_CELSIUS_OFFSET) * CELSIUS_TO_FAHRENHEIT_RATIO + FAHRENHEIT_OFFSET;
    };

    // Celsius conversions
    m_conversions[{"C", "K"}] = [](double c) { return c + KELVIN_TO_CELSIUS_OFFSET; };
    m_conversions[{"C", "F"}] = [](double c) {
        return c * CELSIUS_TO_FAHRENHEIT_RATIO + FAHRENHEIT_OFFSET;
    };

    // Fahrenheit conversions
    m_conversions[{"F", "C"}] = [](double f) {
        return (f - FAHRENHEIT_OFFSET) * FAHRENHEIT_TO_CELSIUS_RATIO;
    };
    m_conversions[{"F", "K"}] = [](double f) {
        return (f - FAHRENHEIT_OFFSET) * FAHRENHEIT_TO_CELSIUS_RATIO + KELVIN_TO_CELSIUS_OFFSET;
    };
}

/**
 * @brief Register bidirectional pressure conversion functions.
 *
 * Registers conversions between kilopascals (kPa), pounds per square inch (psi),
 * bar, and inches of mercury (inHg). Common for oil pressure, fuel pressure,
 * manifold pressure, and atmospheric pressure readings.
 */
void DefaultUnitConverter::registerPressureConversions() {
    // kPa conversions
    m_conversions[{"kPa", "psi"}] = [](double kpa) { return kpa * KPA_TO_PSI; };
    m_conversions[{"kPa", "bar"}] = [](double kpa) { return kpa * KPA_TO_BAR; };
    m_conversions[{"kPa", "inHg"}] = [](double kpa) { return kpa * KPA_TO_INHG; };

    // psi conversions
    m_conversions[{"psi", "kPa"}] = [](double psi) { return psi / KPA_TO_PSI; };
    m_conversions[{"psi", "bar"}] = [](double psi) { return psi * PSI_TO_BAR; };
    m_conversions[{"psi", "inHg"}] = [](double psi) { return psi * PSI_TO_INHG; };

    // bar conversions
    m_conversions[{"bar", "kPa"}] = [](double bar) { return bar / KPA_TO_BAR; };
    m_conversions[{"bar", "psi"}] = [](double bar) { return bar / PSI_TO_BAR; };
    m_conversions[{"bar", "inHg"}] = [](double bar) { return bar * BAR_TO_INHG; };

    // inHg conversions
    m_conversions[{"inHg", "kPa"}] = [](double inhg) { return inhg / KPA_TO_INHG; };
    m_conversions[{"inHg", "psi"}] = [](double inhg) { return inhg / PSI_TO_INHG; };
    m_conversions[{"inHg", "bar"}] = [](double inhg) { return inhg / BAR_TO_INHG; };
}

/**
 * @brief Register bidirectional speed conversion functions.
 *
 * Registers conversions between kilometers per hour (km/h), miles per hour (mph),
 * and meters per second (m/s). Used for vehicle speed, wheel speed, and other
 * velocity measurements.
 */
void DefaultUnitConverter::registerSpeedConversions() {
    // km/h conversions
    m_conversions[{"km/h", "mph"}] = [](double kmh) { return kmh * KMH_TO_MPH; };
    m_conversions[{"km/h", "m/s"}] = [](double kmh) { return kmh * KMH_TO_MS; };

    // mph conversions
    m_conversions[{"mph", "km/h"}] = [](double mph) { return mph / KMH_TO_MPH; };
    m_conversions[{"mph", "m/s"}] = [](double mph) { return mph * MPH_TO_MS; };

    // m/s conversions
    m_conversions[{"m/s", "km/h"}] = [](double ms) { return ms / KMH_TO_MS; };
    m_conversions[{"m/s", "mph"}] = [](double ms) { return ms / MPH_TO_MS; };
}

/**
 * @brief Register bidirectional distance conversion functions.
 *
 * Registers conversions between kilometers (km), miles (mi), meters (m),
 * feet (ft), millimeters (mm), and inches (in). Used for trip distance,
 * displacement, and dimensional measurements.
 */
void DefaultUnitConverter::registerDistanceConversions() {
    // km conversions
    m_conversions[{"km", "mi"}] = [](double km) { return km * KM_TO_MI; };
    m_conversions[{"km", "m"}] = [](double km) { return km * KM_TO_M; };
    m_conversions[{"km", "ft"}] = [](double km) { return km * KM_TO_FT; };

    // mi conversions
    m_conversions[{"mi", "km"}] = [](double mi) { return mi / KM_TO_MI; };
    m_conversions[{"mi", "m"}] = [](double mi) { return mi * MI_TO_M; };
    m_conversions[{"mi", "ft"}] = [](double mi) { return mi * MI_TO_FT; };

    // m conversions
    m_conversions[{"m", "km"}] = [](double m) { return m / KM_TO_M; };
    m_conversions[{"m", "mi"}] = [](double m) { return m / MI_TO_M; };
    m_conversions[{"m", "ft"}] = [](double m) { return m * M_TO_FT; };
    m_conversions[{"m", "mm"}] = [](double m) { return m * M_TO_MM; };
    m_conversions[{"m", "in"}] = [](double m) { return m * M_TO_IN; };

    // ft conversions
    m_conversions[{"ft", "km"}] = [](double ft) { return ft / KM_TO_FT; };
    m_conversions[{"ft", "mi"}] = [](double ft) { return ft / MI_TO_FT; };
    m_conversions[{"ft", "m"}] = [](double ft) { return ft / M_TO_FT; };
    m_conversions[{"ft", "in"}] = [](double ft) { return ft * FT_TO_IN; };

    // mm conversions
    m_conversions[{"mm", "m"}] = [](double mm) { return mm / M_TO_MM; };
    m_conversions[{"mm", "in"}] = [](double mm) { return mm * MM_TO_IN; };

    // in conversions
    m_conversions[{"in", "m"}] = [](double in) { return in / M_TO_IN; };
    m_conversions[{"in", "ft"}] = [](double in) { return in / FT_TO_IN; };
    m_conversions[{"in", "mm"}] = [](double in) { return in * IN_TO_MM; };
}

/**
 * @brief Register bidirectional angle conversion functions.
 *
 * Registers conversions between radians (rad) and degrees (deg).
 * Used for ignition timing, camshaft angle, steering angle, and other
 * angular measurements.
 */
void DefaultUnitConverter::registerAngleConversions() {
    // rad conversions
    m_conversions[{"rad", "deg"}] = [](double rad) { return rad * RAD_TO_DEG; };

    // deg conversions
    m_conversions[{"deg", "rad"}] = [](double deg) { return deg * DEG_TO_RAD; };
}

/**
 * @brief Register bidirectional volume conversion functions.
 *
 * Registers conversions between liters (L), US gallons (gal_us),
 * UK/imperial gallons (gal_uk), and milliliters (ml). Used for fuel
 * consumption, tank capacity, and fluid volume measurements.
 *
 * @note "gal" defaults to US gallons (gal_us) for convenience.
 */
void DefaultUnitConverter::registerVolumeConversions() {
    // L conversions
    m_conversions[{"L", "gal"}] = [](double l) { return l * L_TO_GAL_US; }; // US gallon default
    m_conversions[{"L", "gal_us"}] = [](double l) { return l * L_TO_GAL_US; };
    m_conversions[{"L", "gal_uk"}] = [](double l) { return l * L_TO_GAL_UK; };
    m_conversions[{"L", "ml"}] = [](double l) { return l * L_TO_ML; };

    // gal (US) conversions
    m_conversions[{"gal", "L"}] = [](double gal) { return gal / L_TO_GAL_US; };
    m_conversions[{"gal_us", "L"}] = [](double gal) { return gal / L_TO_GAL_US; };

    // gal (UK) conversions
    m_conversions[{"gal_uk", "L"}] = [](double gal) { return gal / L_TO_GAL_UK; };

    // ml conversions
    m_conversions[{"ml", "L"}] = [](double ml) { return ml / L_TO_ML; };
}

} // namespace devdash
