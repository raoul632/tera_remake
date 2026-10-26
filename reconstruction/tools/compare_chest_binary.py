"""O/P dans les deux overlays, pieges et CAA13 : instructions originales executees."""
import hashlib
import json
import random
import subprocess
import time
from compare_navigation_binary import NavigationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,OVERLAY_HASH,STACK
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_BP
from navigation import pack_navigation


class ChestRoutine(NavigationRoutine):
    def __init__(self,image,game,command,text):
        super().__init__(image,game,command,text,False)
        self.outer_bp=None
        self.cpu.mem_write(DATA+0x940C,bytes(game.get('has_map',[0]*11)))
        self.cpu.mem_write(DATA+0x5E08,(HERE.parent/'data/compiled program/OBJETS').read_bytes())

    def snapshot(self,phase=0):
        opened=self.cpu.mem_read(self.outer_bp-3,1)[0] if self.outer_bp else self.game.get('chest_open',0)
        return CreationRoutine.state(self,5)+[opened]+list(self.cpu.mem_read(DATA+0x940C,11))+list(self.cpu.mem_read(DATA+0x75EE,7700))

    def on_instruction(self,cpu,address,size,u):
        if address==0x4277:
            sp=cpu.reg_read(UC_X86_REG_SP);caller=self.signed_word(sp)&65535
            if caller in (0xCC09,0xCEAB):
                if self.first_command:
                    self.first_command=False;self.outer_bp=cpu.reg_read(UC_X86_REG_BP)
                    self.cpu.mem_write(self.outer_bp-3,bytes([self.game.get('chest_open',0)]))
                    cpu.reg_write(UC_X86_REG_AX,self.command);self.return_from_hook(1);return
                self.states.append(self.snapshot());self.stop_reason='command';cpu.emu_stop();return
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key)
            self.return_from_hook(1);return
        super().on_instruction(cpu,address,size,u)


def execute_c(game,command,text):
    p=subprocess.run([str(HERE/'build/cmake/tera_navigation_bridge.exe')],
        input=pack_navigation(game,command)+''.join(f'{ord(k)}\n' for k in text),
        text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or f'Pont {p.returncode}')
    state=chest=None;maps=[row[:] for row in game['maps']];result=[]
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='G':chest=v
        elif tag=='B':maps[v[0]]=v[1:]
        elif tag=='Z':result.append(state+[chest[0]]+chest[6:]+[n for row in maps for n in row])
    return result


def chest_game(seed,overlay,content):
    g=fixture(seed);g.update(world_id=0 if not overlay else 2,position=45 if not overlay else 245,
        city_overlay=overlay,arioch_position=777,tasks_done=[0]*4,chest_open=0,has_map=[0]*11)
    g['previous_position']=g['position']-1;g['maps'][g['position']][4]=content
    return g


def cases():
    result=[]
    for overlay in (0,1):
        for content in (0,1,23,24,25,26,89,90,91,98,99,100,101,103,255):
            for opened in (0,1):
                if opened and content>=100:continue # Etat impossible : DOS ecrirait hors Keys.
                for command in 'OP':
                    for text in ('','N','OA','\r\x1bA','O?A'):
                        g=chest_game(1986,overlay,content);g['chest_open']=opened
                        result.append((f'content/{overlay}/{content}/{opened}/{command}/{text!r}',g,ord(command),text))
    # Profils faibles pour couvrir chaque piege et ses emissions aleatoires.
    for overlay in (0,1):
        for sound in (0,1):
            for seed in range(100):
                g=chest_game(seed,overlay,23);g['sound']=sound
                for i,a in enumerate(g['actors']):
                    a.update(active=int(i in (0,2,5,9)),condition='HS' if i==5 else 'EM' if i==2 else 'OK',
                        stats=[0]*10,skills=[0]*10,hp=1 if i in (0,9) else 30)
                result.append((f'trap/{overlay}/{sound}/{seed}',g,ord('O'),'OA'))
    for world in range(9):
        for content in (90,91,98,99):
            g=chest_game(1,1,content);g.update(world_id=world,position=world*100+45,chest_open=1)
            g['maps'][g['position']][4]=content
            result.append((f'key_map/{world}/{content}',g,ord('P'),'O'))
    rng=random.Random(1986)
    for index in range(120):
        g=chest_game(rng.getrandbits(32),index%2,rng.choice([1,23,24,26,89]))
        g.update(sound=index%2,chest_open=int(index%3==0),money=32767,difficulty=rng.randrange(11))
        for a in g['actors']:
            a.update(active=rng.randrange(2),condition=rng.choice(['OK','HS','EM']),hp=rng.choice([-32768,0,1,32767]),
                stats=[rng.randrange(256) for _ in range(10)],skills=[rng.randrange(6) for _ in range(10)],
                equipment=[rng.choice([0,18,23,24]) for _ in range(10)])
        g['actors'][0]['active']=1;g['actors'][5]['active']=1
        command='P' if g['chest_open'] else 'O'
        result.append((f'profile/{index}',g,ord(command),'OB'))
    return result


def main():
    started=time.monotonic();image=SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:
        raise SystemExit('Empreinte originale differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,g,command,text) in enumerate(corpus,1):
        m=ChestRoutine(image,g,command,text);a=m.execute();b=execute_c(g,command,text)
        if a!=b:
            offset=next((i for i,p in enumerate(zip(a,b)) if p[0]!=p[1]),min(len(a),len(b)))
            differences=[(i,x,y) for i,(x,y) in enumerate(zip(a[offset],b[offset])) if x!=y] if offset<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(a),len(b)],differences=differences[:15]))
        boundaries+=len(b);calls+=len(m.calls)
        if index%100==0:print(f'Coffres {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:10],
        com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,seconds=round(time.monotonic()-started,2),
        scope='002E/0049 O/P; original trap, explosion, sound RNG, gold, key/map and equipment writes; BP03 retained between commands')
    (HERE/'build/audit/chest_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
