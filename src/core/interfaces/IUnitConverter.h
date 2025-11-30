#pragma once

#include <QString>

namespace devdash {

/**
 * @brief Interface for converting values between units
 *
 * Data sources report values in their native units (e.g., Haltech reports
 * temperature in Kelvin). The UnitConverter handles conversion to user's
 * preferred units (e.g., Fahrenheit).
 *
 * Adapters can provide custom converters for proprietary units, or use
 * the DefaultUnitConverter for common conversions.
 */
class IUnitConverter {
  public:
    /**
     * @brief Virtual destructor for proper polymorphic cleanup.
     */
    virtual ~IUnitConverter() = default;

    /**
     * @brief Convert a value from one unit to another
     * @param value The raw value to convert
     * @param fromUnit Source unit (e.g., "K", "kPa", "rad")
     * @param toUnit Target unit (e.g., "F", "psi", "deg")
     * @return Converted value
     *
     * @note If conversion is not supported, returns the original value unchanged.
     *       Use canConvert() to check if a conversion is supported.
     *
     * @example
     * @code
     * double fahrenheit = converter->convert(373.15, "K", "F");
     * // Returns 211.73
     * @endcode
     */
    [[nodiscard]] virtual double convert(double value, const QString& fromUnit,
                                         const QString& toUnit) const = 0;

    /**
     * @brief Check if this converter can handle a conversion
     * @param fromUnit Source unit
     * @param toUnit Target unit
     * @return true if conversion is supported
     */
    [[nodiscard]] virtual bool canConvert(const QString& fromUnit,
                                          const QString& toUnit) const = 0;
};

} // namespace devdash
