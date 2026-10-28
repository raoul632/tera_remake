"""Developer-only end-to-end tests; no original data required after conversion."""
import importlib.util
import shutil
import sys
import tempfile
from pathlib import Path
from PIL import Image

PROJECT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('pipeline', PROJECT / 'tools/original_resources.py')
pipeline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pipeline)


def main():
    editable = Path(sys.argv[1])
    build = PROJECT / 'build'
    build.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=build) as temp:
        root = Path(temp)
        assets = root / 'editable'
        shutil.copytree(editable, assets)
        original = pipeline.reconstruct(assets)
        pipeline.generate(assets, root / 'one.c')
        pipeline.generate(assets, root / 'two.c')
        assert (root / 'one.c').read_bytes() == (root / 'two.c').read_bytes()
        for s in pipeline.read_json(assets / 'sprites.json')['sprites']:
            pixels = pipeline.indices(assets / s['file'], s['width'], s['height'])
            with Image.open(assets / f'masks/{s["id"]:03}.png') as mask:
                expected = bytes(255 if any(pixels[j:j+4]) else 0
                                 for j in range(0, len(pixels), 4) for _ in range(4))
                assert mask.tobytes() == expected
        table = pipeline.read_json(assets / 'MESSAGE.json')
        table['records'][0]['text'] = 'Édité CP437'
        pipeline.write_json(assets / 'MESSAGE.json', table)
        sprite = assets / 'sprites/001.png'
        with Image.open(sprite) as im:
            old = im.getpixel((0, 0))
            im.putpixel((0, 0), (old+1) % 4)
            im.save(sprite)
        changed = pipeline.reconstruct(assets)
        assert 'Édité CP437'.encode('cp437') in changed['MESSAGE']
        assert changed['CRAPIM.IMA'][2] != original['CRAPIM.IMA'][2]
        assert all(changed[name] == data for name, data in original.items()
                   if name not in ('MESSAGE', 'CRAPIM.IMA'))
        pipeline.generate(assets, root / 'edited.c')
        assert (root / 'edited.c').read_bytes() != (root / 'one.c').read_bytes()
        with Image.open(sprite) as im:
            im.convert('RGBA').save(sprite)
        try:
            pipeline.reconstruct(assets)
        except ValueError:
            pass
        else:
            raise AssertionError('RGBA input must not silently reinterpret CGA indices')
    print('PASS: deterministic generation; 82 exact byte masks; JSON CP437 and PNG edits reach generated data; RGBA rejected')


if __name__ == '__main__':
    main()
