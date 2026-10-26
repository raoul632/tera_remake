"""Executer les overlays Cite et leurs sous-routines, sans oracle Python de regles."""
import copy
import hashlib
import json
import random
import struct
import subprocess
import time
from compare_exploration_binary import ExplorationRoutine, fixture, DATA, HERE, SOURCE, EXPECTED_HASH, OVERLAYS, OVERLAY_HASH, STACK, STOP, word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_SP, UC_X86_REG_IP
from navigation import pack_navigation


class NavigationRoutine(ExplorationRoutine):
    def __init__(self,image,game,key,text,initial=False):
        super().__init__(image,game,list(map(ord,text)),1,key)
        self.command=key;self.initial=initial;self.first_render=True;self.first_command=True
        self.overlay=game.get('city_overlay',int(game['world_id']!=0));self.nav_phase=3;self.in_combat=False
        self.backup=game.get('city_backup_position',0);self.flee_move=game.get('grid_moved',0)
        self.cpu.mem_write(DATA+0xA742,bytes([self.flee_move]))
        self.data_word(0x75E4,game.get('arioch_position',777));self.data_word(0x75DC,game.get('tasks_counter',0))
        self.cpu.mem_write(DATA+0x941B,bytes([game.get('tasks_done',[0]*4)[2]]))
        self.data_word(0xAFD5,self.backup)

    def snapshot(self,phase):
        values=CreationRoutine.state(self,5)
        values += [phase,self.overlay]
        values += [self.cpu.mem_read(DATA+a,1)[0] for a in (0xAFD2,0xAFCE,0xAFD1)]
        values += [self.signed_word(DATA+0x9B72),self.cpu.mem_read(DATA+0x941B,1)[0],
                   self.signed_word(DATA+0x75DC),self.cpu.mem_read(DATA+0xAFD0,1)[0],self.signed_word(DATA+0xAFD5)]
        values += list(self.cpu.mem_read(DATA+0x75EE,7700))
        values += [self.cpu.mem_read(DATA+0xA750,1)[0]]
        return values

    def state(self,*args):return self.snapshot(self.nav_phase)

    def on_instruction(self,cpu,address,size,user_data):
        if address==0xCB88 and not self.initial and self.first_render:
            cpu.reg_write(UC_X86_REG_SP,cpu.reg_read(UC_X86_REG_SP)-1) # BP_03 reste alloue.
            cpu.reg_write(UC_X86_REG_IP,0xCBD5);return # Entree deja accomplie dans l'etat fourni.
        if address==(0xC9B7 if self.overlay else 0xC792) and self.first_render:
            self.first_render=False
            if not self.initial:
                self.cpu.mem_write(DATA+0xA742,b'\0');self.return_from_hook();return
        if address==0x838A:self.in_combat=True
        if address==0x9B45:self.in_combat=False;return
        if address==0x4277:
            sp=cpu.reg_read(UC_X86_REG_SP);caller=self.signed_word(sp)&65535
            if caller>=0xBA21:
                if self.initial or not self.first_command:
                    self.states.append(self.snapshot(3));self.stop_reason='command';cpu.emu_stop();return
                self.first_command=False
                cpu.reg_write(UC_X86_REG_AX,self.command);self.return_from_hook(1);return
            self.nav_phase=2 if self.in_combat else 5 if caller==0x42D4 else 0 if caller in (0x7E09,0x7E67) else 1
            self.states.append(self.snapshot(self.nav_phase))
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0)
            if 97<=key<=122:key-=32
            cpu.reg_write(UC_X86_REG_AX,key);self.return_from_hook(1);return
        # Ne pas sauter C_A0DD : c'est le calendrier reel apres chaque vue.
        if address==0x4210:self.return_from_hook();return # Pause BIOS, aucun calcul/RNG.
        if address==0x47B3:self.return_from_hook(80);return
        if address in (0x666B,0x6721):self.return_from_hook();return
        super().on_instruction(cpu,address,size,user_data)

    def load_overlay(self,overlay):
        self.overlay=overlay
        raw=OVERLAYS.read_bytes()
        self.cpu.mem_write(0xBA21,raw[0x4900:0x6900] if overlay else raw[0x2E00:0x4900])
        self.cpu.ctl_remove_cache(0xBA21,0xDA21)

    def execute(self):
        self.load_overlay(self.overlay)
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xBA28,STOP,timeout=10000000,count=3000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:
            world=self.signed_word(DATA+0x9B76);position=self.signed_word(DATA+0x75D2)
            # TERAMAIN rappelle l'overlay suivant quand le precedent revient.
            if (world==1 and self.overlay==0) or (world==0 and position!=0 and self.overlay==1):
                self.initial=True;self.first_render=True;self.load_overlay(int(world!=0))
                self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
                self.cpu.emu_start(0xBA28,STOP,timeout=10000000,count=3000000)
            else:self.states.append(self.snapshot(3))
        if not self.states:raise RuntimeError(f'Pas de frontiere, IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(game,key,text,initial=False):
    process=subprocess.run([str(HERE/'build/cmake/tera_navigation_bridge.exe')],
        input=pack_navigation(game,key,initial_move=initial)+''.join(f'{ord(k)}\n' for k in text),
        text=True,capture_output=True,timeout=10)
    if process.returncode not in (0,4):raise RuntimeError(f'Pont {process.returncode}: {process.stderr}')
    states=[];maps=copy.deepcopy(game['maps']);characters=None;v=None;information=0
    for line in process.stdout.splitlines():
        tag,*raw=line.split();values=list(map(int,raw))
        if tag=='S':characters=values
        elif tag=='J':information=values[0]
        elif tag=='N':v=values
        elif tag=='B':maps[values[0]]=values[1:]
        elif tag=='Z':states.append(characters+[v[0],v[4],v[5],v[6],v[7],v[8],v[19],v[20],v[21],v[3]]+
                                    [n for row in maps for n in row]+[information])
    return states


def cases():
    corpus=[]
    for overlay in (0,1):
        for heading in (1,-1,10,-10):
            for key in '2468':
                for wall in range(4):
                    for response in ('O','U','\r','N'):
                        g=fixture(1986);g.update(world_id=0 if overlay==0 else 2,position=45 if overlay==0 else 245,
                                                city_overlay=overlay,keys=[0]*10,arioch_position=777,tasks_done=[0]*4)
                        g['previous_position']=g['position']-heading
                        g['maps'][g['position']][:4]=[wall]*4
                        g['maps'][g['position']][4]=0
                        corpus.append((f'move/{overlay}/{heading}/{key}/{wall}/{response!r}',g,ord(key),response+'FOFO',False))
    for overlay in (0,1):
        for left in range(4):
            for right in range(4):
                for seed in (0,1,23):
                    g=fixture(seed);g.update(world_id=0 if not overlay else 2,position=45 if not overlay else 245,
                                            city_overlay=overlay,arioch_position=777,tasks_done=[0]*4)
                    g['previous_position']=g['position']-1
                    g['maps'][g['position']][:4]=[1,left,1,right];g['maps'][g['position']][4]=0
                    corpus.append((f'decor/{overlay}/{left}/{right}/{seed}',g,0,'FOFO',True))
    for world in range(1,9):
        for key,content in (('D',101),('M',100),('L',104),('L',114),('I',103)):
            if world==8 and key=='D':continue
            for response in ('O','\r','N'):
                g=fixture(world);g.update(world_id=world,position=world*100+45,city_overlay=1,
                                         arioch_position=777,tasks_done=[0]*4)
                g['previous_position']=g['position']-1;g['maps'][g['position']][4]=content
                for w in range(8):g['maps'][w*100+45][4]=101
                g['maps'][g['position']][4]=content
                corpus.append((f'command/{world}/{key}/{response!r}',g,ord(key),response+'FOFO',False))
    for kind in range(1,23):
        for overlay in (0,1):
            g=fixture(kind);g.update(world_id=0 if not overlay else 2,position=45 if not overlay else 245,
                                    city_overlay=overlay,tasks_done=[0]*4)
            g['arioch_position']=g['position'];g['previous_position']=g['position']-1
            g['maps'][g['position']][4]=0;g['maps'][g['position']][5]=kind
            corpus.append((f'fixed/{kind}/{overlay}',g,0,'FOFO',True))
    for overlay in (0,1):
        for text in ('ARIOCH\rFOFO','WRONG\r','ar7io\bOCH\rFOFO','ABCDEFGHIJ'):
            g=fixture(31);g.update(world_id=0 if not overlay else 2,position=45 if not overlay else 245,
                                   city_overlay=overlay,arioch_position=777,tasks_done=[0]*4)
            g['previous_position']=g['position']-1
            g['maps'][g['position']][4]=0
            corpus.append((f'invocation/{overlay}/{text!r}',g,ord('I'),text,False))
    for heading in (1,-1,10,-10):
        for key in '2468':
            g=fixture(23);g.update(world_id=0,position=45,city_overlay=0,tasks_done=[0]*4,arioch_position=777)
            g['previous_position']=45-heading;g['maps'][45][4]=101
            corpus.append((f'portal/{heading}/{key}',g,ord(key),'FOFO',False))
    for response in ('O','U','N','\r'):
        g=fixture(1);g.update(world_id=0,position=45,city_overlay=0,arioch_position=777,tasks_done=[0]*4)
        g['previous_position']=44;g['maps'][45][0]=3;g['keys'][0]=10
        corpus.append((f'no_key/{response!r}',g,ord('8'),response,False))
    for key in '2468':
        g=fixture(1);g.update(world_id=0,position=1,city_overlay=0,arioch_position=777,tasks_done=[0]*4)
        g['previous_position']=2;g['maps'][1][2]=1;g['maps'][1][4]=0
        corpus.append((f'exit/{key}',g,ord(key),'FOFO',False))
    for overlay in (0,1):
        for kind in (1,21):
            for done in (0,1):
                g=fixture(1986);g.update(world_id=0 if not overlay else 2,position=45 if not overlay else 245,
                                       city_overlay=overlay,tasks_done=[0,0,done,0],tasks_counter=32767)
                g['arioch_position']=g['position'];g['previous_position']=g['position']-1
                g['maps'][g['position']][4]=0;g['maps'][g['position']][5]=kind
                g['actors'][0]['magic']=200;g['actors'][0]['hp']=10000
                action='TO' if kind==1 else 'COTEST\r'
                corpus.append((f'fixed_victory/{kind}/{overlay}/{done}',g,0,action*12+'FOFO',True))
    rng=random.Random(1986)
    for overlay in (0,1):
        for id in (1,2,6):
            for condition in ('OK','HS'):
                g=fixture(1986);g.update(world_id=2 if overlay else 0,position=245 if overlay else 45,
                    city_overlay=overlay,tasks_done=[0]*4,arioch_position=777,information_context=0)
                g['previous_position']=g['position']-1;g['maps'][g['position']][4:]=[0,0,1]
                g['locations'][id]=g['position'];g['actors'][id].update(active=0,condition=condition)
                corpus.append((f'information_context/{overlay}/{id}/{condition}',g,0,'FOFO',True))
    for overlay in (0,1):
        for sound in (0,1):
            for key in '2468':
                g=fixture(1986);g.update(world_id=0 if not overlay else 2,
                    position=45 if not overlay else 245,city_overlay=overlay,
                    sound=sound,fast_moves=1,tasks_done=[0]*4,arioch_position=777)
                g['previous_position']=g['position']-1
                g['maps'][g['position']][:4]=[1]*4
                corpus.append((f'fast_moves/{overlay}/{sound}/{key}',g,ord(key),'FOFO',False))
    for index in range(50):
        g=fixture(rng.getrandbits(32));g.update(world_id=2,position=245,city_overlay=1,
                                             arioch_position=777,tasks_done=[0]*4)
        g['previous_position']=244;g['maps'][245][4]=103
        for id,actor in enumerate(g['actors']):
            actor['active']=int(id<1+index%10)
            actor['condition']=rng.choice(['OK','HS','EM'])
            actor['stats']=[0]+[rng.choice([0,10,255]) for _ in range(9)]
            actor['skills']=[0]+[rng.randrange(6) for _ in range(9)]
            actor['hp']=rng.choice([-32768,-1,0,1,32767]);actor['magic']=-1
        corpus.append((f'altar_profiles/{index}',g,ord('I'),'O',False))
    return corpus


def main():
    started=time.monotonic();image=SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:
        raise SystemExit('Empreinte originale differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,game,key,text,initial) in enumerate(corpus,1):
        machine=NavigationRoutine(image,game,key,text,initial)
        binary=machine.execute();c=execute_c(game,key,text,initial)
        if binary!=c:
            offset=next((i for i,p in enumerate(zip(binary,c)) if p[0]!=p[1]),min(len(binary),len(c)))
            diffs=[(i,a,b) for i,(a,b) in enumerate(zip(binary[offset],c[offset])) if a!=b] if offset<min(len(binary),len(c)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(binary),len(c)],diffs=diffs[:15]))
        boundaries+=len(c);calls+=len(machine.calls)
        if index%100==0:print(f'Navigation {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),
                first_mismatches=failures[:10],seconds=round(time.monotonic()-started,2),
                com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,
                scope='002E/0049, C7EF6/C7DBD, fixed monsters and real daily/combat; original RNG; presentation/BIOS bypassed')
    (HERE/'build/audit/navigation_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
