# Unit Types Reference

## Temperature

| From | To | Formula |
|------|-----|---------|
| K | C | C = K - 273.15 |
| K | F | F = (K - 273.15) × 9/5 + 32 |
| C | F | F = C × 9/5 + 32 |

**Units**: `K`, `C`, `F`

## Pressure

| From | To | Formula |
|------|-----|---------|
| kPa | psi | psi = kPa × 0.145038 |
| kPa | bar | bar = kPa × 0.01 |
| kPa | inHg | inHg = kPa × 0.2953 |

**Units**: `kPa`, `psi`, `bar`, `inHg`

## Speed

| From | To | Formula |
|------|-----|---------|
| km/h | mph | mph = km/h × 0.621371 |
| km/h | m/s | m/s = km/h / 3.6 |

**Units**: `km/h`, `mph`, `m/s`

## Distance

| From | To | Formula |
|------|-----|---------|
| km | mi | mi = km × 0.621371 |
| m | ft | ft = m × 3.28084 |

**Units**: `km`, `mi`, `m`, `ft`, `mm`, `in`

## Angle

| From | To | Formula |
|------|-----|---------|
| rad | deg | deg = rad × 180/π |

**Units**: `rad`, `deg`

## Volume

| From | To | Formula |
|------|-----|---------|
| L | gal (US) | gal = L × 0.264172 |
| L | gal (UK) | gal = L × 0.219969 |

**Units**: `L`, `gal`, `gal_us`, `gal_uk`, `ml`

## Implementation Note

The `DefaultUnitConverter` class implements all these conversions. Adapters can provide custom converters for proprietary units.
