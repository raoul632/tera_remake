"""Capturer les regions BIN des deplacements et du calendrier quotidien."""
import json
import shutil
from audit_combat_binary import ROOT, OUT, capture_disassembly, describe_source


def main():
    """Ecrire les preuves et empreintes sous build/audit, sans toucher les originaux."""
    objdump=shutil.which('objdump')
    if not objdump:raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    com=ROOT/'data/compiled program/TERA.COM'
    overlays=ROOT/'data/compiled program/TERA.000'
    movement=OUT/'OVL_00FB.bin'
    movement.write_bytes(overlays.read_bytes()[0xFB00:0x11300])
    regions=[('exploration_amarande',movement,0xBA21,0xBA28,0xD18E),
             ('exploration_day',com,0x100,0xA0DD,0xA709),
             ('exploration_monster_display',com,0x100,0x9B4E,0xA074),
             ('exploration_mountains',com,0x100,0x6C4F,0x6D32),
             ('exploration_town_windows',com,0x100,0x724A,0x73ED),
             ('exploration_monster_animation',com,0x100,0x7868,0x795E)]
    sources=[com,overlays,ROOT/'data/source code/OVL_00FB.PAS',ROOT/'data/source code/TERAMAIN.PAS']
    manifest=dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},regions=[])
    for name,source,origin,start,end in regions:
        capture_disassembly(objdump,source,origin,start,end,OUT/f'{name}.asm')
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'exploration_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves exploration/jours ecrites dans',OUT)


if __name__=='__main__':main()
