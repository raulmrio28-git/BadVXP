#!/usr/bin/python3
import argparse
from re import L
import os
from PIL import Image
import struct
import multiprocessing
import lzss
prev_buffer_xor = None

log_f = open("log.txt", "w")

def bit_packing(runs, max_bits):
    # We're going to do something stupid
    # Use a string to store the bits
    temp_int = ""
    data = bytearray()

    for run in runs:
        my_run = run
        for i in range(max_bits):
            temp_int += str((my_run>>(max_bits-i-1)) & 1)

    if len(temp_int) % 32 != 0:
        temp_int += "0" * (32 - len(temp_int) % 32)

    # Encode it into bytes
    for i in range(0, len(temp_int), 32):
        data.append(int(temp_int[i+24:i+32], 2))
        data.append(int(temp_int[i+16:i+24], 2))
        data.append(int(temp_int[i+8:i+16], 2))
        data.append(int(temp_int[i:i+8], 2))

    return data

def encode_rle(image, bpp, xor_frame = False):
    global prev_buffer_xor
   
    if image.mode != "RGB":
        image = image.convert("RGB")
    
    if bpp == 1:
        palette = Image.new("P", (1, 1))
        palette.putpalette([0, 0, 0, 255, 255, 255])
    elif bpp == 2:
        palette = Image.new("P", (1, 1))
        palette.putpalette([0, 0, 0, 96, 96, 96, 192, 192, 192, 255, 255, 255])

    newimage = image.quantize(palette=palette, dither=Image.FLOYDSTEINBERG).convert('L')

    img_data = bytearray(newimage.tobytes())

    for a in range(len(img_data)):
        if bpp == 1:
            img_data[a] = img_data[a] >> 7
        elif bpp == 2:
            img_data[a] = img_data[a] >> 6

    a = 0
    temp_data = []
    if xor_frame and prev_buffer_xor:
        for a in range(len(img_data)):
            temp_data.append(prev_buffer_xor[a] ^ img_data[a])
    else:
        for a in range(len(img_data)):
            temp_data.append(img_data[a])

    img_out = bytearray()

    b = 0
    first_pixel = temp_data[b]

    runs = []

    max_bits = 0
    
    while b < len(temp_data):
        pixel = temp_data[b]
        count = 0
        while b + count < len(temp_data) and temp_data[b + count] == pixel:
            count += 1
            
        runs.append(count) # during decode, it will be inverted
        bits = count.bit_length()
        max_bits = max(max_bits, bits)
        b += count

    # we now need to encode to bitstring
    bp = bit_packing(runs, max_bits)
    bp_byteslike = bytes(bp)
    bp_red = lzss.compress(bp_byteslike)
    is_compressed_bitredundant = 0
    if len(bp_red) < len(bp):
        len_var = len(bp_red) | (len(bp) << 16)
        bp = bp_red
        is_compressed_bitredundant = 1
    else:
        len_var = len(bp)
    magic = ((max_bits<<2)|(is_compressed_bitredundant<<1)|first_pixel)&0xff
    img_out.append(magic)

    img_out.extend(bp)
    
    log_f.write(f"{len(bp)}, {len(bp_red)}\n")

    if prev_buffer_xor is None:
        prev_buffer_xor = []
        for b in range(len(img_data)):
            prev_buffer_xor.append(img_data[b])
    else:
        for b in range(len(img_data)):
            prev_buffer_xor[b] = img_data[b]

    return len_var, img_out

def encode_worker(args):
    """Worker function for multiprocessing"""
    img_path, bpp, xor_frame, prev_buffer_data = args
    i = Image.open(img_path)

    global prev_buffer_xor
    prev_buffer_xor = prev_buffer_data
    
    try:
        ci = encode_rle(i, bpp, xor_frame)
        return ci
    except Exception as e:
        print(f"Error encoding {img_path}: {e}")
        return None

if __name__ == "__main__":
    ap = argparse.ArgumentParser("img2pdk_badvxp")
    ap.add_argument("in_folder")
    ap.add_argument("out_file")
    ap.add_argument("--processes", "-p", type=int, default=4, help="Number of processes to use")

    args = ap.parse_args()

    version = 2
    bpp = 1
    out_file = args.out_file
    folder = args.in_folder  
    num_processes = args.processes

    f = open(out_file, "wb")

    img_array = []
    enc_image_array = []
    img_cnt = 0
    hdr_offset = 0
    img_width = 0
    img_height = 0

    for directory, subdirectories, files in os.walk(folder):
        for file in sorted(files):
            fn = os.path.join(directory, file)
            if file.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.gif')):
                img_array.append(fn)

    img_cnt = len(img_array)
    print(f"Found {img_cnt} images to encode")

    f.write(b"Ani!"+ struct.pack("<LLLL", bpp, 240, 320, img_cnt))

    hdr_offset = f.tell() + 4*(img_cnt+1)
    
    enc_img_cnt = 0

    encode_args = []
    prev_buffer_data = None
    
    for i, img_path in enumerate(img_array):
        xor_frame = (i > 0)
        encode_args.append((img_path, bpp, xor_frame, prev_buffer_data.copy() if prev_buffer_data else None))

    with multiprocessing.Pool(processes=num_processes) as pool:
        print(f"Encoding with {num_processes} processes...")

        results = pool.imap(encode_worker, encode_args)

        completed = 0
        for length, ci in results:
            if ci is not None:
                length_p = struct.pack("<L", length)
                f.write(length_p)
                f.write(ci)
                enc_img_cnt += 1
                completed += 1
                print(f"Encoded frame {completed}/{img_cnt} ({completed/img_cnt*100:.1f}%), size: {length&0xffff}")
            else:
                completed += 1
                print(f"Failed to encode frame {completed}/{img_cnt}")

    enc_image_array.append(b"End!")

    f.close()
    print(f"Encoding complete. Output written to {out_file}")
