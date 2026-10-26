"""Capturer les preuves locales des lots monde/recrutement, sans redistribution."""
import json
import shutil
from audit_combat_binary import ROOT, OUT, capture_disassembly, describe_source


def main():
    """Extraire les deux overlays et les seules routines residentes auditees."""
    objdump = shutil.which('objdump')
    if not objdump:
        raise SystemExit('objdump requis')
    OUT.mkdir(parents=True, exist_ok=True)
    com = ROOT/'data/compiled program/TERA.COM'
    overlay = ROOT/'data/compiled program/TERA.000'
    raw = overlay.read_bytes()
    paths = []
    for name, start, end in [('0000',0,0x1000),('0010',0x1000,0x2200)]:
        path = OUT/f'OVL_{name}.bin'
        path.write_bytes(raw[start:end])
        paths.append(path)
    regions = [('world_maps',paths[0],0xBA21,0xBA28,0xC964),
               ('world_places',paths[1],0xBA21,0xBA28,0xCBCA),
               ('recruit_available',com,0x100,0xA074,0xA0DD),
               ('recruit_commands',com,0x100,0xAFE8,0xB97A),
               ('recruit_party_overflow',com,0x100,0x9419,0x9AAE)]
    sources = [com,overlay,ROOT/'data/compiled program/NOMPERS',
               ROOT/'data/source code/OVL_0000.PAS',ROOT/'data/source code/OVL_0010.PAS',
               ROOT/'data/source code/TERAMAIN.PAS']
    manifest = dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},
                    regions=[], stack_context='SP=FFFE, overlay call entry FFFC')
    for name, source, origin, start, end in regions:
        capture_disassembly(objdump,source,origin,start,end,OUT/f'{name}.asm')
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'world_recruitment_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves monde/recrutement ecrites dans',OUT)


if __name__=='__main__':
    main()
