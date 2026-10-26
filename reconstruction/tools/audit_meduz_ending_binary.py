"""Capturer les preuves ciblees des lots Meduz/fin sans toucher aux originaux."""
import json
import shutil
from audit_combat_binary import ROOT,OUT,capture_disassembly,describe_source


def main():
    """Ecrire extraits et manifeste sous build/audit, exclu de Git."""
    objdump=shutil.which('objdump')
    if not objdump:raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    com=ROOT/'data/compiled program/TERA.COM';overlay=ROOT/'data/compiled program/TERA.000';raw=overlay.read_bytes()
    regions=[('meduz_main_entry',com,0x20FC,0xE4AD,0xE57B),
        ('ending_main_exit',com,0x20FC,0xE6E1,0xE7D0),
        ('ending_real_runtime',com,0x100,0x1745,0x1B90),
        ('ending_format_runtime',com,0x100,0x1F45,0x2170),
        ('meduz_sounds',com,0x100,0x68F4,0x693B)]
    for name,a,b in [('meduz',0xA400,0xAE00),('ending',0xCB00,0xD200),
        ('meduz_transition',0x14500,0x14800),('meduz_pyramid',0x14200,0x14500)]:
        path=OUT/(name+'.bin');path.write_bytes(raw[a:b]);regions.append((name,path,0xBA21,0xBA28,0xBA21+b-a))
    sources=[com,overlay,*[ROOT/'data/source code'/name for name in
        ('OVL_00A4.PAS','OVL_00CB.PAS','OVL_0145.PAS','OVL_0142.PAS','TERAMAIN.PAS')]]
    manifest=dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},regions=[])
    for name,path,origin,start,end in regions:
        capture_disassembly(objdump,path,origin,start,end,OUT/(name+'.asm'))
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'meduz_ending_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves Meduz/fin ecrites dans',OUT)


if __name__=='__main__':main()
