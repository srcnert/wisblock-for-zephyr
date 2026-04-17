import wave
import struct

# 1. PASTE YOUR DATA HERE
raw_data_block = """
<AUDIO DATA>
"""

samples = [int(x.strip()) for x in raw_data_block.replace('\n', '').split(',') if x.strip()]

sample_rate = 16000

with wave.open('app/basic/microphone/script/mic_test.wav', 'w') as f:
    f.setnchannels(1)       # Mono
    f.setsampwidth(2)       # 2 bytes per sample (16-bit)
    f.setframerate(sample_rate)

    # Write the raw processed samples
    # struct.pack('<h') ensures it is written as little-endian 16-bit signed integer
    for sample in samples:
        f.writeframes(struct.pack('<h', sample))

print(f"Generated mic_test.wav with {len(samples)} samples.")
print(f"Duration: {len(samples)/sample_rate:.4f} seconds")
