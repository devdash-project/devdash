# UI Layer

**Status:** 🚧 Stub - To be completed

This document will describe the QML-based UI architecture for /dev/dash.

## Planned Content

### Dual Display Architecture

- ClusterWindow (instrument cluster, non-touch)
- HeadUnitWindow (center console, touch interface)
- Window management and display assignment
- Synchronization between windows

### QML Components

- Gauge components (Tachometer, Speedometer, DigitalGauge, BarGauge)
- Layout system (racing, cruising, minimal)
- Theming (day/night modes)
- Touch controls and button input

### DataBroker Integration

- Property bindings to DataBroker
- Real-time gauge updates
- Signal handling

### Styling and Theming

- Theme system
- Color schemes
- Font management
- Resolution independence

## Resources

Until this document is complete, refer to:

- **Architecture Overview:** [Overview](overview.md)
- **DataBroker Integration:** [DataBroker](data-broker.md)
- **Vehicle Profiles:** [Vehicle Profiles](vehicle-profiles.md) for gauge configuration

## TODO

- [ ] Document ClusterWindow architecture
- [ ] Document HeadUnitWindow architecture
- [ ] Explain gauge component system
- [ ] Document layout switching
- [ ] Explain theming system
- [ ] Add QML code examples
- [ ] Document performance optimization
- [ ] Add screenshots/diagrams

## Contributing

If you'd like to help complete this documentation, see [Contributing Guidelines](../03-contributing/code-style.md).
