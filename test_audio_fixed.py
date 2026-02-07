import soundfile as sf
import numpy as np

def analyze_audio(filename):
    data, samplerate = sf.read(filename)
    duration = len(data) / samplerate
    rms = np.sqrt(np.mean(data**2))
    print(f"{filename:30s}: {len(data):6d} samples, {duration:.2f}s, RMS={rms:.4f}")
    return data, samplerate

print("Audio Analysis:")
print("=" * 80)
py_data, py_sr = analyze_audio("audio.wav")
c_old_data, c_old_sr = analyze_audio("c/build/audio_c.wav")
c_new_data, c_new_sr = analyze_audio("c/build/audio_c_fixed.wav")

print("\nImprovements:")
print(f"  Old C: {len(c_old_data)} samples")
print(f"  New C: {len(c_new_data)} samples (+{len(c_new_data) - len(c_old_data)} samples, +{100*(len(c_new_data)-len(c_old_data))/len(c_old_data):.1f}%)")
print(f"  Python: {len(py_data)} samples")
print(f"  Still missing: {len(py_data) - len(c_new_data)} samples ({100*(len(py_data)-len(c_new_data))/len(py_data):.1f}%)")
