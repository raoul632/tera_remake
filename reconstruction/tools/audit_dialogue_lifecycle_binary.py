"""Provenance BIN/PAS des dialogues et du cycle de defaite/succession."""
import json
import shutil
from audit_combat_binary import ROOT,OUT,capture_disassembly,describe_source


def main():
    objdump=shutil.which('objdump')
    if not objdump:raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    com=ROOT/'data/compiled program/TERA.COM';overlay=ROOT/'data/compiled program/TERA.000'
    raw=overlay.read_bytes();ending=OUT/'life_00CB.bin';creation=OUT/'life_0022.bin'
    ending.write_bytes(raw[0xCB00:0xD200]);creation.write_bytes(raw[0x2200:0x2E00])
    regions=[('dialogue_menu',com,0x100,0xAFE8,0xB2A0),
        ('dialogue_seduction',com,0x100,0xB671,0xB935),
        ('dialogue_information_context',com,0x100,0x6073,0x60BB),
        ('life_ending',ending,0xBA21,0xBA28,0xC121),
        ('life_creation',creation,0xBA21,0xBA28,0xC549),
        ('life_main_start',com,0x20FC,0xDBAC,0xDC15),
        ('life_main_end',com,0x20FC,0xE757,0xE7D7)]
    sources=[com,overlay,ROOT/'data/source code/OVL_00CB.PAS',ROOT/'data/source code/OVL_0022.PAS',
        ROOT/'data/source code/TERAMAIN.PAS',ROOT/'data/compiled program/MESSAGE',
        ROOT/'data/compiled program/MESSAGE2',ROOT/'data/compiled program/MESSAGE3']
    manifest=dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},regions=[])
    for name,path,origin,start,end in regions:
        capture_disassembly(objdump,path,origin,start,end,OUT/f'{name}.asm')
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'dialogue_lifecycle_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves dialogues/succession ecrites dans',OUT)


if __name__=='__main__':main()
