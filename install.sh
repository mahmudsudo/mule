#!/bin/bash

# Mule Installer
# Installs the latest release of Mule from GitHub.

set -e

REPO="mahmudsudo/mule"
LATEST_RELEASE_URL="https://api.github.com/repos/$REPO/releases/latest"

echo "🐴 Mule Installer"
echo "----------------"

# Detect OS
OS="$(uname -s)"
case "$OS" in
    Linux)
        OS_TYPE="linux"
        ;;
    Darwin)
        OS_TYPE="macos"
        ;;
    *)
        echo "Error: Unsupported OS '$OS'"
        exit 1
        ;;
esac

# Detect Arch
ARCH="$(uname -m)"
if [ "$ARCH" = "x86_64" ]; then
    ARCH_TYPE="x86_64"
elif [ "$ARCH" = "arm64" ]; then
    ARCH_TYPE="arm64"
else
    echo "Error: Unsupported architecture '$ARCH'"
    exit 1
fi

ASSET_NAME="mule-${OS_TYPE}-${ARCH_TYPE}.tar.gz"

echo "🔍 Detecting system... $OS_TYPE $ARCH_TYPE"

# Get Download URL
echo "🌐 Fetching latest release info..."
DOWNLOAD_URL=$(curl -s $LATEST_RELEASE_URL | grep "browser_download_url.*$ASSET_NAME" | awk -F '"' '{print $4}')

if [ -z "$DOWNLOAD_URL" ]; then
    echo "Error: Could not find a release asset for $ASSET_NAME"
    exit 1
fi

echo "⬇️  Downloading from $DOWNLOAD_URL"
curl -L -o mule.tar.gz "$DOWNLOAD_URL"

echo "📦 Extracting..."
tar -xzf mule.tar.gz

echo "💿 Installing to /usr/local/bin (requires sudo)..."
if command -v sudo >/dev/null 2>&1; then
    sudo mv mule-${OS_TYPE}-${ARCH_TYPE} /usr/local/bin/mule
else
    mv mule-${OS_TYPE}-${ARCH_TYPE} /usr/local/bin/mule
fi

# Cleanup
rm mule.tar.gz

echo "✅ Success! Mule has been installed."
echo "   Run 'mule --help' to get started."
