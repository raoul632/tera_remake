"""Captures et provenance des lots 19/20 ; originaux lus sans modification."""
import json
import shutil
from audit_combat_binary import ROOT,OUT,capture_disassembly,describe_source


def main():
    """Extraire seulement les overlays et regions utiles sous build/audit."""
    objdump=shutil.which('objdump')
    if not objdump:raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    com=ROOT/'data/compiled program/TERA.COM';overlay=ROOT/'data/compiled program/TERA.000'
    raw=overlay.read_bytes()
    regions=[('space_main_entry',com,0x20FC,0xE20D,0xE405),
        ('travel_sounds',com,0x100,0x6121,0x6A72),
        ('alfol_resident',com,0x100,0xAFE8,0xB77D),
        ('alfol_status_bug',com,0x100,0x7CC8,0x7D8E),
        ('alfol_landing_scene',com,0x100,0x6C4F,0x734C)]
    for name,first,last in [('space',0xAE00,0xBC00),('alfol',0x8500,0xA400)]:
        path=OUT/(name+'.bin');path.write_bytes(raw[first:last])
        regions.append((name,path,0xBA21,0xBA28,0xBA21+last-first))
    sources=[com,overlay,ROOT/'data/source code/OVL_00AE.PAS',ROOT/'data/source code/OVL_0085.PAS',
        ROOT/'data/source code/TERAMAIN.PAS',ROOT/'data/compiled program/VALMON']
    manifest=dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},regions=[])
    for name,path,origin,start,end in regions:
        capture_disassembly(objdump,path,origin,start,end,OUT/(name+'.asm'))
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'space_alfol_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves spatioport/Alfol ecrites dans',OUT)


if __name__=='__main__':main()
