# Development Environment Setup

This guide covers setting up a DevDash development environment on macOS, Linux, and Windows.

## Prerequisites

- **VS Code** with Dev Containers extension
- **Docker Engine** (Linux), **Docker Desktop** (Windows), or **UTM + Ubuntu VM** (macOS - recommended)
- **Git**

## Platform-Specific Setup

### macOS (UTM with Ubuntu VM) - Recommended

Docker Desktop for macOS does **not** support CAN/vcan (the kernel lacks `CONFIG_CAN`). For full CAN bus development, we use UTM to run an ARM64 Ubuntu VM with Docker inside.

**Why UTM?**
- Native ARM64 performance on Apple Silicon (same architecture as Jetson target)
- Full Linux kernel with CAN/vcan support
- Complete end-to-end testing environment with GUI support

#### 1. Install UTM

```bash
# Download UTM from https://mac.getutm.app/ or via Homebrew
brew install --cask utm
```

#### 2. Create Ubuntu VM

1. Download **Ubuntu 24.04 LTS ARM64** (Server or Desktop):
   - Server (lighter): https://ubuntu.com/download/server/arm
   - Desktop: https://cdimage.ubuntu.com/jammy/daily-live/current/

2. Open UTM and click **+** → **Virtualize** → **Linux**

3. Configure the VM:
   - **Boot ISO Image**: Select the downloaded Ubuntu ISO
   - **Memory**: 8192 MB (8 GB) minimum for Qt development
   - **CPU Cores**: 4+ recommended
   - **Storage**: 64 GB minimum
   - **Enable**: "Use Apple Virtualization" for best performance

4. Start the VM and complete Ubuntu installation

5. After installation, eject the ISO:
   - Select VM → Click CD/DVD dropdown → **Clear**

#### 3. Configure Ubuntu VM for Development

After Ubuntu boots, open a terminal and run:

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Docker
sudo apt install -y docker.io docker-compose-v2 git can-utils
sudo usermod -aG docker $USER
newgrp docker

# Install VS Code (optional - can also use Remote-SSH from macOS)
sudo snap install code --classic

# Set up vcan0 to start automatically
sudo tee /etc/systemd/system/vcan0.service << 'EOF'
[Unit]
Description=Virtual CAN interface
After=network.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/bin/bash -c 'modprobe vcan && ip link add dev vcan0 type vcan && ip link set vcan0 up'
ExecStop=/bin/bash -c 'ip link set vcan0 down && ip link del vcan0'

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl enable vcan0.service
sudo systemctl start vcan0.service

# Verify vcan0 is running
ip link show vcan0
```

#### 4. Connect VS Code from macOS (Optional)

You can develop directly in the Ubuntu VM, or connect from macOS VS Code:

1. In Ubuntu VM, note the IP address:
   ```bash
   ip addr show enp0s1 | grep inet
   ```

2. Enable SSH in Ubuntu:
   ```bash
   sudo apt install -y openssh-server
   sudo systemctl enable ssh
   ```

3. In macOS VS Code:
   - Install **Remote - SSH** extension
   - Press **Cmd+Shift+P** → **Remote-SSH: Connect to Host**
   - Enter: `username@<vm-ip-address>`

#### 5. Clone and Build DevDash

Inside the Ubuntu VM (directly or via SSH):

```bash
# Clone the repo
git clone https://github.com/your-org/devdash.git
cd devdash

# Open in VS Code (if running VS Code in VM)
code .

# Or use Dev Containers
# Press Ctrl+Shift+P → "Dev Containers: Reopen in Container"
```

#### 6. Verify Setup

```bash
# Check vcan0 is accessible
ip link show vcan0

# Should show:
# vcan0: <NOARP,UP,LOWER_UP> mtu 72 qdisc noqueue state UNKNOWN

# Test CAN tools
cansend vcan0 123#DEADBEEF
candump vcan0 &
cansend vcan0 456#CAFEBABE
# Should see the CAN frame echoed
```

### macOS (Docker Desktop) - Limited Support

> **Warning:** Docker Desktop's LinuxKit kernel does not include CAN support (`CONFIG_CAN` is not set). The vcan module cannot be loaded. Use the UTM approach above for CAN development.

Docker Desktop can still be used for non-CAN development tasks (building, testing business logic, etc.):

```bash
# Install Docker Desktop
brew install --cask docker

# Start Docker Desktop
open -a Docker

# Clone and open in Dev Container
git clone https://github.com/your-org/devdash.git
cd devdash
code .
# Cmd+Shift+P → "Dev Containers: Reopen in Container"
```

CAN-related tests will be skipped or mocked when vcan0 is unavailable.

### Linux (Native Docker)

#### 1. Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y docker.io git can-utils
sudo usermod -aG docker $USER
# Log out and back in for group changes to take effect
```

**Arch:**
```bash
sudo pacman -S docker git can-utils
sudo usermod -aG docker $USER
# Log out and back in
```

#### 2. Set Up Virtual CAN Interface

```bash
# Load vcan kernel module
sudo modprobe vcan

# Create vcan0 interface
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up

# Verify
ip link show vcan0
```

#### 3. Open in Dev Container

```bash
git clone https://github.com/your-org/devdash.git
cd devdash
code .
```

In VS Code:
1. Press **Ctrl+Shift+P**
2. Type: **"Dev Containers: Reopen in Container"**
3. Press **Enter**

### Windows (WSL2 + Docker Desktop)

#### 1. Enable WSL2

```powershell
# In PowerShell as Administrator
wsl --install
wsl --set-default-version 2
```

Restart your computer.

#### 2. Install Docker Desktop

Download and install [Docker Desktop for Windows](https://www.docker.com/products/docker-desktop/).

Enable WSL2 integration:
1. Open Docker Desktop settings
2. Go to **Resources → WSL Integration**
3. Enable integration for your WSL2 distro (e.g., Ubuntu)

#### 3. Set Up vcan0 in WSL2

```bash
# In your WSL2 terminal (Ubuntu)
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up

# Verify
ip link show vcan0
```

#### 4. Open in Dev Container

```bash
# In WSL2 terminal
git clone https://github.com/your-org/devdash.git
cd devdash
code .
```

The dev container should start automatically in WSL2 mode.

## Making vcan0 Persistent

### macOS (UTM)

If using the UTM approach, vcan0 persistence is handled by the systemd service created during VM setup (see [Configure Ubuntu VM for Development](#3-configure-ubuntu-vm-for-development) above).

The service starts automatically when the VM boots:
```bash
# Check status
sudo systemctl status vcan0.service

# Restart if needed
sudo systemctl restart vcan0.service
```

### Linux

Add to system startup via systemd:

Create `/etc/systemd/system/vcan0.service`:
```ini
[Unit]
Description=Virtual CAN interface
After=network.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/bin/bash -c 'modprobe vcan && ip link add dev vcan0 type vcan && ip link set vcan0 up'
ExecStop=/bin/bash -c 'ip link set vcan0 down && ip link del vcan0'

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable vcan0.service
sudo systemctl start vcan0.service
```

Or add to `/etc/rc.local` (if your distro uses it):
```bash
modprobe vcan
ip link add dev vcan0 type vcan
ip link set vcan0 up
```

### Windows (WSL2)

Add to WSL2 startup:

Create `/etc/wsl.conf` in your WSL2 distro:
```ini
[boot]
command = "modprobe vcan && ip link add dev vcan0 type vcan && ip link set vcan0 up"
```

This runs automatically when WSL2 starts.

## First Build and Test

Once your dev container is running:

```bash
# Build the project
cmake --preset dev
cmake --build build/dev

# Run tests
ctest --test-dir build/dev --output-on-failure

# Set up haltech-mock (one-time)
./scripts/setup-haltech-mock

# Run with mock data
./scripts/run-with-mock idle
```

You should see:
- Instrument cluster window with animated gauges
- Engine data updating in real-time (RPM, temp, pressure, etc.)
- Green connection indicator (top-right corner)

## Troubleshooting

### vcan0 not found in container

**Symptom:**
```
error: Device "vcan0" does not exist
```

**Solution:**
1. Exit the dev container
2. Create vcan0 on your host (see platform-specific instructions above)
3. In VS Code: **Cmd/Ctrl+Shift+P** → **"Dev Containers: Rebuild Container"**

The rebuild is necessary because the container uses `--network=host` to access vcan0.

### Docker Desktop doesn't support vcan (macOS)

**Symptom:**
```
modprobe: FATAL: Module vcan not found in directory /lib/modules/...
```

**Cause:** Docker Desktop's LinuxKit kernel does not include CAN support (`CONFIG_CAN` is not set).

**Solution:** Use the UTM + Ubuntu VM approach described above. Docker Desktop cannot support vcan on macOS.

### UTM VM networking issues (macOS)

**Symptom:** Cannot SSH to Ubuntu VM from macOS.

**Solution:**
1. In UTM, ensure the VM network is set to "Shared Network" (NAT)
2. In Ubuntu VM, check the IP: `ip addr show`
3. Ensure SSH is running: `sudo systemctl status ssh`
4. If using Apple Virtualization, the VM may get a different IP each boot. Consider setting a static IP or using `avahi-daemon` for `.local` hostname resolution:
   ```bash
   sudo apt install avahi-daemon
   # Then connect via: ssh username@hostname.local
   ```

### Permission denied (Linux)

**Symptom:**
```
ERROR: permission denied while trying to connect to the Docker daemon socket
```

**Solution:**
```bash
sudo usermod -aG docker $USER
# Log out and back in
```

### Build fails with Qt errors

**Symptom:**
```
Could not find Qt6 components
```

**Solution:**
This shouldn't happen in the dev container (Qt is pre-installed). If it does:
1. Rebuild the dev container: **Cmd/Ctrl+Shift+P** → **"Dev Containers: Rebuild Container"**
2. If still failing, check Docker has enough resources (8GB+ RAM recommended)

### haltech-mock not found

**Symptom:**
```
haltech-mock: command not found
```

**Solution:**
```bash
# Install haltech-mock
pip install haltech-mock

# Or if using the devcontainer Python:
/opt/haltech-venv/bin/pip install haltech-mock
```

The setup script (`./scripts/setup-haltech-mock`) checks this automatically.

## IDE Configuration

### VS Code Extensions (Auto-installed in Dev Container)

The dev container automatically installs:
- C/C++ Extension Pack
- CMake Tools
- clangd (Language Server)
- QML support

### Recommended Settings

These are pre-configured in `.vscode/settings.json`:
- clang-format on save
- clangd diagnostics
- CMake integration

## Next Steps

- [Build & Run Guide](../../CLAUDE.md#build--run)
- [Creating a Vehicle Profile](../02-api-reference/vehicle-profiles.md)
- [Contributing Guide](../03-contributing/testing-guidelines.md)

## Quick Reference

**Build:**
```bash
cmake --build build/dev
```

**Test:**
```bash
ctest --test-dir build/dev
```

**Lint:**
```bash
cmake --build build/dev --target clang-tidy
```

**Format:**
```bash
cmake --build build/dev --target format-fix
```

**Run with Mock Data:**
```bash
./scripts/run-with-mock idle          # Idle scenario
./scripts/run-with-mock track         # Track driving
./scripts/run-with-mock overheat      # Warning test
./scripts/list-scenarios              # Show all scenarios
```
