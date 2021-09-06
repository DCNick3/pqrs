
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
def get(j, i):
    coord = j * (w * 4) + i * 4
    return bytes(data[coord:coord+4])

out(bytes(f'P6 {w} {h}\n{255}\n', 'utf-8'))
for j in range(0, h):
    for i in range(0, w):
        out(get(j, i)[:3])
