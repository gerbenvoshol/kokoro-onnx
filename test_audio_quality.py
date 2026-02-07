#!/usr/bin/env python3
"""Test audio quality by comparing Python and C implementations."""

import soundfile as sf
import numpy as np

def analyze_audio(filename):
    """Analyze audio file and print statistics."""
    data, samplerate = sf.read(filename)
    
    print(f"\n{filename}:")
    print(f"  Sample rate: {samplerate} Hz")
    print(f"  Samples: {len(data)}")
    print(f"  Duration: {len(data) / samplerate:.2f} seconds")
    print(f"  Min value: {data.min():.6f}")
    print(f"  Max value: {data.max():.6f}")
    print(f"  Mean: {data.mean():.6f}")
    print(f"  Std dev: {data.std():.6f}")
    print(f"  RMS: {np.sqrt(np.mean(data**2)):.6f}")
    
    # Check for clipping
    clipped = np.sum(np.abs(data) > 0.99)
    if clipped > 0:
        print(f"  WARNING: {clipped} samples clipped ({100*clipped/len(data):.2f}%)")
    
    # Check for silence
    silence = np.sum(np.abs(data) < 0.01)
    print(f"  Near-silence samples: {silence} ({100*silence/len(data):.2f}%)")
    
    return data, samplerate

def main():
    print("=" * 60)
    print("Audio Quality Analysis")
    print("=" * 60)
    
    # Analyze Python output
    py_data, py_sr = analyze_audio("audio.wav")
    
    # Analyze C output
    c_data, c_sr = analyze_audio("c/build/audio_c.wav")
    
    # Compare
    print("\n" + "=" * 60)
    print("Comparison:")
    print("=" * 60)
    
    if len(py_data) != len(c_data):
        print(f"  Length mismatch: Python has {len(py_data)} samples, C has {len(c_data)} samples")
        print(f"  Difference: {abs(len(py_data) - len(c_data))} samples ({abs(len(py_data) - len(c_data)) / py_sr:.3f} seconds)")
    else:
        print(f"  ✓ Same length: {len(py_data)} samples")
        
        # Calculate difference
        diff = py_data - c_data
        print(f"  Max difference: {np.abs(diff).max():.6f}")
        print(f"  Mean difference: {diff.mean():.6f}")
        print(f"  RMS difference: {np.sqrt(np.mean(diff**2)):.6f}")
        
        # Calculate correlation
        if np.std(py_data) > 0 and np.std(c_data) > 0:
            correlation = np.corrcoef(py_data, c_data)[0, 1]
            print(f"  Correlation: {correlation:.6f}")
            if correlation < 0.95:
                print(f"  WARNING: Low correlation suggests different audio content!")
    
    print("\n" + "=" * 60)

if __name__ == "__main__":
    main()
