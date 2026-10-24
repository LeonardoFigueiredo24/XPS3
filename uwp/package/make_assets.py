# Render the package tile/splash images from the official RPCS3 logo (rpcs3/rpcs3.svg).
# The PNGs are committed under uwp/package/Assets; rerun this only when the logo or sizes change.
# Usage: python3 make_assets.py   (needs cairosvg and Pillow)
import io, os
import cairosvg
from PIL import Image

here = os.path.dirname(os.path.abspath(__file__))
svg = os.path.join(here, '..', '..', 'rpcs3', 'rpcs3.svg')
background = (0x1A, 0x1A, 0x1F, 255)  # matches BackgroundColor in AppxManifest.xml

def logo(size):
    png = cairosvg.svg2png(url=svg, output_width=size * 4, output_height=size * 4)
    return Image.open(io.BytesIO(png)).convert('RGBA').resize((size, size), Image.LANCZOS)

# name: (width, height, logo size); tiles keep the logo on the tile background colour.
for name, (w, h, s) in {'StoreLogo': (50, 50, 50), 'Square44x44Logo': (44, 44, 44),
                        'Square150x150Logo': (150, 150, 104), 'Wide310x150Logo': (310, 150, 104),
                        'SplashScreen': (620, 300, 200)}.items():
    img = Image.new('RGBA', (w, h), background if name in ('Wide310x150Logo', 'SplashScreen', 'Square150x150Logo') else (0, 0, 0, 0))
    img.alpha_composite(logo(s), ((w - s) // 2, (h - s) // 2))
    img.save(os.path.join(here, 'Assets', name + '.png'), optimize=True)
    print(name, w, h)
