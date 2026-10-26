"""OVL00BC entier : achats, ventes, hopital et lectures, RANDOM x86 reel."""
import hashlib
import json
import random
import subprocess
import time
from compare_creation_binary import CreationRoutine,OVERLAYS,OVERLAY_HASH
from compare_exploration_binary import fixture,DATA,HERE,SOURCE,EXPECTED_HASH,STACK,STOP,word
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_IP
from exploration import pack_exploration


def pack_commerce(game):
    return pack_exploration(game,0,0)+' '.join(map(str,game['has_map']))+'\n'


class CommerceRoutine(CreationRoutine):
    def __init__(self,image,game,text):
        super().__init__(image,OVERLAYS.read_bytes(),game,list(map(ord,text)))
        self.cpu.mem_write(DATA+0x940C,bytes(game['has_map']))
        self.cpu.mem_write(0xBA21,self.overlays[0xBC00:0xCB00])

    def snapshot(self):return self.state(5)+list(self.cpu.mem_read(DATA+0x940C,11))

    def on_instruction(self,cpu,address,size,u):
        if address==0x4277:
            self.states.append(self.snapshot())
            if not self.keys:self.halted=True;cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key)
            self.return_from_hook(1);return
        if address==0xC549:return # Adresse interne du commerce, pas fin de creation.
        primitives={0xBA2F:0,0x6013:0,0x6031:0,0x6052:0,0x6073:0,0x609A:0,0x60BB:0,
            0x4544:10,0x46BC:10,0x4EFE:2}
        if address in primitives:self.return_from_hook(primitives[address]);return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xBA28,STOP,timeout=10000000,count=2000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:self.states.append(self.snapshot())
        elif not self.halted:raise RuntimeError(f'Commerce interrompu {self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(game,text):
    p=subprocess.run([str(HERE/'build/cmake/tera_commerce_bridge.exe')],input=pack_commerce(game)+
        ''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or f'Pont {p.returncode}')
    result=[];state=None
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='K':result.append(state+v[7:18])
    return result


def cases():
    result=[];rng=random.Random(1986)
    for text in ('','\x1b','\x1b\x1b','?\x1b','HN\x1b','AN\x1b','VN\x1b','AO','AO?A',
                 'AOA?\x1b\x1b','VOA?\x1b\x1b','HO\x1b'):
        g=fixture(1986);g['has_map']=[0]*11;result.append((repr(text),g,text))
    for key in 'ABCDEFGHIJKLMNOPQRS':
        for money in (-32768,0,99,500,32766,32767):
            for profile in (0,1,2):
                g=fixture(rng.getrandbits(32));g.update(money=money,has_map=[0]*11)
                g['actors'][0].update(stats=[10]*10,skills=[3]*10,equipment=[0 if profile==0 else 3]*10,
                    condition='HS' if profile==2 else 'OK')
                result.append((f'buy/{key}/{money}/{profile}',g,'AOA'+key+'\x1b\x1b'))
    for item in range(26):
        for seed in (0,1,1986):
            g=fixture(seed);g.update(money=32760,has_map=[0]*11)
            g['actors'][0]['equipment']=[item]*10
            result.append((f'sell/{item}/{seed}',g,'VOA09\x1b\x1b'))
    for index in range(80):
        g=fixture(rng.getrandbits(32));g.update(days=rng.choice([-32768,7300,32767]),has_map=[0]*11)
        for a in g['actors']:
            a.update(active=rng.randrange(2),hp=rng.choice([-32768,-1,0,1,32767]),magic=-1,
                hp_limit=rng.choice([-1,20,32767]),condition=rng.choice(['OK','EM','HS']),
                stats=[rng.randrange(256) for _ in range(10)])
        result.append((f'hospital/{index}',g,'HO\x1b'))
    return result


def main():
    image=SOURCE.read_bytes();started=time.monotonic()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:
        raise SystemExit('Empreinte originale differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,g,text) in enumerate(corpus,1):
        m=CommerceRoutine(image,g,text);a=m.execute();b=execute_c(g,text)
        if a!=b:
            offset=next((i for i,p in enumerate(zip(a,b)) if p[0]!=p[1]),min(len(a),len(b)))
            diff=[(i,x,y) for i,(x,y) in enumerate(zip(a[offset],b[offset])) if x!=y] if offset<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(a),len(b)],diff=diff[:12]))
        boundaries+=len(b);calls+=len(m.draw_bounds)
        if index%100==0:print(f'Commerce {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),
        first_mismatches=failures[:10],com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,
        seconds=round(time.monotonic()-started,2),scope='OVL00BC entier, C8315/C4D46/CAA13/C4CF9, lectures clavier et RANDOM originaux')
    (HERE/'build/audit/commerce_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
