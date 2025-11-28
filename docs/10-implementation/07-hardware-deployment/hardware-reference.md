# Moon Patrol - Hardware Reference

A summary of hardware discussed for the rock crawler dual-display infotainment system.

## System Overview

**Architecture:**
- Dual-display system (instrument cluster + head unit)
- GMSL2 camera system (4-8 cameras)
- CAN bus integration with Haltech Elite 2500 ECU
- Rugged industrial compute platform

---

## Compute Platform

### NVIDIA Jetson Orin Nano 8GB Module

**Why Jetson:**
- Native GMSL2 deserializer support
- GStreamer + nvarguscamerasrc for low-latency video
- Qt 6 runs well with EGLFS
- Enough GPU horsepower for multi-camera + gauges + emulation
- JetPack 6 (Ubuntu 22.04 based) is the target OS

**Module Only (not dev kit):** ~$200-250

---

## Carrier Board

### Option 1: Forecr DSBOARD-ORNX (Recommended)

**Source:** [forecr.io](https://www.forecr.io/products/carrier-board-dsboard-ornx)

**Key Specs:**
- Wide voltage input: **9-28 VDC** (runs directly off vehicle electrical)
- Operating temperature: **-25°C to +85°C**
- Rugged industrial design
- CAN bus interface
- RS232/RS422
- CSI camera connectors (for GMSL deserializer board)
- Multiple USB 3.0/2.0 ports
- HDMI + DisplayPort outputs

**Pricing:** Contact for quote, estimate ~€400-500 (~$450-550)

**With WiFi Module + Orin NX 8GB + 128GB SSD:** €892 (~$975)

### Option 2: D3 Embedded 8-Camera Carrier

**Source:** [d3embedded.com](https://www.d3embedded.com/product/designcore-nvidia-jetson-orin-nano-8-camera-gmsl2-carrier-board/)

**Key Specs:**
- Up to 8 GMSL2 camera inputs built-in
- Designed for benchtop, prototyping, or production
- Includes 1-to-4 HFM to FAKRA breakout cable

**Pricing:** ~$1,999-2,399 (complete carrier with deserializers)

**Pros:** All-in-one solution, no separate GMSL board needed
**Cons:** More expensive, may be overkill if only need 4 cameras

---

## GMSL2 Camera Interface

### Forecr GMSL2 Camera Board for Orin NX/Orin Nano

**Source:** [forecr.io](https://www.forecr.io/products/gmsl2-camera-board-for-orin-nx-orin-nano)

**Key Specs:**
- **4 camera connections** (quad channel)
- GMSL2 protocol (low latency, long cable runs)
- Locking IPEX MHF4L LK connectors (won't vibrate loose)
- **Power over Coax (PoC):** 8V & 12V selectable via DIP switch
- Current limiting protection per channel
- Designed to mate with Forecr carrier boards

**Pricing:** €430 (~$470)

### Alternative: Waveshare MAX9296A

**Specs:**
- 2 camera connections per board
- Lower cost entry point
- Need two boards for 4 cameras

**Pricing:** ~$117 (Aliexpress) / ~$144 (Amazon) per board

---

## Cameras

### Option 1: Waveshare ISX031 IP67 GMSL Camera (Pre-built)

**Source:** Amazon, Waveshare

**Key Specs:**
- Sony ISX031 sensor
- IP67 weatherproof
- GMSL2 interface
- Pre-built housing with mounting

**Pricing:** ~$500 per camera

**Pros:** Ready to install, IP67 rated
**Cons:** Expensive, limited customization

### Option 2: DIY Camera Build (Serializer Board + Sensor + Housing)

**Approach:** Buy a GMSL2 serializer board + camera sensor module + build custom IP67 housing

**Serializer Boards:**
- Videtronic / Innoface serializer modules
- ~$115-165 per camera vs $500 pre-built
- Requires custom housing fabrication

**Sensor Options:**
- Sony IMX327 (excellent low-light, 1080p)
- Sony ISX031 (automotive grade)
- Various M12 lens options (wide angle, etc.)

**Custom Housing Components:**
- Small aluminum project box (70mm × 70mm × 30mm)
- IP67 cable glands
- Polycarbonate/acrylic optical window
- Silicone gaskets
- Waterproof RJ45 or FAKRA connectors

**Total DIY Cost:** ~$150-200 per camera (plus fab time)

### Camera Specs to Consider

| Spec | Recommended |
|------|-------------|
| Resolution | 1080p minimum, 4K optional |
| Lens | 2.8mm wide angle (~110° FOV) for underbody/surround |
| Low Light | Sony Starvis sensors (IMX327, IMX290) |
| Frame Rate | 30fps minimum |
| Interface | GMSL2 with PoC |
| Weather Rating | IP67 minimum |
| Temp Range | -40°C to +85°C for automotive |

---

## Displays

### Instrument Cluster (Driver-facing)

**Requirements:**
- Non-touch (driver shouldn't be poking at it)
- High brightness for daylight visibility
- 7-10" diagonal
- HDMI or DisplayPort input

**Options to Research:**
- Industrial/automotive TFT panels
- Raspberry Pi 7" display (dev/testing)
- Eyoyo portable monitors (budget option)

### Head Unit (Center Console)

**Requirements:**
- Capacitive touch
- 10-12" diagonal
- HDMI input with USB touch
- Possibly portrait orientation

**Options to Research:**
- Waveshare touch displays
- Industrial touch panels
- Android Auto style aftermarket units (gut for display only)

---

## ECU Interface

### Haltech Elite 2500

**CAN Bus Specs:**
- **Bus Speed:** 1 Mbit/s (NOT 500kbps)
- **ID Format:** 11-bit standard CAN
- **Encoding:** Big endian
- **Protocol Version:** V2.35

**USB-CAN Adapters:**
- PCAN-USB (Peak Systems) - industrial grade
- CANable (open source, cheap)
- Kvaser Leaf Light (reliable)
- Native CAN on Forecr carrier board

**Key Data Available:**
- RPM, TPS, MAP
- Coolant/oil/air/fuel temperatures (in Kelvin)
- Oil/fuel/coolant pressures
- Wheel speeds (individual)
- Lambda/AFR (up to 4 widebands)
- Gear position
- Status flags (check engine, oil light, brake, clutch, etc.)

---

## Audio

### Options

1. **USB DAC** - Simplest, wide selection
2. **I2S Codec Board** - Lower latency, more DIY
3. **Jetson Onboard RT5640** - Built into Orin, 8-192kHz sample rates

**Software Stack:** PipeWire on top of ALSA (replaces PulseAudio, better for low-latency)

---

## Budget Summary

### Minimum Viable POC (4 cameras, Forecr stack)

| Component | Est. Price |
|-----------|------------|
| Jetson Orin Nano 8GB module | $200-250 |
| Forecr DSBOARD-ORNX carrier | ~$450 |
| Forecr GMSL2 4-channel board | ~$470 |
| 4x IP67 GMSL cameras (pre-built) | ~$2,000 |
| 2x Displays (cluster + head unit) | ~$200-400 |
| USB-CAN adapter | ~$50-100 |
| Cables, connectors, misc | ~$100-200 |
| **Total** | **~$3,500-4,000** |

### DIY Camera Build (Cost Savings)

| Component | Est. Price |
|-----------|------------|
| Jetson Orin Nano 8GB module | $200-250 |
| Forecr DSBOARD-ORNX carrier | ~$450 |
| Forecr GMSL2 4-channel board | ~$470 |
| 4x DIY cameras (serializer + sensor + housing) | ~$600-800 |
| 2x Displays | ~$200-400 |
| USB-CAN adapter | ~$50-100 |
| Cables, connectors, misc | ~$100-200 |
| **Total** | **~$2,100-2,700** |

---

## Vendor Links

| Vendor | Products | URL |
|--------|----------|-----|
| Forecr | Carrier boards, GMSL boards | https://www.forecr.io |
| D3 Embedded | GMSL carrier boards | https://www.d3embedded.com |
| Waveshare | GMSL cameras, deserializers, displays | https://www.waveshare.com |
| e-con Systems | Industrial GMSL cameras | https://www.e-consystems.com |
| NVIDIA | Jetson modules, JetPack | https://developer.nvidia.com/jetson |
| Haltech | ECU documentation | https://www.haltech.com |

---

## Reference Documents

- Haltech CAN Protocol V2.35: `protocols/haltech/haltech-can-protocol-v2.35.json`
- NVIDIA Jetson Orin Nano Datasheet
- Forecr DSBOARD-ORNX User Manual
- GMSL2 Application Notes (Maxim/ADI)

---

## Open Questions

1. **Final display specs** - Resolution, size, mounting
2. **Camera count and placement** - 4 minimum, up to 8?
3. **Physical buttons** - GPIO inputs or USB HID controller?
4. **Game controllers** - Bluetooth or USB wired for emulation?
5. **Audio output** - USB DAC model or I2S codec board?

---

*Last Updated: November 2025*
