#pragma once

#include "core/interfaces/IUnitConverter.h"
#include <QHash>
#include <QString>
#include <functional>

namespace devdash {

/**
 * @brief Default unit converter for common unit types
 *
 * Handles conversions for:
 * - Temperature: K, C, F
 * - Pressure: kPa, psi, bar, inHg
 * - Speed: km/h, mph, m/s
 * - Distance: km, mi, m, ft, mm, in
 * - Angle: rad, deg
 * - Volume: L, gal (US), gal (UK), ml
 */
class DefaultUnitConverter : public IUnitConverter {
  public:
    DefaultUnitConverter();
    ~DefaultUnitConverter() override = default;

    [[nodiscard]] double convert(double value, const QString& fromUnit,
                                 const QString& toUnit) const override;

    [[nodiscard]] bool canConvert(const QString& fromUnit,
                                  const QString& toUnit) const override;

  private:
    using ConversionFunc = std::function<double(double)>;

    struct ConversionKey {
        QString from;
        QString to;

        bool operator==(const ConversionKey& other) const {
            return from == other.from && to == other.to;
        }
    };

    friend uint qHash(const ConversionKey& key, size_t seed);

    QHash<ConversionKey, ConversionFunc> m_conversions;

    /**
     * @brief Register all temperature conversion functions.
     *
     * Registers bidirectional conversions between Kelvin (K),
     * Celsius (C), and Fahrenheit (F).
     */
    void registerTemperatureConversions();

    /**
     * @brief Register all pressure conversion functions.
     *
     * Registers bidirectional conversions between kilopascals (kPa),
     * pounds per square inch (psi), bar, and inches of mercury (inHg).
     */
    void registerPressureConversions();

    /**
     * @brief Register all speed conversion functions.
     *
     * Registers bidirectional conversions between kilometers per hour (km/h),
     * miles per hour (mph), and meters per second (m/s).
     */
    void registerSpeedConversions();

    /**
     * @brief Register all distance conversion functions.
     *
     * Registers bidirectional conversions between kilometers (km), miles (mi),
     * meters (m), feet (ft), millimeters (mm), and inches (in).
     */
    void registerDistanceConversions();

    /**
     * @brief Register all angle conversion functions.
     *
     * Registers bidirectional conversions between radians (rad)
     * and degrees (deg).
     */
    void registerAngleConversions();

    /**
     * @brief Register all volume conversion functions.
     *
     * Registers bidirectional conversions between liters (L),
     * US gallons (gal_us), UK gallons (gal_uk), and milliliters (ml).
     */
    void registerVolumeConversions();
};

// Hash function for ConversionKey
inline uint qHash(const DefaultUnitConverter::ConversionKey& key, size_t seed = 0) {
    // Cast to uint to avoid precision warning - qHash returns uint anyway
    return static_cast<uint>(qHash(key.from, seed) ^ qHash(key.to, seed));
}

} // namespace devdash
