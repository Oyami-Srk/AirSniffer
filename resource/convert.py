#!python3
from PIL import Image

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)


def convert(fn: str) -> str:
    im = Image.open(fn)
    name = fn.split('.')[0]
    pixels = list(im.getdata())
    p = [pixels[i * 8:(i + 1) * 8] for i in range(im.height * im.width // 8)]
    a = [
        ''.join([str(y) for y in [0 if x == BLACK else 1 for x in u]][::-1])
        for u in p
    ]
    h = ['0x' + hex(int(v, 2))[2:].zfill(2).upper() for v in a]
    return """const uint8_t %s[] = {%s};""" % (name, ', '.join(h))


if __name__ == "__main__":
    import sys
    #  print(convert(sys.argv[1]))
    for fn in sys.argv[1:]:
        print(convert(fn))
