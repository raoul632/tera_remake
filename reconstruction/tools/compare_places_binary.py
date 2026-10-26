"""Tirages des facades de Kronopolis/Seth compares aux instructions originales."""
import hashlib
import json
import subprocess
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,OVERLAY_HASH,STACK,STOP,word
from unicorn.x86_const import UC_X86_REG_SP,UC_X86_REG_IP
from exploration import pack_exploration


class FacadeRoutine(ExplorationRoutine):
    def on_instruction(self,cpu,address,size,u):
        # Chargement du bitmap krono.ima : ces appels n'ont pas de regles/RNG.
        if address in (0x29C4,0x29C9,0x2A20):
            self.return_from_hook({0x29C4:0,0x29C9:0,0x2A20:0}[address]);return
        super().on_instruction(cpu,address,size,u)


def main():
    image=SOURCE.read_bytes();raw=OVERLAYS.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(raw).hexdigest()!=OVERLAY_HASH:raise SystemExit('Empreinte differente')
    failures=[];calls=count=0
    for town in (0,4):
        for sound in (0,1):
            for seed in (0,1,1986,0xffffffff):
                g=fixture(seed);g.update(town_found=1,town_id=town,sound=sound,moved=1)
                m=FacadeRoutine(image,g,[],1,0);m.cpu.mem_write(STACK,word(STOP));m.cpu.reg_write(UC_X86_REG_SP,STACK)
                if town==4:
                    m.cpu.mem_write(0xBA21,raw[0xF000:0xF300]);start,end=0xBA28,STOP
                else:
                    # Le chargement de fichier et MOVE du bitmap sont abstraits.
                    # Debut apres MOVE a DCEF : seulement dessin, texte et musique.
                    m.cpu.mem_write(0xDA21,image[0xDA21-0x20FC:]);start,end=0xDCEF,0xDD6B
                m.cpu.emu_start(start,end,timeout=10000000,count=1000000)
                if m.cpu.reg_read(UC_X86_REG_IP)!=end:raise RuntimeError(f'Facade {town} incomplete {m.cpu.reg_read(UC_X86_REG_IP):04X}')
                expected=int.from_bytes(m.cpu.mem_read(DATA+0x1FC,4),'little')
                p=subprocess.run([str(HERE/'build/cmake/tera_places_bridge.exe')],input=pack_exploration(g,0,0),text=True,capture_output=True,check=True)
                actual=int(next(line for line in p.stdout.splitlines() if line.startswith('S ')).split()[2])
                if actual!=expected:failures.append(dict(town=town,sound=sound,seed=seed,original=expected,c=actual))
                count+=1;calls+=len(m.calls)
    report=dict(cases=count,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures,
        com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,scope='facades : graphe dessin/son original, chargement bitmap DOS abstrait')
    (HERE/'build/audit/places_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
