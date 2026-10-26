"""Capturer les preuves originales du bloc personnages, sans redistribution."""
import json
from pathlib import Path
import shutil
from audit_combat_binary import capture_disassembly, describe_source, ROOT, OUT


def main():
    """Extraire overlays locaux et routines pertinentes sous build/audit."""
    objdump = shutil.which('objdump')
    if not objdump:
        raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    original = ROOT/'data/compiled program/TERA.000'
    com = ROOT/'data/compiled program/TERA.COM'
    overlays = original.read_bytes()
    paths = {}
    for name,start,end in [('0010',0x1000,0x2200),('0022',0x2200,0x2E00)]:
        path = OUT/f'OVL_{name}.bin'
        path.write_bytes(overlays[start:end])
        paths[name] = path
    regions = [('creation_roster',paths['0010'],0xBA21,0xC597,0xC845),
               ('creation_equipment',paths['0010'],0xBA21,0xCB91,0xCBC1),
               ('creation_hero',paths['0022'],0xBA21,0xBA28,0xC54D),
               ('creation_restore',com,0x100,0x4C6C,0x4D46),
               ('creation_digit',com,0x100,0x4418,0x4473),
               ('creation_name',com,0x100,0x42AC,0x4418),
               ('creation_card',com,0x100,0x49FD,0x4AAF)]
    manifest = dict(sources={},regions=[])
    for source in (com,original,ROOT/'data/compiled program/NOMPERS',
                   ROOT/'data/source code/OVL_0010.PAS',ROOT/'data/source code/OVL_0022.PAS',
                   ROOT/'data/source code/OVL_00DB.PAS'):
        manifest['sources'][source.relative_to(ROOT).as_posix()] = describe_source(source)
    for name,source,origin,start,end in regions:
        capture_disassembly(objdump,source,origin,start,end,OUT/f'{name}.asm')
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'creation_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves personnages ecrites dans',OUT)


if __name__=='__main__':
    main()
