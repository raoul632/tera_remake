"""OVL00CB : Real48 calcule par le runtime Pascal original, musique et pause.

Seules les sorties texte/video sont abstraites ; les operations flottantes
du COM sont executees. Comparer les six octets de score, pas un arrondi Python.
"""
import hashlib
import json
import random
import subprocess
import time
from compare_creation_binary import CreationRoutine,OVERLAYS,OVERLAY_HASH
from compare_exploration_binary import fixture,DATA,HERE,SOURCE,EXPECTED_HASH,STACK,STOP,word
from unicorn.x86_const import UC_X86_REG_SP,UC_X86_REG_IP,UC_X86_REG_BX,UC_X86_REG_DX
from ending import pack_ending


class EndingRoutine(CreationRoutine):
    def __init__(self,image,g,text):
        super().__init__(image,OVERLAYS.read_bytes(),g,list(map(ord,text)))
        self.cpu.mem_write(0xBA21,self.overlays[0xCB00:0xD200]);self.error=0
        self.cpu.mem_write(DATA+0xA74A,bytes([g['sound']]))
        self.data_word(0x75E8,g['difficulty']);self.raw=[0]*6;self.formatted=''

    def snapshot(self,done=0):
        return CreationRoutine.state(self,5)+[done,int(bool(self.game['party_available'])),self.error]+self.raw+list(self.formatted.encode('ascii').ljust(32,b'\0'))

    def on_instruction(self,cpu,address,size,u):
        if address==0xBFCD:self.raw=list(cpu.mem_read(STACK-10,6))
        if address==0x2904:
            self.formatted=bytes(cpu.mem_read(DATA+0xB6,cpu.reg_read(UC_X86_REG_BX)-0xB6)).decode('ascii').strip()
            cpu.reg_write(UC_X86_REG_SP,cpu.reg_read(UC_X86_REG_SP)+2);self.return_from_hook();return
        if address==0x103C:
            self.error=cpu.reg_read(UC_X86_REG_DX)&255;self.states.append(self.snapshot(1));self.halted=True;cpu.emu_stop();return
        if address==0x4210:
            self.states.append(self.snapshot())
            if not self.keys:self.halted=True;cpu.emu_stop();return
            self.keys.pop(0);self.return_from_hook();return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xBA28,STOP,timeout=10000000,count=3000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:self.states.append(self.snapshot(1))
        elif not self.halted:raise RuntimeError(f'Ending IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(g,text):
    p=subprocess.run([str(HERE/'build/cmake/tera_ending_bridge.exe')],input=pack_ending(g)+
        ''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or str(p.returncode))
    result=[];state=None;flags=None
    for line in p.stdout.splitlines():
        tag,*v=line.split()
        if tag=='S':state=list(map(int,v))
        elif tag=='K':flags=list(map(int,v))
        elif tag=='R':result.append(state+flags+list(('' if v[1]=='none' else v[1]).encode('ascii').ljust(32,b'\0')))
    return result


def cases():
    result=[];rng=random.Random(1986)
    for difficulty in range(10):
        for days in (-32768,-30000,-1,0,3999,4001,4003,4240,7300,32767):
            for exp in (-32768,-1,0,1,32767):
                g=fixture(rng.getrandbits(32));g.update(tasks_done=[1]*4,difficulty=difficulty,days=days,experience=exp,sound=0)
                result.append((f'score/{difficulty}/{days}/{exp}',g,'O'))
    for sound in (0,1):
        for alive in (0,1):
            for text in ('','X'):
                g=fixture(1986);g.update(tasks_done=[1]*4,sound=sound,party_available=alive)
                result.append((f'ending/{sound}/{alive}/{text}',g,text))
        g=fixture(1986);g.update(tasks_done=[1]*4,sound=sound,days=4000)
        result.append((f'division_zero/{sound}',g,''))
    return result


def main():
    image=SOURCE.read_bytes();started=time.monotonic()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:raise SystemExit('Empreinte differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,g,text) in enumerate(corpus,1):
        m=EndingRoutine(image,g,text);a=m.execute();b=execute_c(g,text)
        if a!=b:
            at=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            diffs=[(i,x,y) for i,(x,y) in enumerate(zip(a[at],b[at])) if x!=y] if at<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=at,lengths=[len(a),len(b)],diffs=diffs[:12]))
        boundaries+=len(b);calls+=len(m.draw_bounds)
        if index%100==0:print(f'Fin {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),input_boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:15],seconds=round(time.monotonic()-started,2),com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,scope='OVL00CB score Real48 bytes, victory/death, original arithmetic and sound RNG; text/video abstracted')
    (HERE/'build/audit/ending_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
