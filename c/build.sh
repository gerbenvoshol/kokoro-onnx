#!/bin/bash
#
# Simple build script for Kokoro C library
# This script provides an easy way to build the library without CMake
#
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
ONNXRUNTIME_DIR=""
BUILD_TYPE="Release"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --onnxruntime)
            ONNXRUNTIME_DIR="$2"
            shift 2
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            echo -e "${YELLOW}Cleaning build directory...${NC}"
            rm -rf "$BUILD_DIR"
            echo -e "${GREEN}✓ Clean complete${NC}"
            exit 0
            ;;
        --help)
            echo "Kokoro C Library Build Script"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --prefix PATH        Installation prefix (default: /usr/local)"
            echo "  --onnxruntime PATH   Path to ONNX Runtime installation"
            echo "  --debug              Build in debug mode"
            echo "  --clean              Clean build directory"
            echo "  --help               Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0"
            echo "  $0 --prefix ~/.local"
            echo "  $0 --onnxruntime /opt/onnxruntime"
            echo "  $0 --debug"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

echo "Kokoro C Library - Build Script"
echo "================================"
echo ""

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake is not installed${NC}"
    echo "Please install CMake:"
    echo "  Ubuntu/Debian: sudo apt-get install cmake"
    echo "  macOS: brew install cmake"
    exit 1
fi

# Check for compiler
if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
    echo -e "${RED}Error: No C compiler found${NC}"
    echo "Please install a C compiler (gcc or clang)"
    exit 1
fi

# Create build directory
echo "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
echo ""
echo "Configuring with CMake..."
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"

if [ -n "$ONNXRUNTIME_DIR" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DONNXRUNTIME_DIR=$ONNXRUNTIME_DIR"
fi

if ! cmake .. $CMAKE_ARGS; then
    echo ""
    echo -e "${RED}✗ CMake configuration failed${NC}"
    echo ""
    echo "Common issues:"
    echo "  - ONNX Runtime not found: use --onnxruntime PATH"
    echo "  - espeak-ng not installed: install with package manager"
    exit 1
fi

# Build
echo ""
echo "Building..."
if ! make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2); then
    echo ""
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}✓ Build complete!${NC}"
echo ""
echo "Binary location: $BUILD_DIR/kokoro_example"
echo ""
echo "Next steps:"
echo "  1. Download model files (see QUICKSTART.md)"
echo "  2. Run the example:"
echo "     ./kokoro_example kokoro-v1.0.onnx voices-v1.0-c.bin output.wav"
echo "  3. Install system-wide (optional):"
echo "     sudo make install"
echo ""
