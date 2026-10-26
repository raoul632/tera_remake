"""Executer les generateurs originaux avec le RNG reel et comparer tout l'etat."""
import hashlib
import json
import struct
import subprocess
import time
from compare_combat_binary import (Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE,
                                   OriginalRoutine, HERE, ROOT, SOURCE, EXPECTED_HASH, STACK, PARENT, STOP, word)
from unicorn.x86_const import UC_X86_REG_DS, UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_IP

DATA=0x10000
OVERLAY=ROOT/'data/compiled program/TERA.000'
NAMES=ROOT/'data/compiled program/NOMPERS'


class WorldRoutine(OriginalRoutine):
    def __init__(self,image,overlays,seed,difficulty):
        # Hooks cibles : observer chaque instruction des millions de pas
        # geographiques serait inutile et ralentirait fortement l'oracle.
        self.cpu=Uc(UC_ARCH_X86,UC_MODE_16)
        self.cpu.mem_map(0,65536);self.cpu.mem_map(DATA,65536)
        self.cpu.mem_write(0x100,image)
        self.cpu.reg_write(UC_X86_REG_DS,DATA>>4)
        self.cpu.reg_write(UC_X86_REG_BP,PARENT)
        self.write_word(PARENT-2,PARENT)
        self.overlays=overlays
        self.calls=0;self.attempts=0;self.placement_entries=[];self.collisions=[]
        self.cpu.mem_write(DATA+0x1FC,struct.pack('<I',seed))
        for address,value in ((0x75E8,difficulty),(0x75D4,500),(0x75E6,50)):
            self.cpu.mem_write(DATA+address,word(value))
        raw=NAMES.read_bytes()
        for group,address in ((0,0x94D2),(1,0xA684),(2,0xA6E8)):
            self.cpu.mem_write(DATA+address,raw[group*90:(group+1)*90])
        for address in (0x10DA,0xC599,0xBA2F,0xBB1F):
            self.cpu.hook_add(UC_HOOK_CODE,self.on_instruction,begin=address,end=address)

    def on_instruction(self,cpu,address,size,user_data):
        if address==0x10DA: self.calls+=1
        elif address==0xC599: self.attempts+=1
        elif address==0xBA2F:
            if self.phase==1:
                sp=cpu.reg_read(UC_X86_REG_SP)
                self.placement_entries.append((self.signed_word(sp)&65535,self.signed_word(sp+4)))
        elif address==0xBB1F and self.phase==1:
            if not cpu.mem_read(DATA+0xA744,1)[0]:
                bp=cpu.reg_read(UC_X86_REG_BP)
                self.collisions.append((self.placement_entries[-1],self.signed_word(bp+6)))

    def execute(self,full=False):
        self.phase=0
        self.cpu.mem_write(0xBA21,self.overlays[:0x1000])
        self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xC53B,0xC961,timeout=60000000,count=100000000)
        if self.cpu.reg_read(UC_X86_REG_IP)!=0xC961:
            raise RuntimeError(f'Generation interrompue IP={self.cpu.reg_read(UC_X86_REG_IP):04X}, draws={self.calls}')
        if full:
            self.phase=1
            self.cpu.mem_write(0xBA21,self.overlays[0x1000:0x2200])
            self.cpu.ctl_remove_cache(0xBA21,0xCC00)
            self.cpu.mem_write(0xFFFC,word(STOP))
            self.cpu.reg_write(UC_X86_REG_SP,0xFFFC)
            self.cpu.emu_start(0xBA28,0xCBCA,timeout=10000000,count=2000000)
            if self.cpu.reg_read(UC_X86_REG_IP)!=0xCBCA:
                raise RuntimeError('Initialisation geographique incomplete')
        return self.snapshot()

    def snapshot(self):
        seed=struct.unpack('<I',self.cpu.mem_read(DATA+0x1FC,4))[0]
        v=[seed,self.calls,self.attempts,len(self.collisions)]
        v += [self.signed_word(DATA+a) for a in (0x75D2,0x9B76,0x75D4,0x75E6,0x75E8,0x75E4,0x75D0,0x75DE,0x75E0)]
        for address,count in ((0x7564,9),(0x7576,25),(0x75A8,20),(0x7552,9),(0x7540,9)):
            v += [self.signed_word(DATA+address+2*i) for i in range(count)]
        result={'W':v,'M':list(self.cpu.mem_read(DATA+0x75EE,7700))}
        result['F']=[self.signed_word(DATA+0x752C+2*i) for i in range(10)]
        result['F']+=list(self.cpu.mem_read(DATA+0x940C,11))+list(self.cpu.mem_read(DATA+0x9419,4))
        result['F']+=[self.cpu.mem_read(DATA+a,1)[0] for a in (0xA74D,0x9417,0x9418)]
        result['F']+=[self.signed_word(DATA+0x75DC)]
        hints=[]
        for i in range(38):
            raw=bytes(self.cpu.mem_read(DATA+0x954A+40*i,40))
            hints+=list(raw[1:1+raw[0]].ljust(40,b'\0'))
        result['H']=hints
        words=[]
        for i in range(4):
            raw=bytes(self.cpu.mem_read(DATA+0x9B3A+10*i,10))
            words+=list(raw[1:1+raw[0]].ljust(10,b'\0'))
        result['K']=words+list(self.cpu.mem_read(DATA+0x75EA,4))
        actors=[]
        for i in range(10):
            actors+=[self.cpu.mem_read(DATA+0x9402+i,1)[0]]
            actors += [self.signed_word(DATA+a+2*i) for a in (0x74DC,0x74F0,0x7504,0x7518)]
            raw=bytes(self.cpu.mem_read(DATA+0x952C+3*i,3))
            actors+=list(raw[1:1+raw[0]].ljust(2,b'\0'))
            raw=bytes(self.cpu.mem_read(DATA+0x94C8+10*i,10))
            actors+=list(raw[1:1+raw[0]].ljust(10,b'\0'))
            for a in (0x7414,0x7478,0x73B0): actors+=list(self.cpu.mem_read(DATA+a+10*i,10))
        result['A']=actors
        return result


def execute_c(seed,difficulty,full=False):
    raw=NAMES.read_bytes()
    packet=f'{seed&65535} {seed>>16} {difficulty} {int(full)} 2000000 '+' '.join(map(str,raw))+'\n'
    out=subprocess.run([str(HERE/'build/cmake/tera_world_bridge.exe')],input=packet,
                       text=True,capture_output=True,check=True).stdout
    return {line.split()[0]:list(map(int,line.split()[1:])) for line in out.splitlines()}


def main():
    started=time.monotonic()
    image,overlays=SOURCE.read_bytes(),OVERLAY.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(overlays).hexdigest()!='539d95781f9296d40ecd05698eb98255835a45cb03e60b49cf581e3ada7038f4':
        raise SystemExit('Empreinte originale differente')
    subprocess.run(['cmake','--build',str(HERE/'build/cmake')],check=True)
    cases=[(seed,difficulty,full) for seed in (0,1,27,1986,0x80000000,0xffffffff)
           for difficulty in range(10) for full in (False,True)]
    cases += [(23,1,full) for full in (False,True)] # Carte refusee puis nouveau monde.
    failures=[];calls=attempts=collisions=0
    for index,(seed,difficulty,full) in enumerate(cases,1):
        machine=WorldRoutine(image,overlays,seed,difficulty)
        actual=machine.execute(full);c=execute_c(seed,difficulty,full)
        calls+=machine.calls;attempts+=machine.attempts;collisions+=len(machine.collisions)
        for tag in actual:
            dif=[(i,a,b) for i,(a,b) in enumerate(zip(actual[tag],c[tag])) if a!=b]
            if dif or len(actual[tag])!=len(c[tag]):
                failures.append(dict(seed=seed,difficulty=difficulty,full=full,tag=tag,differences=dif[:10]))
        if index%20==0:print(f'Mondes compares {index}/{len(cases)} ; ecarts {len(failures)}',flush=True)
    # Reprendre la memoire produite par les deux overlays originaux dans
    # l'oracle 0022 : aucune reconstruction Python des donnees intermediaires.
    from compare_creation_binary import CreationRoutine, execute_c as create_c
    from world import generate_world
    hero_cases=hero_boundaries=0
    for seed in (23,27,1986):
        for text in ('ALIX\rH333FIVEHAO','FIRST\rF999FFFFFFNSECOND\rH000NNNNNNO'):
            machine=WorldRoutine(image,overlays,seed,1)
            machine.execute(True)
            game=generate_world(seed,1)
            game['party_available']=1 # TERAMAIN apres 0010, avant 0022.
            keys=list(map(ord,text))
            hero=CreationRoutine(image,overlays,game,keys)
            hero.cpu.mem_write(DATA,bytes(machine.cpu.mem_read(DATA,65536)))
            hero.cpu.mem_write(DATA+0xA745,b'\x01')
            actual,c=hero.run(False),create_c(game,keys,False)
            hero_cases+=1;hero_boundaries+=len(c)
            if actual!=c:failures.append(dict(seed=seed,tag='generated_world_to_hero'))
    report=dict(cases=len(cases),hero_cases=hero_cases,hero_input_boundaries=hero_boundaries,mismatches=len(failures),first_mismatches=failures[:5],
                random_calls=calls,map_attempts=attempts,placement_collisions=collisions,
                com_sha256=EXPECTED_HASH,overlays_sha256=hashlib.sha256(overlays).hexdigest(),
                original_stack_top='FFFE (call entry FFFC)',seconds=round(time.monotonic()-started,2),
                scope='full OVL0000 generation after seed/difficulty input; full OVL0010; original RNG; all map bytes, names, stats, placements, hints, words, H')
    (HERE/'build/audit/world_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__': main()
