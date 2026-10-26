"""N/S complets contre le resident COM : observer les indices lus, pas les calculer."""
import copy
import hashlib
import json
import random
import subprocess
import time
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,STACK,STOP,word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_IP,UC_X86_REG_DI
from exploration import pack_exploration,execute_operation
from compare_creation_binary import OVERLAYS,OVERLAY_HASH


class DialogueRoutine(ExplorationRoutine):
    def __init__(self,image,game,command,text):
        super().__init__(image,game,list(map(ord,text)),1,command)
        self.command=command;self.hints=[];self.response_id=-1;self.response_variant=0
        self.cpu.mem_write(DATA+0xA750,bytes([game.get('information_context',0)]))
        for i in range(38):
            raw=f'INDICE ORIGINAL {i+1}'.encode('ascii')
            self.cpu.mem_write(DATA+0x954A+40*i,bytes([len(raw)])+raw.ljust(39,b'\0'))

    def snapshot(self):
        return CreationRoutine.state(self,5)+[self.cpu.mem_read(DATA+a,1)[0] for a in (0xAFCE,0xA750)]+[len(self.hints)]+self.hints+[0]*(4-len(self.hints))+[self.response_id,self.response_variant]

    def on_instruction(self,cpu,address,size,u):
        if address==0x11CC:
            offset=cpu.reg_read(UC_X86_REG_DI)-0x954A
            if 0<=offset<38*40 and offset%40==0:self.hints.append(offset//40+1)
            for variant,base in enumerate((0x63C0,0x9C24,0xA14C)):
                offset=cpu.reg_read(UC_X86_REG_DI)-base
                if offset in (21*40,22*40):self.response_id=offset//40+1;self.response_variant=variant
        if address==0x4277:
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key);self.return_from_hook(1);return
        if address==0x4EFE:self.return_from_hook(2);return
        if address==0x47B3:self.return_from_hook(80);return
        if address==0x4210:self.return_from_hook();return
        if address==0x6073:self.cpu.mem_write(DATA+0xA750,b'\1');self.return_from_hook();return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP)+word(self.command));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xAFE8,STOP,timeout=10000000,count=2000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:self.states.append(self.snapshot())
        elif self.stop_reason!='input':raise RuntimeError(f'Dialogue interrompu {self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states

    def arrive_at_extra(self):
        """Executer le decor Amarande avant N/I, y compris l'ecriture A750."""
        self.mode=0;self.cpu.mem_write(DATA+0xA679,b'8')
        self.cpu.mem_write(0xBA21,OVERLAYS.read_bytes()[0xFB00:0x11300])
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xBA28,STOP,timeout=10000000,count=3000000)
        if self.cpu.reg_read(UC_X86_REG_IP)!=STOP:raise RuntimeError('Arrivee incomplete')
        self.mode=1


def execute_c(game,command,text):
    p=subprocess.run([str(HERE/'build/cmake/tera_dialogue_bridge.exe')],
        input=pack_exploration(game,0,command)+''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or f'Pont {p.returncode}')
    state=None;result=[]
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='T':result.append(state+[v[3],v[19],v[7]]+v[8:12]+v[24:26])
    return result


def cases():
    result=[]
    for choice in 'IAVDS?\x1b':
        for answer in ('','N','O','\r','\x1b'):
            for ready in (0,1):
                g=fixture(1986);g['recruit_ready']=ready
                result.append((f'menu/{choice!r}/{answer!r}/{ready}',g,ord('N'),choice+answer+'A0A' if choice=='D' and answer in ('O','\r') else choice+answer))
    for id in range(1,10):
        for context in (0,1):
            for active in (0,1):
                for condition in ('OK','HS'):
                    g=fixture(id);g.update(information_context=context,recruit_ready=1)
                    g['extra_locations'][id]=g['position'];g['locations'][id]=g['position']
                    g['actors'][id].update(active=active,condition=condition)
                    result.append((f'information/{id}/{context}/{active}/{condition}',g,ord('N'),'IO'))
    for seed in range(100):
        g=fixture(seed);g.update(information_context=1)
        for id in (1,2,7):g['locations'][id]=g['position'];g['extra_locations'][id]=g['position']
        result.append((f'indice_rng/{seed}',g,ord('N'),'IO'))
    for id in (1,2,4,6,7,9):
        for seed in range(4):
            g=fixture(seed);g.update(position=999,information_context=0)
            g['extra_locations'][id]=1000
            result.append((f'move_information/{id}/{seed}',g,ord('N'),'IO'))
    rng=random.Random(1986)
    for index in range(100):
        g=fixture(rng.getrandbits(32));g.update(recruit_ready=index%2,money=rng.choice([0,249,250,32767]),experience=32767)
        for i,a in enumerate(g['actors']):
            a.update(active=int(i<index%6),condition=rng.choice(['OK','EM','HS']),
                stats=[rng.randrange(256) for _ in range(10)],skills=[rng.randrange(6) for _ in range(10)])
            g['locations'][i]=g['position'] if i%2 else 0
        result.append((f'solicit/{index}',g,ord('N'),'SO'))
        result.append((f'seduce/{index}',g,ord('S'),'O'))
    return result


def main():
    started=time.monotonic();image=SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:
        raise SystemExit('Empreinte originale differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,g,command,text) in enumerate(corpus,1):
        m=DialogueRoutine(image,g,command,text)
        if name.startswith('move_information/'):
            m.arrive_at_extra();g=copy.deepcopy(g);execute_operation(g,0,ord('8'))
        a=m.execute();b=execute_c(g,command,text)
        if a!=b:
            offset=next((i for i,p in enumerate(zip(a,b)) if p[0]!=p[1]),min(len(a),len(b)))
            diff=[(i,x,y) for i,(x,y) in enumerate(zip(a[offset],b[offset])) if x!=y] if offset<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(a),len(b)],diff=diff[:12]))
        boundaries+=len(b);calls+=len(m.calls)
        if index%100==0:print(f'Dialogues {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:10],
        com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,seconds=round(time.monotonic()-started,2),
        scope='CAFE8 N/S, C_AFEF, OVL00FB arrival; actual hint/response addresses observed; original menus, recruitment and RNG')
    (HERE/'build/audit/dialogue_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
