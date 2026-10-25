# Render XPS3-owned package tile/splash images.
# The PNGs are committed under uwp/package/Assets; rerun this only when the sizes change.
# Usage: python3 make_assets.py   (needs Pillow)
import os
from PIL import Image, ImageDraw

here = os.path.dirname(os.path.abspath(__file__))
background = (0x1A, 0x1A, 0x1F, 255)  # matches BackgroundColor in AppxManifest.xml
accent = (0x55, 0xAA, 0xFF, 255)

def logo(size):
    scale = 4
    image = Image.new('RGBA', (size * scale, size * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    margin = max(2, size // 6) * scale
    width = max(2, size // 11) * scale
    last = size * scale - margin
    draw.line((margin, margin, last, last), fill=accent, width=width)
    draw.line((last, margin, margin, last), fill=accent, width=width)
    return image.resize((size, size), Image.Resampling.LANCZOS)

# name: (width, height, logo size); tiles keep the logo on the tile background colour.
for name, (w, h, s) in {'StoreLogo': (50, 50, 50), 'Square44x44Logo': (44, 44, 44),
                        'Square150x150Logo': (150, 150, 104), 'Wide310x150Logo': (310, 150, 104),
                        'SplashScreen': (620, 300, 200)}.items():
    img = Image.new('RGBA', (w, h), background if name in ('Wide310x150Logo', 'SplashScreen', 'Square150x150Logo') else (0, 0, 0, 0))
    img.alpha_composite(logo(s), ((w - s) // 2, (h - s) // 2))
    img.save(os.path.join(here, 'Assets', name + '.png'), optimize=True)
    print(name, w, h)
