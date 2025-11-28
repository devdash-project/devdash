FROM arm64v8/ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

# System packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    pkg-config \
    clang \
    clang-format \
    clang-tidy \
    gdb \
    # Qt 6
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-multimedia-dev \
    qt6-serialbus-dev \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-shapes \
    # GStreamer
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    # CAN
    can-utils \
    iproute2 \
    # Python (for haltech-mock)
    python3 \
    python3-pip \
    python3-venv \
    # Node.js (for Claude Code)
    nodejs \
    npm \
    && rm -rf /var/lib/apt/lists/*

# Display & UI testing tools
RUN apt-get update && apt-get install -y \
    xvfb \
    x11vnc \
    novnc \
    websockify \
    openbox \
    xdotool \
    imagemagick \
    && rm -rf /var/lib/apt/lists/*

# Qt platform settings
ENV DISPLAY=:99
ENV QT_QPA_PLATFORM=xcb

# Install Claude Code globally
# Users authenticate with their own Anthropic account on first run
RUN npm install -g @anthropic-ai/claude-code

# Install haltech-mock in a venv (if available)
RUN python3 -m venv /opt/haltech-venv && \
    /opt/haltech-venv/bin/pip install haltech-mock || echo "haltech-mock not yet published"
ENV PATH="/opt/haltech-venv/bin:$PATH"

# Create non-root user (handle case where UID/GID 1000 already exists in Ubuntu 24.04)
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID
RUN apt-get update && apt-get install -y sudo \
    && rm -rf /var/lib/apt/lists/* \
    # Get existing user with UID 1000 or create new one
    && EXISTING_USER=$(getent passwd $USER_UID | cut -d: -f1) \
    && if [ -z "$EXISTING_USER" ]; then \
         groupadd --gid $USER_GID $USERNAME; \
         useradd --uid $USER_UID --gid $USER_GID -m $USERNAME; \
         EXISTING_USER=$USERNAME; \
       fi \
    && echo "$EXISTING_USER ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$EXISTING_USER \
    && chmod 0440 /etc/sudoers.d/$EXISTING_USER

USER $USER_UID
WORKDIR /workspace
