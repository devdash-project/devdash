#!/bin/bash
# Setup virtual CAN interface for development and testing
#
# Usage: ./scripts/setup-vcan.sh [interface_name]
# Default interface: vcan0

set -e

INTERFACE="${1:-vcan0}"

echo "Setting up virtual CAN interface: $INTERFACE"

# Load vcan kernel module
if ! lsmod | grep -q "^vcan"; then
    echo "Loading vcan kernel module..."
    sudo modprobe vcan
fi

# Check if interface already exists
if ip link show "$INTERFACE" &>/dev/null; then
    echo "Interface $INTERFACE already exists"

    # Make sure it's up
    if ! ip link show "$INTERFACE" | grep -q "UP"; then
        echo "Bringing up $INTERFACE..."
        sudo ip link set "$INTERFACE" up
    fi
else
    echo "Creating $INTERFACE..."
    sudo ip link add dev "$INTERFACE" type vcan
    sudo ip link set "$INTERFACE" up
fi

# Verify interface is up
if ip link show "$INTERFACE" | grep -q "UP"; then
    echo "Virtual CAN interface $INTERFACE is ready"
    echo ""
    echo "Test with:"
    echo "  candump $INTERFACE"
    echo "  cansend $INTERFACE 360#0DAC01F400000000"
else
    echo "ERROR: Failed to bring up $INTERFACE"
    exit 1
fi
