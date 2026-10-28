"""Lossless Original assets pipeline. Pillow is used only by developer commands.

extract reads the fifteen DATA files; generate reads only editable PNG/JSON.
verify reconstructs every source byte (including Pascal tails/CGA padding).
No historical Python extractor is imported: its per-black-pixel alpha is wrong.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from PIL import Image

TABLES = {'VERBES': (56, 19), 'OBJETS': (25, 16), 'NOMON': (22, 40),
          'NOMPERS': (27, 10), 'MESSAGE': (102, 40), 'MESSAGE2': (33, 40),
          'MESSAGE3': (33, 40)}
BACKGROUNDS = ['ASTRON', 'BANNIERE', 'FACADE', 'KRONO', 'MAISONJ', 'MAISONS']
PALETTE = [0, 0, 0, 85, 255, 255, 255, 85, 255, 255, 255, 255]


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8'))


def scales(i):
    # OVL_00DB successive IFs, identical to resources.c (not resampling).
    h, v = 1, 1
    if i > 50:
        h = 2
        if i == 59:
            h, v = 1, 2
        if i > 59:
            h, v = 2, 3 if i > 74 else 2
        if i > 76:
            h, v = 3, 1
        if i == 81:
            v = 3
        if i == 82:
            h, v = 2, 6
    return h, v


def png(path, pixels, width, height):
    image = Image.frombytes('P', (width, height), bytes(pixels))
    image.putpalette(PALETTE + [0] * (768 - len(PALETTE)))
    image.save(path, optimize=False)


def indices(path, width, height):
    with Image.open(path) as image:
        if image.mode != 'P' or image.size != (width, height):
            raise ValueError(f'{path}: expected indexed PNG {width}x{height}')
        if image.getpalette()[:12] != PALETTE or 'transparency' in image.info:
            raise ValueError(f'{path}: preserve the four CGA palette entries; no alpha')
        pixels = list(image.tobytes())
    if max(pixels) > 3:
        raise ValueError(f'{path}: indices must be 0..3')
    return pixels


def unpack(value):
    return [(value >> shift) & 3 for shift in (6, 4, 2, 0)]


def pack(pixels):
    return sum(value << shift for value, shift in zip(pixels, (6, 4, 2, 0)))


def extract(source, out):
    if out.exists():
        raise ValueError('Extraction destination already exists; refusing to overwrite edits')
    out.mkdir(parents=True)
    manifest = {'schema': 1, 'provenance': 'TERA, Loriciels 1986; DATA; local conversion, original copyright unchanged',
                'palette_rgb': [PALETTE[i:i+3] for i in range(0, 12, 3)],
                'mask_rule': 'skip a complete four-pixel CGA byte only when all four indices are zero; black within mixed bytes stays opaque',
                'sources': {}}
    for name in [*TABLES, 'VALMON', 'CRAPIM.IMA', *[s + '.IMA' for s in BACKGROUNDS]]:
        data = (source / name).read_bytes()
        manifest['sources'][name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    for name, (count, stride) in TABLES.items():
        data = (source / name).read_bytes()
        assert len(data) == count * stride
        records = []
        for i in range(count):
            row = data[i*stride:(i+1)*stride]
            assert row[0] < stride and 0 not in row[1:1+row[0]]
            records.append({'id': i+1, 'text': row[1:1+row[0]].decode('cp437'),
                            'unused_tail': list(row[1+row[0]:])})
        write_json(out / (name + '.json'), {'encoding': 'CP437', 'stride': stride, 'records': records})
    values = struct.unpack('<132h', (source / 'VALMON').read_bytes())
    write_json(out / 'VALMON.json', {'columns': ['original_column_' + str(i+1) for i in range(6)],
                                   'records': [{'id': i+1, 'values': list(values[i*6:i*6+6])} for i in range(22)]})
    for name in BACKGROUNDS:
        data = (source / (name + '.IMA')).read_bytes()
        assert len(data) == 16384
        pixels = [p for y in range(200) for x in range(80)
                  for p in unpack(data[(y % 2)*8192 + (y//2)*80 + x])]
        png(out / (name + '.png'), pixels, 320, 200)
        write_json(out / (name + '.json'), {'width': 320, 'height': 200, 'source': name + '.IMA',
                                          'unused_bank_tails': [list(data[8000:8192]), list(data[16192:])]})
    data = (source / 'CRAPIM.IMA').read_bytes()
    offset, sprites = 0, []
    (out / 'sprites').mkdir()
    (out / 'masks').mkdir()
    for i in range(1, 83):
        raw_w, raw_h = data[offset:offset+2]
        h, v = scales(i)
        row = raw_w*h
        plane = raw_w*raw_h*h*v//2
        if raw_h % 2 and i < 59:
            plane = row*v*((raw_h-1)//2)
        width, height = row*4, 2*plane//row
        pixels = [p for y in range(height) for x in range(row)
                  for p in unpack(data[offset+2+2*((y//2)*row+x)+y%2])]
        filename = f'sprites/{i:03}.png'
        png(out / filename, pixels, width, height)
        mask = [255 if any(pixels[j:j+4]) else 0 for j in range(0, len(pixels), 4) for _ in range(4)]
        Image.frombytes('L', (width, height), bytes(mask)).save(out / f'masks/{i:03}.png')
        sprites.append({'id': i, 'file': filename, 'raw_width_bytes': raw_w, 'raw_height': raw_h,
                        'hscale': h, 'vscale': v, 'width': width, 'height': height, 'source_offset': offset})
        offset += 2 + 2*plane
    assert offset == len(data) == 23206
    write_json(out / 'sprites.json', {'source': 'CRAPIM.IMA', 'sprites': sprites})
    write_json(out / 'manifest.json', manifest)


def reconstruct(root):
    result = {}
    for name, (count, stride) in TABLES.items():
        table = read_json(root / (name + '.json'))
        assert table['stride'] == stride and len(table['records']) == count
        data = bytearray()
        for i, row in enumerate(table['records'], 1):
            text = row['text'].encode('cp437')
            if row['id'] != i or len(text) >= stride or b'\0' in text:
                raise ValueError(f'{name} record {i}: invalid ID/CP437 length/NUL')
            # Preserve unused original bytes; truncate/zero extend after a text edit.
            tail = bytes(row['unused_tail'])[:stride-1-len(text)]
            data.extend(bytes([len(text)]) + text + tail.ljust(stride-1-len(text), b'\0'))
        result[name] = bytes(data)
    rows = read_json(root / 'VALMON.json')['records']
    assert len(rows) == 22 and all(r['id'] == i+1 and len(r['values']) == 6 for i, r in enumerate(rows))
    result['VALMON'] = struct.pack('<132h', *(v for r in rows for v in r['values']))
    for name in BACKGROUNDS:
        pixels = indices(root / (name + '.png'), 320, 200)
        meta = read_json(root / (name + '.json'))
        data = bytearray(16384)
        data[8000:8192], data[16192:] = map(bytes, meta['unused_bank_tails'])
        assert len(data) == 16384
        for y in range(200):
            for x in range(80):
                data[(y%2)*8192+(y//2)*80+x] = pack(pixels[y*320+x*4:y*320+x*4+4])
        result[name + '.IMA'] = bytes(data)
    sprites = read_json(root / 'sprites.json')['sprites']
    assert len(sprites) == 82
    data = bytearray()
    for i, s in enumerate(sprites, 1):
        assert s['id'] == i and s['source_offset'] == len(data)
        h, v = scales(i)
        rw, rh = s['raw_width_bytes'], s['raw_height']
        row = rw*h
        plane = row*rh*v//2 if not (rh%2 and i < 59) else row*v*((rh-1)//2)
        width, height = row*4, 2*plane//row
        assert (s['width'], s['height'], s['hscale'], s['vscale']) == (width, height, h, v)
        pixels = indices(root / s['file'], width, height)
        data.extend(bytes([rw, rh]))
        for y in range(0, height, 2):
            for x in range(row):
                for dy in (0, 1):
                    start = (y+dy)*width+x*4
                    data.append(pack(pixels[start:start+4]))
    assert len(data) == 23206
    result['CRAPIM.IMA'] = bytes(data)
    return result


def generate(root, output):
    resources = reconstruct(root)
    lines = ['/* Generated by original_resources.py; TERA DATA, Loriciels 1986. */',
             '#include "embedded.h"', '#include <string.h>']
    entries = []
    for i, (name, data) in enumerate(sorted(resources.items())):
        symbol = f'resource_{i}'
        lines.append(f'static const unsigned char {symbol}[]={{')
        lines.extend(','.join(str(v) for v in data[j:j+24]) + ',' for j in range(0, len(data), 24))
        lines.append('};')
        entries.append(f'    {{"{name}",{symbol},sizeof {symbol}}},')
    lines.extend(['static const struct {const char *name;const unsigned char *bytes;size_t size;} entries[]={',
                  *entries, '};', 'const unsigned char *tera_embedded_resource(const char *name,size_t *size)',
                  '{', '    size_t i;', '    for(i=0;i<sizeof entries/sizeof entries[0];++i)',
                  '        if(!strcmp(name,entries[i].name)) {*size=entries[i].size;return entries[i].bytes;}',
                  '    *size=0;return NULL;', '}'])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines) + '\n', encoding='ascii')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['extract', 'generate', 'verify'])
    parser.add_argument('--source', type=Path)
    parser.add_argument('--editable', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.command == 'extract':
        extract(args.source, args.editable)
    elif args.command == 'generate':
        generate(args.editable, args.output)
    else:
        for name, data in reconstruct(args.editable).items():
            assert data == (args.source / name).read_bytes(), name
            expected = read_json(args.editable / 'manifest.json')['sources'][name]
            assert hashlib.sha256(data).hexdigest() == expected['sha256']
            print(f'{name}: {len(data)} bytes, SHA256 {expected["sha256"]}, byte-exact')
        print('PASS: 15 sources, 7 CP437 tables, 132 signed values, 82 sprites, 6 backgrounds; all bytes identical')


if __name__ == '__main__':
    main()
