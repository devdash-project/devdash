# macOS Development with UTM

This guide covers setting up a DevDash development environment on macOS using UTM to run an ARM64 Ubuntu VM. This approach provides full CAN/vcan support that Docker Desktop cannot offer.

## Why UTM Instead of Docker Desktop?

Docker Desktop for macOS uses LinuxKit, which does **not** include CAN support (`CONFIG_CAN` is not compiled into the kernel). The vcan module cannot be loaded.

UTM with Ubuntu provides:
- Full Linux kernel with CAN/vcan support
- Native ARM64 performance on Apple Silicon (same architecture as Jetson target)
- Complete end-to-end testing with GUI support
- VirtioFS for fast shared folder access to your macOS files

## Prerequisites

- macOS with Apple Silicon (M1/M2/M3) or Intel
- ~70GB free disk space (64GB VM + ISO + overhead)
- 16GB+ RAM recommended (8GB allocated to VM)

## Quick Start

### 1. Install UTM

```bash
brew install --cask utm
```

Or download from https://mac.getutm.app/

### 2. Download Ubuntu 24.04 ARM64 Server

```bash
# Download to your Downloads folder
curl -L -o ~/Downloads/ubuntu-24.04-live-server-arm64.iso \
  "https://cdimage.ubuntu.com/releases/24.04/release/ubuntu-24.04.1-live-server-arm64.iso"
```

### 3. Create VM in UTM

1. Open UTM and click **+** (Create New VM)
2. Select **Virtualize** (not Emulate)
3. Select **Linux**
4. Configure:
   - **Boot ISO Image**: Browse to the downloaded Ubuntu ISO
   - **Memory**: 8192 MB (8 GB)
   - **CPU Cores**: 4 (or half your available cores)
   - **Storage**: 64 GB
   - **Enable**: "Use Apple Virtualization" for best performance

5. Before starting, add a shared directory:
   - Select the VM → Click **Edit** (gear icon)
   - Go to **Sharing**
   - Add your `~/Projects` directory
   - Set share name to `share`

6. Start the VM and complete Ubuntu Server installation:
   - Username: `dev` (or your preference)
   - Hostname: `devdash-vm`
   - Enable OpenSSH server
   - Skip additional snaps

7. After installation completes, power off the VM

8. Eject the ISO:
   - Select VM → Click **Edit**
   - Go to **Drives** → Select the CD/DVD drive → **Clear**

9. Start the VM

### 4. Run the Setup Script

Once Ubuntu boots and you log in:

```bash
# Clone devdash (or use shared folder)
git clone https://github.com/your-org/devdash.git
cd devdash

# Run the automated setup script
./scripts/setup-utm-vm.sh
```

This script installs:
- LXQt desktop environment
- Qt 6, GStreamer, PipeWire
- Build tools (cmake, ninja, clang)
- Docker
- Node.js LTS and Claude Code
- Python venv with haltech-mock
- Persistent vcan0 systemd service
- Shared folder mount at `/mnt/projects`

**Reboot after the script completes:**
```bash
sudo reboot
```

### 5. Verify Setup

After rebooting and logging into the LXQt desktop:

```bash
# Check vcan0
ip link show vcan0

# Check Docker
docker run --rm hello-world

# Check shared folder
ls ~/Projects

# Check haltech-mock
haltech-mock --version
```

### 6. Build DevDash

```bash
cd ~/Projects/devdash
cmake --preset dev
cmake --build build/dev
```

### 7. Run with Mock Data

```bash
./scripts/setup-haltech-mock   # Verify vcan0 setup
./scripts/run-with-mock idle   # Run with idle scenario
```

## Shared Folder Access

The setup script mounts your macOS `~/Projects` folder at:
- Mount point: `/mnt/projects`
- Symlink: `~/Projects`

Changes sync bidirectionally in real-time via VirtioFS.

**Note**: Git operations should work normally. If you see permission issues, ensure the shared directory is configured correctly in UTM settings.

## Using Claude Code in the VM

Claude Code is installed by the setup script. To start a session:

```bash
cd ~/Projects/devdash
claude
```

## Keyboard Mapping Issues

If Ctrl/Cmd keys don't work correctly:

1. Shut down the VM
2. In UTM: Edit VM → **Input**
3. Toggle **"Use Apple Keyboard"** option
4. Or try switching from "VirtIO" to "USB" keyboard mode
5. If issues persist, try disabling "Use Apple Virtualization" (uses QEMU backend instead)

Inside the VM, you can also reset the keyboard layout:
```bash
setxkbmap us
```

## Clipboard Sharing

The setup script installs `spice-vdagent` for clipboard sharing between macOS and the VM. This activates automatically when you log into the LXQt desktop.

If clipboard isn't working:
```bash
# Check if spice-vdagent is running
pgrep -a spice-vdagent

# Restart if needed (will restart on next login)
killall spice-vdagent
spice-vdagent
```

## SSH Access from macOS (Optional)

You can connect to the VM via SSH instead of using the UTM window:

```bash
# In the VM, find the IP address
ip addr show enp0s1 | grep inet

# From macOS Terminal
ssh dev@<vm-ip-address>
```

For persistent hostname resolution, install avahi in the VM:
```bash
sudo apt install avahi-daemon
# Then connect via: ssh dev@devdash-vm.local
```

## Performance Tips

1. **Apple Virtualization**: Provides the best performance on Apple Silicon. Use this unless you have keyboard/compatibility issues.

2. **Memory**: 8GB is the minimum for Qt development. 12-16GB is better if your Mac has 32GB+ RAM.

3. **Cores**: Allocate half your cores to the VM. More cores = faster builds.

4. **Storage**: Use the default VirtIO storage for best I/O performance.

5. **Shared Folders**: VirtioFS (default with Apple Virtualization) is very fast. Avoid 9P/WebDAV if possible.

## Troubleshooting

### VM won't start after ISO install
Ensure you ejected the ISO (Edit VM → Drives → Clear the CD/DVD).

### vcan0 not found
```bash
sudo systemctl status vcan0.service
sudo systemctl restart vcan0.service
```

### Shared folder not mounted
```bash
# Check fstab entry
grep projects /etc/fstab

# Try manual mount
sudo mount -a

# Check UTM shared directory configuration
```

### Qt application shows no window
Ensure you're running from the LXQt desktop session (not SSH). Check:
```bash
echo $DISPLAY  # Should show :0 or similar
```

### Build fails with Qt errors
The dev container has Qt pre-installed, but the VM uses system Qt. Ensure Qt 6 packages are installed:
```bash
apt list --installed | grep qt6
```

## Related Documentation

- [Development Setup](./development-setup.md) - All platform setup options
- [haltech-mock Integration](../01-guides/haltech-mock.md) - Using the ECU simulator
