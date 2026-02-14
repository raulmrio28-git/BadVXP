from PIL import Image
import os
import sys

folder = sys.argv[1]
out = sys.argv[2]

#fwork = open(file_debug, "wb")

for directory, subdirectories, files in os.walk(folder):
    for file in files:
        fn = os.path.join(directory, file)
        img = Image.open(fn)
        thresh = 128
        fnx = lambda x : 255 if x > thresh else 0
        r = img.convert('L').point(fnx, mode='1')
        #fwork.write(r.tobytes())
        fn2 = os.path.join(out, file)
        r.save(fn2)

#fwork.close()