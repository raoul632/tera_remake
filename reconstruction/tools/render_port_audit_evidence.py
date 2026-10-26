"""Render actual BIN/SDL indexed buffers side by side, with exact byte diffs.

Optional evidence exporter; Pillow is a development dependency only. This
never creates historical pixels: both inputs are captured raster buffers.
"""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-dir', required=True, type=Path)
    parser.add_argument('--sdl-dir', required=True, type=Path)
    parser.add_argument('--output-dir', required=True, type=Path)
    parser.add_argument('--dos-creation', type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    palette = [0, 0, 0, 85, 255, 255, 255, 85, 255, 255, 255, 255] + [0] * 756
    report = []
    for index in (277, 337, 340, 350):
        candidates = list(args.native_dir.glob(f'{index}-*.raw'))
        if len(candidates) != 1:
            raise RuntimeError(f'Expected one native raster for {index}')
        paths = [candidates[0], args.sdl_dir / f'verified-raster-{index}.raw']
        buffers = [p.read_bytes() for p in paths]
        if any(len(b) != 64000 or max(b) > 3 for b in buffers):
            raise ValueError('Expected captured 320x200 CGA indexed pixels')
        difference = sum(a != b for a, b in zip(*buffers))
        canvas = Image.new('RGB', (640, 224), '#202020')
        draw = ImageDraw.Draw(canvas)
        for side, (label, raw) in enumerate(zip(('BIN B800', 'SDL readback'), buffers)):
            pic = Image.frombytes('P', (320, 200), raw)
            pic.putpalette(palette)
            pic.convert('RGB').save(args.output_dir / f'{index}-{side}.png')
            canvas.paste(pic.convert('RGB'), (side * 320, 24))
            draw.text((side * 320 + 4, 6), label, fill='white')
        canvas.save(args.output_dir / f'{index}-comparison.png')
        report.append({'index': index, 'native': str(paths[0]), 'sdl': str(paths[1]),
                       'different_pixels': difference,
                       'sha256': [hashlib.sha256(b).hexdigest() for b in buffers]})
    (args.output_dir / 'pixel-comparison.json').write_text(
        json.dumps(report, indent=2) + '\n', encoding='utf-8')
    if args.dos_creation:
        pictures = [Image.open(args.dos_creation).convert('RGB'),
                    Image.open(args.sdl_dir / 'creation-name.bmp').convert('RGB')]
        if any(p.size != (640, 400) for p in pictures):
            raise ValueError('Creation comparison requires native 640x400 captures')
        canvas = Image.new('RGB', (1280, 424), '#202020')
        draw = ImageDraw.Draw(canvas)
        for side, (label, pic) in enumerate(zip(('DOSBox TERA.COM', 'SDL after correction'), pictures)):
            canvas.paste(pic, (side * 640, 24))
            draw.text((side * 640 + 4, 6), label, fill='white')
        canvas.save(args.output_dir / 'creation-comparison.png')
        a, b = [list(p.getdata()) for p in pictures]
        detail = {'different_pixels_full_screen': sum(x != y for x, y in zip(a, b)),
                  'different_pixels_top_band': sum(a[y*640+x] != b[y*640+x]
                                                    for y in range(16) for x in range(632)),
                  'scope': 'same initial name prompt; worlds not synchronized; full screen includes cursor and generation status'}
        (args.output_dir / 'creation-comparison.json').write_text(
            json.dumps(detail, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
    if any(item['different_pixels'] for item in report):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
