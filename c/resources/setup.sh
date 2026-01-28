#!/bin/bash
#
# Download Kokoro TTS Model Files and Generate Demo
#
# This script downloads the required ONNX model and voice files,
# converts them to C format, and generates a demo WAV file.
#

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Model URLs
MODEL_BASE_URL="https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0"
MODEL_FILE="kokoro-v1.0.onnx"
VOICES_FILE="voices-v1.0.bin"
VOICES_C_FILE="voices-v1.0-c.bin"

# Demo settings
DEMO_TEXT="Hello! This is a demonstration of the Kokoro Text to Speech system. It supports multiple languages and voices with natural sounding speech."
DEMO_OUTPUT="demo.wav"
DEMO_VOICE="af_sarah"
DEMO_LANG="en-us"
DEMO_SPEED="1.0"

echo -e "${BLUE}Kokoro TTS - Resource Setup${NC}"
echo -e "${BLUE}============================${NC}\n"

# Check if wget or curl is available
if command -v wget &> /dev/null; then
    DOWNLOAD_CMD="wget -q --show-progress"
elif command -v curl &> /dev/null; then
    DOWNLOAD_CMD="curl -L -O --progress-bar"
else
    echo -e "${RED}Error: Neither wget nor curl found. Please install one of them.${NC}"
    exit 1
fi

# Function to download file if not exists
download_file() {
    local url=$1
    local filename=$2
    
    if [ -f "$filename" ]; then
        echo -e "${YELLOW}✓ $filename already exists, skipping download${NC}"
        return 0
    fi
    
    echo -e "${BLUE}Downloading $filename...${NC}"
    if command -v wget &> /dev/null; then
        wget -q --show-progress "$url" -O "$filename"
    else
        curl -L "$url" -o "$filename" --progress-bar
    fi
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Downloaded $filename${NC}"
    else
        echo -e "${RED}✗ Failed to download $filename${NC}"
        return 1
    fi
}

# Check if Python is available for voice conversion
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: python3 not found. Required for voice file conversion.${NC}"
    exit 1
fi

# Download model file
echo -e "\n${BLUE}Step 1: Downloading ONNX Model${NC}"
echo "  File: $MODEL_FILE (~300MB)"
echo "  This may take a few minutes..."
download_file "$MODEL_BASE_URL/$MODEL_FILE" "$MODEL_FILE"

# Download voices file
echo -e "\n${BLUE}Step 2: Downloading Voices File${NC}"
echo "  File: $VOICES_FILE (~80MB)"
download_file "$MODEL_BASE_URL/$VOICES_FILE" "$VOICES_FILE"

# Convert voices to C format
echo -e "\n${BLUE}Step 3: Converting Voices to C Format${NC}"
if [ -f "$VOICES_C_FILE" ]; then
    echo -e "${YELLOW}✓ $VOICES_C_FILE already exists, skipping conversion${NC}"
else
    echo "  Converting $VOICES_FILE to $VOICES_C_FILE..."
    if [ -f "../../scripts/convert_voices.py" ]; then
        python3 ../../scripts/convert_voices.py "$VOICES_FILE" "$VOICES_C_FILE"
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Converted to C format${NC}"
        else
            echo -e "${RED}✗ Conversion failed${NC}"
            exit 1
        fi
    else
        echo -e "${RED}✗ Conversion script not found at ../../scripts/convert_voices.py${NC}"
        exit 1
    fi
fi

# Check if example binary exists
EXAMPLE_BIN="../build/kokoro_example"
if [ ! -f "$EXAMPLE_BIN" ]; then
    echo -e "\n${YELLOW}Building examples...${NC}"
    cd ..
    if [ ! -d "build" ]; then
        mkdir build
        cd build
        cmake ..
        make
        cd ..
    else
        cd build
        make
        cd ..
    fi
    cd resources
fi

# Generate demo WAV file
echo -e "\n${BLUE}Step 4: Generating Demo Audio${NC}"
if [ -f "$DEMO_OUTPUT" ]; then
    echo -e "${YELLOW}✓ $DEMO_OUTPUT already exists${NC}"
    echo -e "${YELLOW}  Delete it to regenerate or skip this step${NC}"
else
    echo "  Text: \"$DEMO_TEXT\""
    echo "  Voice: $DEMO_VOICE"
    echo "  Language: $DEMO_LANG"
    echo "  Speed: $DEMO_SPEED"
    
    if [ -f "$EXAMPLE_BIN" ]; then
        "$EXAMPLE_BIN" "$MODEL_FILE" "$VOICES_C_FILE" "$DEMO_OUTPUT" "$DEMO_VOICE" "$DEMO_TEXT"
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Generated $DEMO_OUTPUT${NC}"
            
            # Get file size
            if [ -f "$DEMO_OUTPUT" ]; then
                SIZE=$(du -h "$DEMO_OUTPUT" | cut -f1)
                echo -e "${GREEN}  File size: $SIZE${NC}"
            fi
        else
            echo -e "${RED}✗ Failed to generate demo${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Example binary not found. Build the project first:${NC}"
        echo "    cd .. && make"
        echo -e "${YELLOW}  Then run this script again to generate the demo.${NC}"
    fi
fi

# Summary
echo -e "\n${GREEN}Setup Complete!${NC}"
echo -e "\n${BLUE}Downloaded Files:${NC}"
[ -f "$MODEL_FILE" ] && echo "  ✓ $MODEL_FILE" || echo "  ✗ $MODEL_FILE"
[ -f "$VOICES_FILE" ] && echo "  ✓ $VOICES_FILE" || echo "  ✗ $VOICES_FILE"
[ -f "$VOICES_C_FILE" ] && echo "  ✓ $VOICES_C_FILE" || echo "  ✗ $VOICES_C_FILE"
[ -f "$DEMO_OUTPUT" ] && echo "  ✓ $DEMO_OUTPUT" || echo "  ✗ $DEMO_OUTPUT (run after building)"

echo -e "\n${BLUE}Next Steps:${NC}"
echo "  1. Build the project (if not already done):"
echo "     cd .. && make"
echo ""
echo "  2. Run the basic example:"
echo "     cd resources"
echo "     ../build/kokoro_example $MODEL_FILE $VOICES_C_FILE output.wav"
echo ""
echo "  3. Generate an audiobook:"
echo "     cd resources"
echo "     ../build/kokoro_audiobook $MODEL_FILE $VOICES_C_FILE ../examples/sample_story.txt audiobook.wav"
echo ""
echo "  4. Play the demo (if generated):"
if [ -f "$DEMO_OUTPUT" ]; then
    if command -v aplay &> /dev/null; then
        echo "     aplay $DEMO_OUTPUT"
    elif command -v afplay &> /dev/null; then
        echo "     afplay $DEMO_OUTPUT"
    else
        echo "     Use your preferred audio player to play $DEMO_OUTPUT"
    fi
fi

echo ""
