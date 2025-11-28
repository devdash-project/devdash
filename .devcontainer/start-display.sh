#!/bin/bash
# Start virtual display for Qt UI development

# Start Xvfb
Xvfb :99 -screen 0 1920x1080x24 &
sleep 1

# Minimal window manager
openbox &
sleep 0.5

# VNC server (no password for local dev)
x11vnc -display :99 -forever -shared -nopw -quiet &

# noVNC web interface
websockify --web=/usr/share/novnc 6080 localhost:5900 &

echo "✓ Display ready - open http://localhost:6080 in browser"