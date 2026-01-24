#!/usr/bin/env python3
"""
Convert voices file from NumPy .npz format to C binary format

Usage:
    python scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin
"""

import sys
import struct
import numpy as np


def convert_voices(input_path, output_path):
    """Convert NumPy voices file to C binary format"""
    
    print(f"Loading voices from {input_path}...")
    voices = np.load(input_path, allow_pickle=True)
    
    # Get voice names and embeddings
    voice_items = list(voices.items())
    num_voices = len(voice_items)
    
    if num_voices == 0:
        print("Error: No voices found in file")
        return 1
    
    # Get embedding size from first voice
    first_voice_name, first_embedding = voice_items[0]
    if len(first_embedding.shape) == 1:
        # Single embedding
        embedding_size = len(first_embedding)
    else:
        # Multiple embeddings (one per sequence length)
        embedding_size = first_embedding.shape[1] if len(first_embedding.shape) > 1 else len(first_embedding)
        # For simplicity, we'll flatten the embeddings
        # In practice, the C code should handle multi-dimensional embeddings
        print(f"Warning: Voice has shape {first_embedding.shape}, will flatten")
        embedding_size = first_embedding.size
    
    print(f"Found {num_voices} voices")
    print(f"Embedding size: {embedding_size}")
    
    # Write binary file
    print(f"Writing to {output_path}...")
    with open(output_path, 'wb') as f:
        # Write header
        f.write(struct.pack('I', num_voices))  # uint32_t num_voices
        f.write(struct.pack('I', embedding_size))  # uint32_t embedding_size
        
        # Write each voice
        for voice_name, embedding in voice_items:
            # Flatten if needed
            if len(embedding.shape) > 1:
                embedding = embedding.flatten()
            
            # Ensure we have the right size
            if len(embedding) != embedding_size:
                print(f"Warning: Voice {voice_name} has size {len(embedding)}, expected {embedding_size}")
                # Pad or truncate
                if len(embedding) < embedding_size:
                    embedding = np.pad(embedding, (0, embedding_size - len(embedding)))
                else:
                    embedding = embedding[:embedding_size]
            
            # Write voice name
            voice_name_bytes = voice_name.encode('utf-8')
            f.write(struct.pack('I', len(voice_name_bytes)))  # uint32_t name_length
            f.write(voice_name_bytes)  # voice name
            
            # Write embeddings as float32
            embedding_float32 = embedding.astype(np.float32)
            f.write(embedding_float32.tobytes())
            
            print(f"  ✓ {voice_name}")
    
    print(f"Done! Converted {num_voices} voices to {output_path}")
    return 0


def main():
    if len(sys.argv) != 3:
        print("Usage: python convert_voices.py <input.bin> <output.bin>")
        print()
        print("Example:")
        print("  python scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin")
        return 1
    
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    
    try:
        return convert_voices(input_path, output_path)
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
