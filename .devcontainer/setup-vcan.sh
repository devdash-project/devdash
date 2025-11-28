#!/bin/bash
set -e

# Load vcan module
sudo modprobe vcan || echo "vcan module not available (expected in container)"

# Create virtual CAN interface
sudo ip link add dev vcan0 type vcan 2>/dev/null || true
sudo ip link set up vcan0 2>/dev/null || true

echo "Virtual CAN interface vcan0 configured (if supported)"
