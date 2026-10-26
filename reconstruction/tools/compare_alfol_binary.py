"""OVL0085 x86 : surface, batiments, portes, medias, radiation et rencontres.

Reprise a la boucle de commandes pour tester chaque position ; seule la
premiere initialisation/rendu est sautee lors d'une reprise explicite.
Les rendus consomment leurs vrais RANDOM, ainsi que le jour et les combats.
"""
import copy
import hashlib
import json
import subprocess
import time
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,OVERLAY_HASH,STACK,STOP,word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_IP
from alfol import pack_alfol


class AlfolRoutine(ExplorationRoutine):
    def __init__(self,image,g,key,text,initial):
        super().__init__(image,g,list(map(ord,text)),1,key)
        self.command=key;self.initial=initial;self.first=True
        self.inner=g['world_id']==9;self.started=False
        self.data_word(0x9B66,g.get('alfol_status_actor',9));self.data_word(0x75DC,g['tasks_counter'])
        self.cpu.mem_write(DATA+0x9419,bytes(g['tasks_done']))
        hint=g.get('hints',[''])[0].encode('cp437')
        self.cpu.mem_write(DATA+0x954A,bytes([len(hint)])+hint.ljust(39,b'\0'))
        self.cpu.mem_write(0xBA21,OVERLAYS.read_bytes()[0x8500:0xA400])

    def snapshot(self):
        return CreationRoutine.state(self,5)+[self.cpu.mem_read(DATA+a,1)[0] for a in
            (0xA748,0xA750,0xAFD2,0xAFCE,0xAFD1)]+[self.signed_word(DATA+0x9B72),
            self.signed_word(DATA+0x75DC),self.cpu.mem_read(DATA+0x941A,1)[0]]+list(self.cpu.mem_read(DATA+0x75EE,7700))+list(self.cpu.mem_read(DATA+0x954B,39))+[self.signed_word(DATA+0x9B66)]

    def state(self,*args):return self.snapshot()

    def main_read(self):
        if self.initial or not self.first:
            self.states.append(self.snapshot());self.stop_reason='command';self.cpu.emu_stop();return None
        self.first=False;return self.command

    def on_instruction(self,cpu,address,size,u):
        if address==0xD70E and not self.initial:cpu.reg_write(UC_X86_REG_IP,0xD729);return
        if address==0xCCCA and self.inner and not self.started:
            self.started=True;cpu.reg_write(UC_X86_REG_IP,0xCCE5);return
        if address==0xCD09:
            key=self.main_read()
            if key is not None:cpu.mem_write(DATA+0xA679,bytes([key]));cpu.reg_write(UC_X86_REG_IP,0xCD1A)
            return
        if address==0x4277:
            caller=self.signed_word(cpu.reg_read(UC_X86_REG_SP))&65535
            if caller==0xD751:
                key=self.main_read()
                if key is not None:cpu.reg_write(UC_X86_REG_AX,key);self.return_from_hook(1)
                return
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key);self.return_from_hook(1);return
        if address==0x4210:self.return_from_hook();return
        if address==0x609A:self.return_from_hook();return
        if address==0x47B3:self.return_from_hook(80);return
        if address==0x4EFE:self.return_from_hook(2);return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xCCBE if self.inner else 0xBA28,STOP,timeout=10000000,count=4000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:
            if self.inner:self.data_word(0x9B76,11) # Affectation D7CA du seul appelant.
            self.states.append(self.snapshot())
        elif self.stop_reason not in ('input','command'):raise RuntimeError(f'Alfol IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(g,key,text,initial):
    p=subprocess.run([str(HERE/'build/cmake/tera_alfol_bridge.exe')],input=pack_alfol(g,key,initial)+
        ''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=15)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or str(p.returncode))
    result=[];state=None;maps=copy.deepcopy(g['maps']);flags=None
    hint=list(g.get('hints',[''])[0].encode('cp437').ljust(40,b'\0'))
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='Y':flags=v
        elif tag=='B':maps[v[0]]=v[1:]
        elif tag=='H':hint=v
        elif tag=='Z':result.append(state+[flags[i] for i in (2,4,5,6,7,8,9,10)]+[x for row in maps for x in row]+hint[:39]+[flags[3]])
    return result


def game(seed=1986):
    g=fixture(seed);g.update(on_alfol=1,world_id=11,position=45,sound=0,tasks_counter=0,tasks_done=[0]*4)
    return g


def cases():
    result=[]
    for position in range(100):
        g=game(position);g['position']=position
        for key in '2468':result.append((f'surface/{position}/{key}',g,ord(key),'FOFOFO',False))
    for seed in range(8):
        for sound in (0,1):
            g=game(seed);g.update(sound=sound,fast_moves=seed%2)
            result.append((f'initial/{seed}/{sound}',g,0,'FOFOFO',True))
    for key in 'ERI':
        for pos in (0,45,75):
            for text in ('','N','O','\r'):
                g=game();g['position']=pos;g['maps'][900]=[0,0,1,0,0,0,1]
                result.append((f'confirm/{key}/{pos}/{text!r}',g,ord(key),text+'FOFOFO' if text else '',False))
    for heading in (1,-1,10,-10):
        for key in '2468':
            for wall in range(4):
                g=game();g.update(world_id=9,position=945,previous_position=945-heading)
                g['maps'][945][:4]=[wall]*4
                result.append((f'grid/{heading}/{key}/{wall}',g,ord(key),'OUFOFO',False))
    for content in (0,1,2):
        for sound in (0,1):
            for money in (999,1000,32767):
                for done in (0,1):
                    g=game();g.update(world_id=9,position=944,previous_position=943,sound=sound,money=money,tasks_counter=32767)
                    g['tasks_done'][1]=done;g['maps'][944][0]=1;g['maps'][945][4]=content
                    result.append((f'building/{content}/{sound}/{money}/{done}',g,ord('8'),'OFOFO',False))
    for key in 'DML OP':
        g=game();result.append((f'generic/{key}',g,ord(key),'',False))
    for sound in (0,1):
        g=game();g.update(world_id=9,position=900,previous_position=899,sound=sound);g['maps'][900][2]=1
        result.append((f'exit/{sound}',g,ord('2'),'FOFO',False))
    for actor in range(10):
        for sound in (0,1):
            g=game();g.update(world_id=9,position=944,previous_position=943,sound=sound,alfol_status_actor=actor)
            g['maps'][944][0]=1;g['maps'][945][4:]=[2,12,1]
            for a in g['actors']:a.update(active=1,hp=1,condition='EM')
            result.append((f'radiation_index/{actor}/{sound}',g,ord('8'),'FOFO',False))
    for seed in range(12):
        g=game(seed);g.update(world_id=9,position=944,previous_position=943)
        g['maps'][944][0]=1;g['maps'][945][5]=1
        result.append((f'fixed_flee/{seed}',g,ord('8'),'FO'*30,False))
        g=copy.deepcopy(g);g['actors'][0].update(hp=10000,magic=200,stats=[0]+[10]*9,skills=[0]+[3]*9)
        result.append((f'fixed_victory/{seed}',g,ord('8'),'TO'*50,False))
    for npc in (7,9):
        g=game();g.update(world_id=9,position=944,previous_position=943)
        g['locations'][npc]=945;g['maps'][944][0]=1
        result.append((f'npc/{npc}',g,ord('8'),'FOFO',False))
    for sound in (0,1):
        g=game();g['sound']=sound
        for a in g['actors']:a.update(active=0,hp=0,condition='HS')
        result.append((f'initial_dead/{sound}',g,0,'',True))
    for magic in (-32768,-1,0,1,32767):
        g=game();g.update(world_id=9,position=944,previous_position=943,alfol_status_actor=16)
        g['hints']=['0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'];g['actors'][6]['magic']=magic
        g['maps'][944][0]=1;g['maps'][945][4]=2
        result.append((f'radiation_alias16/{magic}',g,ord('8'),'FOFO',False))
    return result


def main():
    image=SOURCE.read_bytes();started=time.monotonic()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:raise SystemExit('Empreinte originale differente')
    failures=[];calls=boundaries=0;corpus=cases()
    for index,(name,g,key,text,initial) in enumerate(corpus,1):
        m=AlfolRoutine(image,g,key,text,initial);a=m.execute();b=execute_c(g,key,text,initial)
        if a!=b:
            offset=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            diffs=[(i,x,y) for i,(x,y) in enumerate(zip(a[offset],b[offset])) if x!=y] if offset<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(a),len(b)],diffs=diffs[:12]))
        calls+=len(m.calls);boundaries+=len(b)
        if index%100==0:print(f'Alfol {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),input_boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:20],seconds=round(time.monotonic()-started,2),com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,scope='OVL0085 command loop and initial arrival; full render RNG, day and nested combat; video/BIOS abstracted')
    (HERE/'build/audit/alfol_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
