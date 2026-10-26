"""Captures ciblees et provenance des lots coffres/pieges et inventaire."""
import json
import shutil
from audit_combat_binary import ROOT,OUT,capture_disassembly,describe_source


def main():
    objdump=shutil.which('objdump')
    if not objdump:raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    com=ROOT/'data/compiled program/TERA.COM';overlay=ROOT/'data/compiled program/TERA.000'
    regions=[('objects_acquire_drop_transfer_drink',com,0x100,0xAA13,0xAFE8),
        ('objects_character_picker',com,0x100,0x4B55,0x4C6C),
        ('objects_digit_reader',com,0x100,0x4418,0x4473),
        ('objects_limits_restore',com,0x100,0x4C6C,0x4D46),
        ('objects_generic_drop_drink',com,0x100,0xB2A0,0xB2F7),
        ('objects_generic_give',com,0x100,0xB1FA,0xB215),
        ('objects_explosion',com,0x100,0x6D32,0x6DC4),
        ('objects_trap_sound',com,0x100,0x6121,0x6179)]
    raw=overlay.read_bytes()
    for name,start,end,first,last in [('objects_chest_zero',0x2E00,0x4900,0xCC09,0xD2AA),
                                      ('objects_chest_levels',0x4900,0x6900,0xCEAB,0xD54A)]:
        path=OUT/f'{name}.bin';path.write_bytes(raw[start:end])
        regions.append((name,path,0xBA21,first,last))
    sources=[com,overlay,ROOT/'data/compiled program/OBJETS',ROOT/'data/compiled program/VERBES',
        ROOT/'data/compiled program/MESSAGE',ROOT/'data/source code/OVL_002E.PAS',ROOT/'data/source code/OVL_0049.PAS']
    manifest=dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},regions=[])
    for name,path,origin,start,end in regions:
        capture_disassembly(objdump,path,origin,start,end,OUT/f'{name}.asm')
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'inventory_chest_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves objets/coffres ecrites dans',OUT)


if __name__=='__main__':main()
