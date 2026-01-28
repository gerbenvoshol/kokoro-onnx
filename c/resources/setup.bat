@echo off
REM Download Kokoro TTS Model Files and Generate Demo (Windows)
REM
REM This script downloads the required ONNX model and voice files,
REM converts them to C format, and generates a demo WAV file.
REM

setlocal enabledelayedexpansion

echo Kokoro TTS - Resource Setup
echo ============================
echo.

set MODEL_BASE_URL=https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0
set MODEL_FILE=kokoro-v1.0.onnx
set VOICES_FILE=voices-v1.0.bin
set VOICES_C_FILE=voices-v1.0-c.bin

set DEMO_TEXT=Hello! This is a demonstration of the Kokoro Text to Speech system.
set DEMO_OUTPUT=demo.wav
set DEMO_VOICE=af_sarah

REM Check for curl (available on Windows 10+)
where curl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: curl not found. Please install curl or download files manually.
    echo See README.md for manual download instructions.
    pause
    exit /b 1
)

REM Download model file
echo Step 1: Downloading ONNX Model
echo   File: %MODEL_FILE% (~300MB)
if exist %MODEL_FILE% (
    echo   File already exists, skipping download
) else (
    echo   Downloading...
    curl -L -O --progress-bar %MODEL_BASE_URL%/%MODEL_FILE%
    if !ERRORLEVEL! neq 0 (
        echo   Error: Download failed
        pause
        exit /b 1
    )
    echo   Downloaded successfully
)
echo.

REM Download voices file
echo Step 2: Downloading Voices File
echo   File: %VOICES_FILE% (~80MB)
if exist %VOICES_FILE% (
    echo   File already exists, skipping download
) else (
    echo   Downloading...
    curl -L -O --progress-bar %MODEL_BASE_URL%/%VOICES_FILE%
    if !ERRORLEVEL! neq 0 (
        echo   Error: Download failed
        pause
        exit /b 1
    )
    echo   Downloaded successfully
)
echo.

REM Check for Python
where python >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: python not found. Required for voice file conversion.
    echo Please install Python 3 and try again.
    pause
    exit /b 1
)

REM Convert voices to C format
echo Step 3: Converting Voices to C Format
if exist %VOICES_C_FILE% (
    echo   File already exists, skipping conversion
) else (
    if exist ..\..\scripts\convert_voices.py (
        echo   Converting...
        python ..\..\scripts\convert_voices.py %VOICES_FILE% %VOICES_C_FILE%
        if !ERRORLEVEL! neq 0 (
            echo   Error: Conversion failed
            pause
            exit /b 1
        )
        echo   Converted successfully
    ) else (
        echo   Error: Conversion script not found
        pause
        exit /b 1
    )
)
echo.

REM Check if example binary exists
set EXAMPLE_BIN=..\build\Release\kokoro_example.exe
if not exist %EXAMPLE_BIN% (
    set EXAMPLE_BIN=..\build\kokoro_example.exe
)

if not exist %EXAMPLE_BIN% (
    echo Warning: Example binary not found. Build the project first:
    echo   cd .. ^&^& mkdir build ^&^& cd build ^&^& cmake .. ^&^& cmake --build .
    echo.
    echo Then run this script again to generate the demo.
    goto :summary
)

REM Generate demo WAV file
echo Step 4: Generating Demo Audio
if exist %DEMO_OUTPUT% (
    echo   File already exists
    echo   Delete it to regenerate
) else (
    echo   Generating...
    %EXAMPLE_BIN% %MODEL_FILE% %VOICES_C_FILE% %DEMO_OUTPUT% %DEMO_VOICE% "%DEMO_TEXT%"
    if !ERRORLEVEL! equ 0 (
        echo   Generated successfully
    ) else (
        echo   Error: Demo generation failed
    )
)
echo.

:summary
echo Setup Complete!
echo.
echo Downloaded Files:
if exist %MODEL_FILE% (echo   [OK] %MODEL_FILE%) else (echo   [--] %MODEL_FILE%)
if exist %VOICES_FILE% (echo   [OK] %VOICES_FILE%) else (echo   [--] %VOICES_FILE%)
if exist %VOICES_C_FILE% (echo   [OK] %VOICES_C_FILE%) else (echo   [--] %VOICES_C_FILE%)
if exist %DEMO_OUTPUT% (echo   [OK] %DEMO_OUTPUT%) else (echo   [--] %DEMO_OUTPUT%)
echo.
echo Next Steps:
echo   1. Build the project (if not already done)
echo   2. Run the basic example
echo   3. See README.md for more information
echo.

pause
