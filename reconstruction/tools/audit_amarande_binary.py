"""Capturer les preuves des facades, panneaux et commandes d'Amarande.

Les extraits locaux restent dans build/audit, exclu de Git. Les originaux
et leurs ressources ne sont lus qu'en entree, jamais modifies ou copies
dans les sources de la reconstruction.
"""
import json
import shutil
from audit_combat_binary import ROOT,OUT,capture_disassembly,describe_source


def main():
    """Produire le manifeste SHA256 et les seules regions utiles a ce port."""
    objdump=shutil.which('objdump')
    if not objdump:raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    com=ROOT/'data/compiled program/TERA.COM'
    overlay=ROOT/'data/compiled program/TERA.000';raw=overlay.read_bytes()
    regions=[('amarande_facades_main',com,0x20FC,0xDC70,0xE1B1),
        ('amarande_panels_main',com,0x20FC,0xE5C7,0xE6E1),
        ('amarande_resident',com,0x100,0xAFE8,0xBA0F),
        ('amarande_halt',com,0x100,0xA709,0xAA13),
        ('amarande_local_map',com,0x100,0x5495,0x5E04),
        ('amarande_response',com,0x100,0x4473,0x4503),
        ('amarande_confirmation',com,0x100,0x81D7,0x8215)]
    for name,first,last in [('mahalore',0xEC00,0xF000),('city_facade',0xF300,0xFB00),
                            ('pyramid_facade',0x14200,0x14500),('port_facade',0x14800,0x14900)]:
        path=OUT/(name+'.bin');path.write_bytes(raw[first:last])
        regions.append((name,path,0xBA21,0xBA28,0xBA21+last-first))
    sources=[com,overlay,ROOT/'data/source code/TERAMAIN.PAS']
    sources += [ROOT/'data/compiled program'/name for name in ('MESSAGE','MESSAGE2','MESSAGE3','VERBES')]
    manifest=dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},regions=[])
    for name,path,origin,start,end in regions:
        capture_disassembly(objdump,path,origin,start,end,OUT/(name+'.asm'))
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'amarande_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves Amarande ecrites dans',OUT)


if __name__=='__main__':main()
