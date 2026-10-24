# Lay out the UWP package directory: exe (with the AppContainer PE flag set), manifest,
# tile logos and the bundled test programs.
# Usage: make_layout.py <rpcs3_uwp.exe> <rpcs3 source dir> <layout dir>
import os, shutil, struct, sys

exe, src, out = sys.argv[1:4]
here = os.path.dirname(os.path.abspath(__file__))

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
# Tile and splash images: the RPCS3 logo, rendered by make_assets.py.
for f in os.listdir(os.path.join(here, 'Assets')):
    shutil.copy(os.path.join(here, 'Assets', f), os.path.join(out, 'Assets'))

# Overlay UI icons; the shell copies them into LocalState/rpcs3/Icons on first start.
shutil.copytree(os.path.join(src, 'bin', 'Icons'), os.path.join(out, 'Icons'))

# Overlay and menu font: DejaVu Sans (free license, see fonts/DejaVu-LICENSE.txt).
os.makedirs(os.path.join(out, 'fonts'))
for f in ('DejaVuSans.ttf', 'DejaVu-LICENSE.txt'):
    shutil.copy(os.path.join(src, 'uwp', 'package', 'fonts', f), os.path.join(out, 'fonts'))

test_dir = os.path.join(src, 'bin', 'test')
for f in os.listdir(test_dir):
    if os.path.isfile(os.path.join(test_dir, f)):
        shutil.copy(os.path.join(test_dir, f), os.path.join(out, 'test'))
# Stress tests and benchmarks for LocalState/autotest "bench" (uwp/bench/README.md).
for d in ('stress', 'stress-rsx'):
    shutil.copytree(os.path.join(src, 'bin', 'test', d), os.path.join(out, 'test', d))
print('layout ready:', out)
