"""Executer la boucle TERAMAIN, OVL00CB defaite et OVL0022, RNG reel.

Le chargeur DOS est remplace par les octets de l'overlay demande. Les
branches des objectifs et l'affectation A748 restent des instructions x86.
"""
import hashlib
import json
import random
import subprocess
import time
from compare_creation_binary import CreationRoutine,OVERLAYS,OVERLAY_HASH
from compare_exploration_binary import fixture,DATA,HERE,SOURCE,EXPECTED_HASH,STACK,word
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_DX,UC_X86_REG_SP,UC_X86_REG_IP
from lifecycle import pack_lifecycle


class LifeRoutine(CreationRoutine):
    def __init__(self,image,overlays,game,text):
        super().__init__(image,overlays,game,list(map(ord,text)))
        self.overlay=0;self.life=0;self.finished=False
        # Copier le corps reloge, comme le deplacement de TERAMAIN a 0CCC.
        self.cpu.mem_write(0xDA21,image[0xDA21-0x20FC:])
        self.cpu.mem_write(DATA+0x9419,bytes(game['tasks_done']))
        for key,a in [('sound',0xA74A),('on_alfol',0xA748),('fled',0xAFCF),
                      ('victory',0xA746),('recruit_ready',0xAFCE)]:
            self.cpu.mem_write(DATA+a,bytes([game.get(key,0)]))

    def snapshot(self):
        return self.state(5)+[self.life]+[self.cpu.mem_read(DATA+a,1)[0] for a in (0xA748,0xAFCF,0xAFCE,0xA746)]

    def on_instruction(self,cpu,address,size,u):
        if address==0xBA0F:
            self.overlay=cpu.reg_read(UC_X86_REG_DX)
            start,end={0xCB:(0xCB00,0xD200),0x22:(0x2200,0x2E00)}[self.overlay]
            cpu.mem_write(0xBA21,self.overlays[start:end]);cpu.ctl_remove_cache(0xBA21,0xDA21)
            cpu.reg_write(UC_X86_REG_IP,0xBA28);return
        if address==0x4210 and self.overlay==0xCB:
            self.life=0;self.states.append(self.snapshot())
            if not self.keys:self.halted=True;cpu.emu_stop();return
            self.keys.pop(0);self.return_from_hook();return
        if address==0x4277:
            self.life=1;self.states.append(self.snapshot())
            if not self.keys:self.halted=True;cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key)
            self.return_from_hook(1);return
        if address in (0xDBE2,0xE7C0):
            self.life=2 if address==0xDBE2 else 3
            self.states.append(self.snapshot());self.finished=True;cpu.emu_stop();return
        if address==0xC549:return # Laisser revenir OVL0022 dans TERAMAIN.
        if address in (0x6013,0x6031,0x6052,0x6073,0x60BB):self.return_from_hook();return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(0xF100));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xE790,0xF100,timeout=10000000,count=3000000)
        if not self.halted and not self.finished:raise RuntimeError(f'Cycle interrompu {self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(game,text):
    p=subprocess.run([str(HERE/'build/cmake/tera_lifecycle_bridge.exe')],
        input=pack_lifecycle(game)+''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or f'Pont {p.returncode}')
    state=None;result=[]
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v;state[0]=5
        elif tag=='L':result.append(state+[v[0]]+v[6:10])
    return result


def cases():
    result=[];rng=random.Random(1986)
    for index in range(120):
        g=fixture(rng.getrandbits(32));g.update(party_available=0,arrival_pending=0,
            money=rng.choice([-32768,0,499,500,32767]),experience=12345,days=-4,level=12,dummy_level=7,
            on_alfol=1,sound=index%2,fled=index%2,victory=(index//2)%2,recruit_ready=(index//3)%2,
            tasks_done=[int(bool(index%16 & (1<<i))) for i in range(4)])
        g['keys']=[rng.randrange(-5,255) for _ in range(10)]
        for i,a in enumerate(g['actors']):
            a.update(active=rng.randrange(2),hp=0,magic=-1,hp_limit=333,magic_limit=444,
                condition=rng.choice(['OK','HS','EM']),name='OLD'+str(i),
                stats=[rng.randrange(256) for _ in range(10)],skills=[rng.randrange(6) for _ in range(10)],
                equipment=[rng.randrange(26) for _ in range(10)])
        text='\rALIX\rH333FIVEHAO'
        if index%16==15:text='\r'
        elif index%11==0:text=''
        elif index%11==1:text='\rAL'
        elif index%11==2:text='\rFIRST\rH333FIVEHANSECOND\rF999NNNNNNO'
        result.append((f'cycle/{index}',g,text))
    return result


def main():
    started=time.monotonic();image=SOURCE.read_bytes();raw=OVERLAYS.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(raw).hexdigest()!=OVERLAY_HASH:
        raise SystemExit('Empreinte originale differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for name,g,text in corpus:
        m=LifeRoutine(image,raw,g,text);a=m.execute();b=execute_c(g,text)
        if a!=b:
            offset=next((i for i,p in enumerate(zip(a,b)) if p[0]!=p[1]),min(len(a),len(b)))
            diff=[(i,x,y) for i,(x,y) in enumerate(zip(a[offset],b[offset])) if x!=y] if offset<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(a),len(b)],diff=diff[:12]))
        boundaries+=len(b);calls+=len(m.draw_bounds)
    report=dict(cases=len(corpus),boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),
        first_mismatches=failures[:10],com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,
        seconds=round(time.monotonic()-started,2),scope='TERAMAIN E790..E7C0/DBAC..DBE2; death OVL00CB, successor OVL0022; score excluded')
    (HERE/'build/audit/lifecycle_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
