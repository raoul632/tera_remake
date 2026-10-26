"""CAA13 et commandes d'objets : vraies instructions COM, memes touches/rosters."""
import copy
import hashlib
import json
import random
import subprocess
import time
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,STACK,STOP,word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_IP
from inventory import pack_inventory


class InventoryRoutine(ExplorationRoutine):
    def __init__(self,image,game,command,text,acquire=False,item=0):
        super().__init__(image,game,list(map(ord,text)),1,command)
        self.command=command;self.acquire=acquire;self.item=item
        for name,address in [('OBJETS',0x5E08)]:
            self.cpu.mem_write(DATA+address,(HERE.parent/'data/compiled program'/name).read_bytes())

    def snapshot(self):
        return CreationRoutine.state(self,5)+[self.cpu.mem_read(DATA+0xA675,1)[0]]

    def on_instruction(self,cpu,address,size,u):
        if address==0x4277:
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0)
            cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key);self.return_from_hook(1);return
        if address==0x4EFE:self.return_from_hook(2);return # Texte des dix slots, sans regle/RNG.
        if address==0x47B3:self.return_from_hook(80);return
        if address==0x4210:self.return_from_hook();return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        if self.acquire:
            self.cpu.mem_write(DATA+0xA675,bytes([self.item]));start=0xAA13;argument=self.command
        else:start=0xAFE8;argument=self.command
        self.cpu.mem_write(STACK,word(STOP)+word(argument));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(start,STOP,timeout=10000000,count=2000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:self.states.append(self.snapshot())
        elif self.stop_reason!='input':raise RuntimeError(f'Inventaire interrompu IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(game,command,text,acquire=False,item=0):
    p=subprocess.run([str(HERE/'build/cmake/tera_inventory_bridge.exe')],
        input=pack_inventory(game,command,acquire,item)+''.join(f'{ord(k)}\n' for k in text),
        text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or f'Pont {p.returncode}')
    result=[];state=None
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='I':result.append(state+[v[4]])
    return result


def cases():
    result=[]
    for actor in range(10):
        for condition in ('OK','EM','HS'):
            for free in range(11):
                g=fixture(1986);g['actors'][actor]['condition']=condition
                g['actors'][actor]['equipment']=[18]*10
                if free<10:g['actors'][actor]['equipment'][free]=0
                result.append((f'acquire/{actor}/{condition}/{free}',g,actor,'',True,23))
    for text in ('','O','OA','OA0','OC','OCB','OCZ','OX','OX0','OZA0','O\x1b', 'C','CA','CB','CC','CD','CZBA','?'):
        g=fixture(23)
        for i in (0,2,5,9):g['actors'][i]['active']=1
        result.append((f'drop/{text!r}',g,ord('A'),text,False,0))
    for item in range(26):
        for condition in ('OK','EM','HS'):
            for subject in ('A','B'):
                g=fixture(1986);g['actors'][5]['active']=1
                for i in (0,5):g['actors'][i].update(condition=condition,hp=1,magic=0,equipment=[item]+[0]*9)
                result.append((f'drink/{item}/{condition}/{subject}',g,ord('B'),subject+'?\x1b0',False,0))
    for text in ('','D','DN','DO','DOA','DOA0','DOA0B','DOA0A','DOA0ZB','DOA?0B','DOZA0B','DO\x1bA0B'):
        for full in (0,1):
            for hs in (0,1):
                g=fixture(1);g['actors'][5]['active']=1
                g['actors'][5]['equipment']=[3]*10 if full else [0]*10
                g['actors'][5]['condition']='HS' if hs else 'EM'
                result.append((f'give/{text!r}/{full}/{hs}',g,ord('N'),text,False,0))
    rng=random.Random(1986)
    for index in range(80):
        g=fixture(rng.getrandbits(32))
        for actor in g['actors']:
            actor.update(active=rng.randrange(2),condition=rng.choice(['OK','EM','HS']),
                stats=[rng.randrange(256) for _ in range(10)],skills=[rng.randrange(6) for _ in range(10)],
                equipment=[rng.randrange(26) for _ in range(10)],hp=rng.randrange(-10,40),magic=-1)
        g['actors'][0]['active']=1;g['actors'][5]['active']=1
        key,text=rng.choice([(ord('B'),'A0'),(ord('A'),'OA9'),(ord('N'),'DOA0B')])
        result.append((f'profile/{index}',g,key,text,False,0))
    return result


def main():
    started=time.monotonic();image=SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH:raise SystemExit('Empreinte COM differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,g,command,text,acquire,item) in enumerate(corpus,1):
        m=InventoryRoutine(image,g,command,text,acquire,item);a=m.execute();b=execute_c(g,command,text,acquire,item)
        if a!=b:
            offset=next((i for i,p in enumerate(zip(a,b)) if p[0]!=p[1]),min(len(a),len(b)))
            differences=[(i,x,y) for i,(x,y) in enumerate(zip(a[offset],b[offset])) if x!=y] if offset<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(a),len(b)],differences=differences[:12]))
        boundaries+=len(b);calls+=len(m.calls)
        if index%100==0:print(f'Inventaire {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),
        first_mismatches=failures[:10],com_sha256=EXPECTED_HASH,seconds=round(time.monotonic()-started,2),
        scope='CAA13, A/O,C, B, N/D; original menus, character/slot readers, effects and RNG; text/BIOS abstracted')
    (HERE/'build/audit/inventory_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
