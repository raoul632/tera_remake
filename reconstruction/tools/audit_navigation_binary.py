"""Preuves locales de la navigation Cite et des interactions liees aux niveaux."""
import json
import shutil
from audit_combat_binary import ROOT,OUT,capture_disassembly,describe_source


def main():
    """Capturer les regions pertinentes, sans redistribuer BIN/PAS/DATA."""
    objdump=shutil.which('objdump')
    if not objdump:raise SystemExit('objdump requis')
    OUT.mkdir(parents=True,exist_ok=True)
    com=ROOT/'data/compiled program/TERA.COM';overlays=ROOT/'data/compiled program/TERA.000'
    raw=overlays.read_bytes()
    zero,levels=OUT/'OVL_002E.bin',OUT/'OVL_0049.bin'
    zero.write_bytes(raw[0x2E00:0x4900]);levels.write_bytes(raw[0x4900:0x6900])
    regions=[('navigation_grid',com,0x100,0x7DBD,0x81D7),
             ('navigation_city_zero',zero,0xBA21,0xBA28,0xD521),
             ('navigation_city_levels',levels,0xBA21,0xBA28,0xDA21),
             ('navigation_invocation',com,0x100,0xB4FC,0xB625),
             ('navigation_arioch_scene',com,0x100,0x795E,0x7B05),
             ('navigation_entrance_sound',com,0x100,0x65B5,0x65F1),
             ('navigation_sound_rng',com,0x100,0x6121,0x6A72),
             ('navigation_combat_flash',com,0x100,0x6D32,0x6DC4),
             ('navigation_combat_action',com,0x100,0x8790,0x8CE3),
             ('navigation_combat_target',com,0x100,0x92FE,0x9419),
             ('navigation_combat_reward',com,0x100,0x9AAE,0x9B4E),
             ('navigation_main_relocation',com,0x100,0xCCC,0xD0E),
             ('navigation_main_entry',com,0x20FC,0xE40D,0xE4AD)]
    sources=[com,overlays,ROOT/'data/source code/OVL_002E.PAS',ROOT/'data/source code/OVL_0049.PAS',
             ROOT/'data/source code/TERAMAIN.PAS']
    manifest=dict(sources={p.relative_to(ROOT).as_posix():describe_source(p) for p in sources},regions=[])
    for name,path,origin,start,end in regions:
        capture_disassembly(objdump,path,origin,start,end,OUT/f'{name}.asm')
        manifest['regions'].append(dict(name=name,origin=origin,start=start,end_exclusive=end))
    (OUT/'navigation_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('Preuves navigation/lieux ecrites dans',OUT)


if __name__=='__main__':main()
