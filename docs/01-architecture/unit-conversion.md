# Unit Conversion System

## Principle: Keep Adapter Data Pure

**Rule**: Adapters report data in their **native units**. No ETL (Extract-Transform-Load) in the adapter.

Example:
- Haltech reports temperature in **Kelvin** (as per spec)
- OBD-II reports temperature in **Celsius** (as per spec)
- Inclinometer reports angles in **radians** (sensor native)

Unit conversion happens at the **DataBroker** level based on user preferences.

## Architecture

```
Data Source              Queue                 DataBroker
(Haltech)                                      (User Pref: °F)
   │                                                │
   │ coolantTemp = 373K                             │
   ├─────────────────────────────────────────►      │
   │ (sourceUnit: "K")                              │
   │                                                │
   │                                  1. Get value: 373K, sourceUnit: "K"
   │                                  2. Lookup channel type: "temperature"
   │                                  3. Lookup user pref: "F"
   │                                  4. Convert: K → F
   │                                  5. Store: 211.73°F
   │                                                │
   │                                                ▼
   │                                            QML Gauge
   │                                           (shows 211.73°F)
```

## IUnitConverter Interface

```cpp
class IUnitConverter {
public:
    virtual ~IUnitConverter() = default;

    virtual double convert(double value,
                          const QString& fromUnit,
                          const QString& toUnit) const = 0;

    virtual bool canConvert(const QString& fromUnit,
                           const QString& toUnit) const = 0;
};
```

## DefaultUnitConverter

Built-in converter handles common conversions:

| Category | Supported Units |
|----------|-----------------|
| Temperature | K, C, F |
| Pressure | kPa, psi, bar, inHg |
| Speed | km/h, mph, m/s |
| Distance | km, mi, m, ft |
| Angle | rad, deg |
| Volume | L, gal (US), gal (UK) |

## Adapter-Provided Converters

Adapters can provide custom converters for proprietary units:

```cpp
class HaltechAdapter : public IDataSource {
public:
    std::unique_ptr<IUnitConverter> createUnitConverter() override {
        return std::make_unique<HaltechUnitConverter>();
    }
};
```

## Channel Metadata

Channels declare their unit type:

```json
{
  "channels": {
    "coolantTemp": {
      "unitType": "temperature",
      "defaultUnit": "C"
    },
    "oilPressure": {
      "unitType": "pressure",
      "defaultUnit": "kPa"
    }
  }
}
```

## User Preferences

```json
{
  "userPreferences": {
    "units": {
      "temperature": "F",
      "pressure": "psi",
      "speed": "mph"
    }
  }
}
```

## Benefits

1. **Adapter Simplicity**: No conversion logic in adapters
2. **Data Integrity**: Raw data preserved in queue
3. **User Choice**: Each user sets their preferred units
4. **Extensibility**: Easy to add new unit types
5. **Debugging**: Can inspect raw values vs converted values

## See Also

- [Dash Protocol](dash-protocol.md) - Overall data flow
- [Unit Types Reference](../reference/unit-types.md) - Full conversion tables
