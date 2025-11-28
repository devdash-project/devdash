# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

#### Documentation
- Comprehensive documentation structure with conceptual and implementation sections
- **Getting Started** guides:
  - Quick Start (5-minute setup)
  - Development Environment (dev container and manual setup)
  - Running Tests (test execution and debugging)
- **Architecture** documentation:
  - Overview with layer diagrams
  - Protocol Adapters (explains adapter pattern and Dependency Inversion Principle)
  - DataBroker (central data hub architecture)
  - Vehicle Profiles (JSON configuration system)
  - UI Layer (stub for future completion)
- **API Reference**:
  - Core Interfaces (IProtocolAdapter API)
  - Haltech Protocol API (HaltechProtocol and PD16Protocol)
  - DataBroker API (stub for future completion)
- **Contributing** guides:
  - Code Style (C++20, SOLID principles, Qt/QML conventions)
  - Adding Protocol Adapters (step-by-step implementation guide)
  - Testing Guidelines (Catch2 patterns and best practices)
  - Adding Gauges (stub for future completion)
- Navigation index in `docs/README.md`

#### Repository Organization
- Moved implementation-phase documentation to `docs/10-implementation/`
- Organized protocol files:
  - PDFs moved to `docs/references/`
  - JSON protocol definitions in `protocols/haltech/`

#### Haltech Adapter
- JSON-driven protocol decoder for Haltech Elite ECUs
- Support for Haltech CAN Broadcast Protocol v2.35
- PD16 power distribution module support
- 18 passing tests for protocol decoding

### Changed
- Documentation structure reorganized for clarity:
  - Conceptual docs in `docs/00-getting-started/`, `docs/01-architecture/`, etc.
  - Implementation-phase docs in `docs/10-implementation/`

### Fixed
- All documentation internal links verified and working

## [0.1.0] - YYYY-MM-DD

### Added
- Initial project structure
- CMake build system with presets (dev, release, ci)
- Core abstractions:
  - `IProtocolAdapter` interface
  - `DataBroker` class
  - `ChannelValue` struct
- Protocol adapter framework:
  - `ProtocolAdapterFactory`
  - Haltech adapter with JSON protocol definitions
  - Simulator adapter for development
- Testing framework:
  - Catch2 v3 integration
  - Unit tests for protocol decoding
- Development environment:
  - Dev container configuration
  - Virtual CAN setup scripts
  - clang-format and clang-tidy configuration
- Documentation:
  - Project specification
  - Repository setup guide
  - CLAUDE.md for AI assistant context

---

## Version History

- **Unreleased**: Current development version
- **0.1.0**: Initial release (planned)

## Contributing

See [CONTRIBUTING.md](docs/03-contributing/code-style.md) for how to contribute to this project.

## References

- [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
- [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
