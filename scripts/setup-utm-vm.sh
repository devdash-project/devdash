#!/bin/bash
#
# setup-utm-vm.sh
# Sets up a fresh Ubuntu 24.04 Server VM in UTM for DevDash development
#
# Usage: curl -fsSL <raw-url> | bash
#    or: ./scripts/setup-utm-vm.sh
#
# Prerequisites:
#   - Fresh Ubuntu 24.04 Server ARM64 install in UTM
#   - Internet connection
#   - Run as regular user (script uses sudo where needed)
#

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() { echo -e "${BLUE}[INFO]${NC} $*"; }
log_success() { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# Check if running as root (we don't want that)
if [[ $EUID -eq 0 ]]; then
    log_error "Do not run this script as root. Run as regular user."
    exit 1
fi

echo ""
echo "=============================================="
echo "  DevDash UTM VM Setup Script"
echo "  Ubuntu 24.04 Server ARM64"
echo "=============================================="
echo ""

# -----------------------------------------------------------------------------
# Step 1: System Update
# -----------------------------------------------------------------------------
log_info "Updating system packages..."
sudo apt update && sudo apt upgrade -y
log_success "System updated"

# -----------------------------------------------------------------------------
# Step 2: Desktop Environment (LXQt)
# -----------------------------------------------------------------------------
if dpkg -l | grep -q "^ii.*sddm"; then
    log_info "LXQt/sddm already installed, skipping..."
else
    log_info "Installing LXQt desktop environment..."
    sudo apt install -y lxqt sddm
    sudo systemctl enable sddm
    log_success "LXQt installed"
fi

# -----------------------------------------------------------------------------
# Step 2b: SPICE guest agent (clipboard sharing with macOS)
# -----------------------------------------------------------------------------
log_info "Installing SPICE guest agent for clipboard sharing..."
sudo apt install -y spice-vdagent
# Note: spice-vdagent starts automatically via display manager, not systemd
log_success "SPICE guest agent installed (activates on graphical login)"

# -----------------------------------------------------------------------------
# Step 3: Core Development Tools
# -----------------------------------------------------------------------------
log_info "Installing core development tools..."
sudo apt install -y \
    git \
    curl \
    wget \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    clang \
    clang-format \
    clang-tidy \
    gdb \
    docker.io \
    docker-compose-v2 \
    can-utils \
    iproute2 \
    python3 \
    python3-pip \
    python3-venv
log_success "Core tools installed"

# -----------------------------------------------------------------------------
# Step 4: Qt 6 Development
# -----------------------------------------------------------------------------
log_info "Installing Qt 6 development packages..."
sudo apt install -y \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-multimedia-dev \
    qt6-serialbus-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtmultimedia \
    qml6-module-qtquick-window \
    qml6-module-qtqml-workerscript \
    libgl1-mesa-dev \
    libegl1-mesa-dev
log_success "Qt 6 installed"

# -----------------------------------------------------------------------------
# Step 4b: Qt 6.10+ via Qt Online Installer (for qml-gauges)
# -----------------------------------------------------------------------------
# The system Qt 6.2 works for devdash, but qml-gauges requires Qt 6.10+
# for QtQuick.Effects (MultiEffect) and CurveRenderer features.
# Qt Online Installer must be run interactively (GUI) to install Qt 6.10.
#
# After installing Qt 6.10.1 via the Online Installer, install WebSockets addon:
#   ~/Qt/MaintenanceTool install qt.qt6.6101.addons.qtwebsockets --accept-licenses --confirm-command
#
# This is documented here so it can be added to future automated setup if Qt
# supports headless installation of base Qt (currently requires GUI login).

# -----------------------------------------------------------------------------
# Step 5: GStreamer (video/camera support)
# -----------------------------------------------------------------------------
log_info "Installing GStreamer..."
sudo apt install -y \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev
log_success "GStreamer installed"

# -----------------------------------------------------------------------------
# Step 6: Audio (PipeWire)
# -----------------------------------------------------------------------------
log_info "Installing PipeWire audio..."
sudo apt install -y \
    pipewire \
    pipewire-audio-client-libraries \
    wireplumber
log_success "PipeWire installed"

# -----------------------------------------------------------------------------
# Step 7: Node.js LTS (via nvm)
# -----------------------------------------------------------------------------
log_info "Installing Node.js LTS via nvm..."
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash

# Load nvm for this session (check both possible locations)
# Disable strict mode temporarily - nvm has unbound variable issues
set +u
export NVM_DIR="${NVM_DIR:-$HOME/.nvm}"
[ -d "$HOME/.config/nvm" ] && export NVM_DIR="$HOME/.config/nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"

# Install Node.js LTS
nvm install --lts
nvm use --lts
nvm alias default lts/*
set -u
log_success "Node.js $(node --version) installed"

# -----------------------------------------------------------------------------
# Step 8: Claude Code
# -----------------------------------------------------------------------------
log_info "Installing Claude Code..."
npm install -g @anthropic-ai/claude-code
log_success "Claude Code installed"

# -----------------------------------------------------------------------------
# Step 9: Docker permissions
# -----------------------------------------------------------------------------
log_info "Adding user to docker group..."
sudo usermod -aG docker "$USER"
log_success "Docker group configured (re-login required)"

# -----------------------------------------------------------------------------
# Step 10: Virtual CAN (vcan0) systemd service
# -----------------------------------------------------------------------------
log_info "Setting up vcan0 systemd service..."
sudo tee /etc/systemd/system/vcan0.service > /dev/null << 'EOF'
[Unit]
Description=Virtual CAN interface for DevDash development
After=network.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/bin/bash -c 'modprobe vcan && ip link add dev vcan0 type vcan && ip link set vcan0 up'
ExecStop=/bin/bash -c 'ip link set vcan0 down && ip link del vcan0'

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable vcan0.service
sudo systemctl start vcan0.service
log_success "vcan0 service created and started"

# Verify vcan0
if ip link show vcan0 &>/dev/null; then
    log_success "vcan0 interface is UP"
else
    log_warn "vcan0 interface not found - may need reboot"
fi

# -----------------------------------------------------------------------------
# Step 11: Mount shared Projects folder (VirtioFS from UTM)
# -----------------------------------------------------------------------------
log_info "Configuring shared folder mount..."
sudo mkdir -p /mnt/projects

# Check if already in fstab
if ! grep -q "share /mnt/projects virtiofs" /etc/fstab; then
    echo 'share /mnt/projects virtiofs rw,nofail 0 0' | sudo tee -a /etc/fstab
    log_success "Added /mnt/projects to fstab"
else
    log_info "/mnt/projects already in fstab"
fi

# Try to mount now
if sudo mount -a 2>/dev/null; then
    if mountpoint -q /mnt/projects; then
        log_success "Shared folder mounted at /mnt/projects"
    else
        log_warn "Shared folder not mounted - check UTM shared directory config"
    fi
fi

# Create convenience symlink
if [[ ! -L "$HOME/Projects" ]] && [[ ! -d "$HOME/Projects" ]]; then
    ln -s /mnt/projects "$HOME/Projects"
    log_success "Created ~/Projects symlink"
fi

# -----------------------------------------------------------------------------
# Step 12: VS Code (ARM64 .deb package - snap not available for ARM64)
# -----------------------------------------------------------------------------
if command -v code &>/dev/null; then
    log_info "VS Code already installed, skipping..."
else
    log_info "Installing VS Code (ARM64 .deb)..."
    wget -qO /tmp/vscode.deb "https://code.visualstudio.com/sha/download?build=stable&os=linux-deb-arm64"
    # Pre-accept the Microsoft repo prompt
    echo "code code/add-microsoft-repo boolean true" | sudo debconf-set-selections
    sudo DEBIAN_FRONTEND=noninteractive dpkg -i /tmp/vscode.deb || sudo DEBIAN_FRONTEND=noninteractive apt-get install -f -y
    rm -f /tmp/vscode.deb
    log_success "VS Code installed"
fi

# -----------------------------------------------------------------------------
# Step 13: Python packages for haltech-mock
# -----------------------------------------------------------------------------
log_info "Setting up Python environment for haltech-mock..."
python3 -m venv "$HOME/.venv/devdash"
source "$HOME/.venv/devdash/bin/activate"
pip install --upgrade pip
pip install python-can haltech-mock
deactivate

# Add venv bin to PATH in .bashrc for easy access to haltech-mock
if ! grep -q "/.venv/devdash/bin" "$HOME/.bashrc"; then
    echo 'export PATH="$HOME/.venv/devdash/bin:$PATH"' >> "$HOME/.bashrc"
    log_info "Added ~/.venv/devdash/bin to PATH in .bashrc"
fi
log_success "Python venv with haltech-mock created at ~/.venv/devdash"

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
echo ""
echo "=============================================="
echo "  Setup Complete!"
echo "=============================================="
echo ""
echo "Installed:"
echo "  - LXQt desktop (sddm display manager)"
echo "  - Build tools (cmake, ninja, clang, gcc)"
echo "  - Qt 6.2 (system) with QML, Multimedia, SerialBus"
echo "  - GStreamer with plugins"
echo "  - PipeWire audio"
echo "  - Node.js LTS (via nvm)"
echo "  - Claude Code CLI"
echo "  - Docker"
echo "  - CAN utilities + vcan0 service"
echo "  - Python venv at ~/.venv/devdash"
echo ""
echo "For qml-gauges (requires Qt 6.10+):"
echo "  1. Install Qt 6.10.1 via Qt Online Installer (GUI required)"
echo "  2. Add WebSockets addon:"
echo "     ~/Qt/MaintenanceTool install qt.qt6.6101.addons.qtwebsockets --accept-licenses --confirm-command"
echo "  3. Build qml-gauges with:"
echo "     cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=\$HOME/Qt/6.10.1/gcc_arm64"
echo "     cmake --build build"
echo ""
echo "Shared folder: /mnt/projects (symlinked to ~/Projects)"
echo ""
echo -e "${YELLOW}ACTION REQUIRED:${NC}"
echo "  1. Reboot to start LXQt desktop: sudo reboot"
echo "  2. After reboot, log in to graphical session"
echo "  3. Open terminal and verify:"
echo "     - docker run hello-world"
echo "     - ip link show vcan0"
echo "     - ls ~/Projects"
echo ""
echo "To clone devdash:"
echo "  cd ~/Projects"
echo "  git clone <repo-url> devdash"
echo "  cd devdash"
echo "  cmake --preset dev && cmake --build build/dev"
echo ""
