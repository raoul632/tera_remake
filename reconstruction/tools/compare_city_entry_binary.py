"""Verifier la branche TERAMAIN d'entree/retour, avec sa relocation runtime."""
import hashlib
import copy
import json
import struct
import subprocess
from compare_navigation_binary import NavigationRoutine, fixture, SOURCE, HERE, DATA, STACK, word, EXPECTED_HASH, OVERLAYS, OVERLAY_HASH
from navigation import pack_navigation
from unicorn.x86_const import UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_DX


class CityEntryRoutine(NavigationRoutine):
    def __init__(self,image,game,text):
        super().__init__(image,game,ord('E'),text,True)
        # Le COM stocke le corps principal apres la zone d'overlays.
        # CCC (appele par 2D95) le deplace de 1FFC : le JMP 2D9C vise DAF6.
        # Ainsi les appels E426->4850 et E47A->BA0F retrouvent les residents.
        self.cpu.mem_write(0xE40D,image[0xC411-0x100:0xC4B1-0x100])
        self.cpu.ctl_remove_cache(0xE40D,0xE4B0)
        self.enter_mode=True

    def on_instruction(self,cpu,address,size,user_data):
        if address==0xBA0F:
            selector=cpu.reg_read(UC_X86_REG_DX)
            if selector in (0x2E,0x49):
                self.initial=True;self.first_render=True;self.load_overlay(int(selector==0x49))
                cpu.reg_write(UC_X86_REG_IP,0xBA28);return
            self.return_from_hook();return # Facade F3, aucun RNG.
        super().on_instruction(cpu,address,size,user_data)

    def execute(self):
        self.cpu.mem_write(STACK,word(0xF100));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xE40D,0xE6E1,timeout=10000000,count=3000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==0xE6E1:self.states.append(self.snapshot(3))
        if not self.states:raise RuntimeError('Branche entree sans frontiere')
        return self.states


def c_states(game,text,leave=False):
    p=subprocess.run([str(HERE/'build/cmake/tera_navigation_bridge.exe')],
        input=pack_navigation(game,ord('E'),enter=not leave,leave=leave)+''.join(f'{ord(k)}\n' for k in text),
        text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(f'C entree {p.returncode}: {p.stderr}')
    states=[];maps=copy.deepcopy(game['maps']);characters=v=None;information=0
    for line in p.stdout.splitlines():
        tag,*raw=line.split();q=list(map(int,raw))
        if tag=='S':characters=q
        elif tag=='J':information=q[0]
        elif tag=='N':v=q
        elif tag=='B':maps[q[0]]=q[1:]
        elif tag=='Z':states.append(characters+[v[0],v[4],v[5],v[6],v[7],v[8],v[19],v[20],v[21],v[3]]+
                                    [z for row in maps for z in row]+[information])
    return states


def main():
    image=SOURCE.read_bytes();failures=[];count=boundaries=0
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:
        raise SystemExit("Empreinte originale differente")
    for seed in (0,1,23,1986):
        for position in (1000,1450,1899):
            for text in ('OFOFO','\rFOFO','N','\x1b'):
                game=fixture(seed);game.update(position=position,town_found=1,town_id=2,
                    city_overlay=0,city_backup_position=1234,arioch_position=777,tasks_done=[0]*4)
                machine=CityEntryRoutine(image,game,text);a=machine.execute();b=c_states(game,text)
                count+=1;boundaries+=len(b)
                if a!=b:
                    index=next((i for i,p in enumerate(zip(a,b)) if p[0]!=p[1]),min(len(a),len(b)))
                    differences=[(i,x,y) for i,(x,y) in enumerate(zip(a[index],b[index])) if x!=y] if index<min(len(a),len(b)) else []
                    failures.append(dict(seed=seed,position=position,text=text,boundary=index,differences=differences[:10]))
    # Les instructions de retour sont independantes du chargeur de facade.
    for backup in (1000,1450,1899):
        game=fixture(1986);game.update(world_id=0,position=0,previous_position=1,
            city_backup_position=backup,city_overlay=0,arioch_position=777,tasks_done=[0]*4)
        machine=CityEntryRoutine(image,game,'')
        # Copier aussi la partie retour de TERAMAIN, puis l'executer directement.
        machine.cpu.mem_write(0xE48C,image[0xC490-0x100:0xC4B1-0x100])
        machine.cpu.reg_write(UC_X86_REG_SP,STACK)
        machine.cpu.emu_start(0xE48C,0xE6E1,count=100000)
        a=[machine.snapshot(3)];b=c_states(game,'',leave=True);count+=1;boundaries+=len(b)
        if a!=b:failures.append(dict(return_backup=backup))
    report=dict(cases=count,boundaries=boundaries,mismatches=len(failures),first_mismatches=failures[:5],
                com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,main_runtime_relocation='1FFC, branche E40D..E4AA',scope='TERAMAIN city entry/return and real initial city render; BIOS/presentation abstracted')
    (HERE/'build/audit/city_entry_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
