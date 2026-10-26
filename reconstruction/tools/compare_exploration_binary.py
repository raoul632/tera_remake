"""Comparer les mouvements et C_A0DD aux instructions originales, RNG execute."""
import copy
import hashlib
import json
import random
import struct
import subprocess
import sys
import time
from compare_interactive_binary import LiveRoutine, DATA, SOURCE, EXPECTED_HASH, HERE, STACK, STOP, word
from compare_creation_binary import CreationRoutine, OVERLAYS, OVERLAY_HASH
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_SP, UC_X86_REG_IP
sys.path.insert(0,str(HERE/'frontend'))
from exploration import pack_exploration
from combat import demo_game


class ExplorationRoutine(LiveRoutine):
    def __init__(self,image,game,keys,mode,key):
        super().__init__(image,game,keys)
        self.cpu.mem_write(DATA+0xA750,bytes([game.get('information_context',0)]))
        self.cpu.mem_write(DATA+0xA74D,bytes([game.get('fast_moves',0)]))
        self.mode=mode;self.key=key;self.daily_reached=False
        for name,address in [('days',0x75E2),('level',0x75D8),('dummy_level',0x75DA),('energy',0x75E6),('alfol_position',0x75D0)]:
            self.data_word(address,game[name])
        for name,address in [('sound',0xA74A),('moved',0xA749),('town_found',0xAFD4),('town_id',0xAFD3),
                             ('force_day',0xAFD0),('encounter_suppressed',0xAFD2),('arrival_pending',0xA74F),('party_available',0xA745)]:
            self.cpu.mem_write(DATA+address,bytes([game.get(name,0)]))
        for name,address,start in [('horizontal_mountains',0x7564,0),('vertical_mountains',0x7576,0),
                                   ('towns',0x75A8,0),('locations',0x7550,1),('extra_locations',0x753E,1),('keys',0x752C,0)]:
            for i,v in enumerate(game[name][start:]):self.data_word(address+2*(i+start),v)
        for i,actor in enumerate(game['actors']):
            raw=actor['name'].encode('cp437');self.cpu.mem_write(DATA+0x94C8+10*i,bytes([len(raw)])+raw.ljust(9,b'\0'))
        self.cpu.mem_write(DATA+0x75EE,bytes(v for row in game['maps'] for v in row))
        self.cpu.mem_write(DATA+0xA679,bytes([key]))
        self.encounters=0

    def state(self,phase=0,actor=0,attempt=0):
        # Le meme record canonique que le pont personnages, puis les flags
        # durables. Aucun calcul de soin/niveau/deplacement dans cet oracle.
        values=CreationRoutine.state(self,5)
        values += [self.cpu.mem_read(DATA+a,1)[0] for a in (0xA749,0xAFD4,0xAFD3,0xAFD0,0xAFD2,0xAFCE,0xA748,0xAFD1)]
        values += [self.signed_word(DATA+0x9B72)]
        values += [self.cpu.mem_read(DATA+0xA750,1)[0]]
        return values

    def on_instruction(self,cpu,address,size,user_data):
        if address==0x6073:
            cpu.mem_write(DATA+0xA750,b'\1');self.return_from_hook();return
        if address==0xA0DD:
            self.daily_reached=True
            if self.mode==0:self.return_from_hook();return
        if address==0x9B45:return # Le retour du combat doit reprendre le jour.
        if address==0x9B4E:self.encounters+=1
        if address==0x0DA4:
            cpu.reg_write(UC_X86_REG_AX,0);self.return_from_hook();return
        primitives={0x6013:0,0x6031:0,0x6052:0,0x6073:0,0x60BB:0,
                    0x4601:10,0x5051:6,0x5E04:6,0x4544:10,0x46BC:10,0x2DB7:10,
                    0x2DB4:6,0x2DC0:8,0x2DCF:8,0x4520:2,
                    0x2DC3:12,0x2DC6:8}
        if address in primitives:self.return_from_hook(primitives[address]);return
        super().on_instruction(cpu,address,size,user_data)

    def execute(self):
        if self.mode!=1:
            self.cpu.mem_write(0xBA21,OVERLAYS.read_bytes()[0xFB00:0x11300])
        self.cpu.mem_write(STACK,word(STOP))
        self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xA0DD if self.mode==1 else 0xBA28,STOP,timeout=10000000,count=3000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:self.states.append(self.state())
        elif self.stop_reason!='input':raise RuntimeError(f'Original interrompu IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(game,keys,mode,key):
    result=subprocess.run([str(HERE/'build/cmake/tera_exploration_bridge.exe')],
        input=pack_exploration(game,mode,key)+''.join(f'{k}\n' for k in keys),
        text=True,capture_output=True,timeout=10)
    if result.returncode not in (0,4):raise RuntimeError(f'C {result.returncode}: {result.stderr}')
    states=[];snapshot=None;information=0
    for line in result.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':snapshot=v
        elif tag=='J':information=v[0]
        elif tag=='D':states.append(snapshot+v[5:13]+[v[19],information])
    return states


def fixture(seed):
    game=demo_game(7,seed)
    game.update(party_available=1,days=7300,level=0,dummy_level=0,energy=50,keys=[10]*10,
                name_variants=[['']*10,['']*10],arrival_pending=0,
                alfol_position=75,sound=1,moved=0,town_found=0,town_id=0,
                force_day=0,encounter_suppressed=0,world_id=11,position=1000,
                horizontal_mountains=[-1]*9,vertical_mountains=[-1]*25,
                towns=[-1]*20,locations=[0]+[-1]*9,extra_locations=[0]+[-1]*9,
                maps=[[0,0,0,0,255,0,v] for v in game['maps_column7']])
    return game


def cases():
    result=[]
    for key in '2468':
        for pos in (-1,0,4,30,761,899,957,999,1000,1001,1029,1030,1869,1898,1899):
            for sound in (0,1):
                g=fixture(1986);g.update(position=pos,sound=sound)
                result.append((f'edge/{pos}/{key}/{sound}',g,0,ord(key),'FO'))
    for scene in range(23):
        for near in (False,True):
            for town in (-1,0,9,10,19):
                pos=1000+(scene-1000)%23
                g=fixture(scene);g['position']=pos-1
                if near:g['horizontal_mountains'][0]=pos
                if town>=0:g['towns'][town]=pos
                g['extra_locations'][1]=pos
                result.append((f'scene/{scene}/{near}/{town}',g,0,ord('8'),''))
    for key in '2468':
        g=fixture(1);g['horizontal_mountains'][0]=1000 if key=='8' else 999
        g['vertical_mountains'][0]=1000 if key=='6' else 970
        result.append((f'blocked/{key}',g,2,ord(key),''))
    rng=random.Random(1986)
    for index in range(250):
        g=fixture(rng.getrandbits(32))
        g.update(world_id=rng.choice([0,4,11]),position=rng.choice([0,75,800,1000,1500,450]),
                 force_day=rng.randrange(2),encounter_suppressed=rng.randrange(2),
                 experience=rng.choice([0,10,50,255,32767,-1]),level=rng.choice([0,1,3]),
                 days=rng.choice([7300,32767]),difficulty=rng.randrange(10))
        for id,actor in enumerate(g['actors']):
            actor.update(active=int(id<1+index%10),hp=rng.choice([-1,0,1,20,32767]),
                         magic=rng.choice([-1,0,3,32767]),condition=rng.choice(['OK','EM','HS']))
            actor['stats']=[rng.randrange(13) for _ in range(10)]
            actor['skills']=[rng.randrange(6) for _ in range(10)]
            actor['equipment']=[rng.randrange(26) for _ in range(10)]
        result.append((f'day/{index}',g,1,ord('8'),'FOFOFO'))
    # Rencontres frequentes, tous les decors de monstres rencontres, et
    # passages effectifs mouvement -> jour -> combat -> retour.
    for seed in range(150):
        g=fixture(seed);g.update(experience=255,level=10,position=1500)
        result.append((f'roaming/{seed}',g,1,ord('8'),'FOFOFO'))
    # Un exemple garanti pour chacun des onze types de rencontre errante.
    for seed in (14,30,87,103,183,231,310,336,352,384,913):
        g=fixture(seed);g.update(experience=255,level=10,position=1500)
        result.append((f'roaming/type_coverage/{seed}',g,1,ord('8'),'FOFOFO'))
    for seed in range(60):
        g=fixture(seed);g.update(experience=255,level=10,position=450,world_id=4)
        result.append((f'treasure/{seed}',g,1,ord('8'),'FOFOFO'))
    for id in range(1,10):
        g=fixture(id);g.update(position=450,world_id=4)
        g['locations'][id]=450
        result.append((f'npc/{id}',g,1,ord('8'),'FO'))
    for seed in range(20):
        g=fixture(seed);g.update(experience=255,level=10,position=1499)
        result.append((f'integrated/{seed}',g,2,ord('8'),'FOFOFO'))
    return result


def main():
    started=time.monotonic();image=SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:
        raise SystemExit('Empreinte originale differente')
    failures=[];boundaries=draws=encounters=0
    corpus=cases()
    for index,(name,game,mode,key,text) in enumerate(corpus,1):
        machine=ExplorationRoutine(image,game,list(map(ord,text)),mode,key)
        binary=machine.execute();c=execute_c(game,list(map(ord,text)),mode,key)
        if binary!=c:
            offset=next((i for i,p in enumerate(zip(binary,c)) if p[0]!=p[1]),min(len(binary),len(c)))
            diffs=[(i,a,b) for i,(a,b) in enumerate(zip(binary[offset],c[offset])) if a!=b] if offset<min(len(binary),len(c)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(binary),len(c)],diffs=diffs[:15]))
        boundaries+=len(c);draws+=len(machine.calls);encounters+=machine.encounters
        if index%100==0:print(f'Explorations {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),input_boundaries=boundaries,original_rng_calls=draws,
                encounters=encounters,mismatches=len(failures),first_mismatches=failures[:10],
                seconds=round(time.monotonic()-started,2),com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,
                scope='OVL00FB, full A0DD and nested combat; original RNG; graphics/BIOS bypassed; animation poll=no key')
    (HERE/'build/audit/exploration_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
