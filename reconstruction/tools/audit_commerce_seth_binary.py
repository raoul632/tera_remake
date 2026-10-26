"""Captures ciblees, empreintes et provenance des lots Kronopolis/Seth."""
import json
import shutil
from audit_combat_binary import ROOT,OUT,capture_disassembly,describe_source


def main():
    """Lire les originaux ; ecrire uniquement sous build/audit, exclu de Git."""
    objdump=shutil.which('objdump')
    if not objdump:raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    com=ROOT/'data/compiled program/TERA.COM';overlay=ROOT/'data/compiled program/TERA.000';raw=overlay.read_bytes()
    regions=[('places_main_facades',com,0x20FC,0xDC70,0xDE06),
        ('places_main_seth_entry',com,0x20FC,0xE57B,0xE5C7),
        ('places_music',com,0x100,0x6352,0x6496),
        ('places_monster',com,0x100,0x9B4E,0xA074)]
    for name,first,last in [('commerce',0xBC00,0xCB00),('seth',0x6900,0x8500),('seth_facade',0xF000,0xF300)]:
        path=OUT/(name+'.bin');path.write_bytes(raw[first:last])
        regions.append((name,path,0xBA21,0xBA28,0xBA21+last-first))
    sources=[com,overlay,ROOT/'data/source code/OVL_00BC.PAS',ROOT/'data/source code/OVL_0069.PAS',
        ROOT/'data/source code/OVL_00F0.PAS',ROOT/'data/source code/TERAMAIN.PAS',
        ROOT/'data/compiled program/OBJETS',ROOT/'data/compiled program/MESSAGE']
    manifest=dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},regions=[])
    for name,path,origin,start,end in regions:
        capture_disassembly(objdump,path,origin,start,end,OUT/(name+'.asm'))
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'commerce_seth_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves Kronopolis/Seth ecrites dans',OUT)


if __name__=='__main__':main()
