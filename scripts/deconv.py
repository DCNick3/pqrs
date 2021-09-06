
import base64
import sys

# ImageData {data: Uint8ClampedArray(1228800), width: 480, height: 640} 

w = 480
h = 640
data = [ int(x) for x in str(base64.b64decode(input()), 'utf8').split(',') ]
print(len(data))
assert len(data) == w * h * 4

def out(d):
    sys.stdout.buffer.write(d)


out(bytes(f'P6 {w} {h}\n{255}\n', 'utf-8'))
out(bytes(data[3::4]))
