# Lay out the UWP package directory: exe (with the AppContainer PE flag set), manifest,
# placeholder logos and the bundled test programs.
# Usage: make_layout.py <rpcs3_uwp.exe> <rpcs3 source dir> <layout dir>
import os, shutil, struct, sys, zlib

exe, src, out = sys.argv[1:4]
here = os.path.dirname(os.path.abspath(__file__))

def png(w, h, rgb):
    raw = b''.join(b'\x00' + bytes(rgb) * w for _ in range(h))
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')

shutil.rmtree(out, ignore_errors=True)
os.makedirs(os.path.join(out, 'Assets'))
os.makedirs(os.path.join(out, 'test'))

data = bytearray(open(exe, 'rb').read())
pe = struct.unpack_from('<I', data, 0x3c)[0]
dllchar = pe + 24 + 70  # IMAGE_OPTIONAL_HEADER64.DllCharacteristics
struct.pack_into('<H', data, dllchar, struct.unpack_from('<H', data, dllchar)[0] | 0x1000)  # APPCONTAINER
open(os.path.join(out, 'rpcs3_uwp.exe'), 'wb').write(data)

shutil.copy(os.path.join(here, 'AppxManifest.xml'), out)
shutil.copy(os.path.join(os.path.dirname(exe), 'rpcs3_uwp_importcheck.exe'), out)
# Graphics driver (Mesa Dozen: Vulkan on D3D12) and Microsoft's DXIL signer, if given.
for extra in os.environ.get('UWP_EXTRA_FILES', '').split(os.pathsep):
    if extra:
        shutil.copy(extra, out)
for name, (w, h) in {'StoreLogo': (50, 50), 'Square150x150Logo': (150, 150), 'Square44x44Logo': (44, 44),
                     'Wide310x150Logo': (310, 150), 'SplashScreen': (620, 300)}.items():
    open(os.path.join(out, 'Assets', name + '.png'), 'wb').write(png(w, h, (26, 26, 31)))

# Menu font (the PS3 firmware's fonts are used too once installed).
os.makedirs(os.path.join(out, 'fonts'))
shutil.copy(os.path.join(src, 'bin', 'fonts', 'DejaVuSans.ttf'), os.path.join(out, 'fonts'))

test_dir = os.path.join(src, 'bin', 'test')
for f in os.listdir(test_dir):
    shutil.copy(os.path.join(test_dir, f), os.path.join(out, 'test'))
print('layout ready:', out)
