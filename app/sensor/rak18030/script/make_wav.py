import wave
import struct

# 1. PASTE YOUR DATA HERE
raw_data_block = """
<AUDIO DATA>
"""

sample_rate = 16000

with wave.open('mic_test.wav', 'w') as f:
    gain = 4 # If you are trying to determine what gain to add to the microphone, you can
             # temporarily increase this above 1 (or below) to find the correct gain
             # this number must be 1 for actual model creation as the audio won't
             # be the same on device if this is not 1!
             # The percentage of clipped samples should be kept low (<1%)
             # If you aren't getting the number of samples you expect or much lower (eg 2 when you should have 8)
             # try increasing the gain here, and when you find the correct number update it on the device

    samples = [int(gain*int(x.strip())) for x in raw_data_block.replace('\n', '').split(',') if x.strip()]

    print("Verification of Gain settings:")
    print("    Maximum found:", max(samples), "  32767 is maximum allowable")
    print("    Minimum found:", min(samples), " -32767 is minimum allowed")

    # Set the parameters for the WAV file
    f.setnchannels(1)       # Mono
    f.setsampwidth(2)       # 2 bytes per sample (16-bit)
    f.setframerate(sample_rate)

    # Write the raw processed samples
    # struct.pack('<h') ensures it is written as little-endian 16-bit signed integer
    for sample in samples:
        f.writeframes(struct.pack('<h', sample))

print(f"Generated mic_test.wav with {len(samples)} samples.")
print(f"Duration: {len(samples)/sample_rate:.4f} seconds")
