"""OVL00AE execute : carburant, tirs imposes au dernier poll, explosions et HALT.

Les seuls sauts remplacent le chargement du bitmap DOS et les ondes sans
RANDOM. Les touches sont injectees au poll final ; les purges precedentes
voient un clavier vide. Aucun calcul spatial du port n'est utilise ici.
"""
import hashlib
import json
import subprocess
import time
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,OVERLAY_HASH,STACK,STOP,word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_IP,UC_X86_REG_BP,UC_X86_REG_EFLAGS
from exploration import pack_exploration


class SpaceRoutine(ExplorationRoutine):
    def __init__(self,image,g,keys,pirates):
        super().__init__(image,g,keys,1,0)
        self.data_word(0x9B6C,pirates);self.cpu.mem_write(DATA+0xA74E,b'\1')
        self.cpu.mem_write(0xBA21,OVERLAYS.read_bytes()[0xAE00:0xBC00])
        self.pending=0;self.halt=False

    def snapshot(self,shot=False):
        # Counter et coordonnees ne sont definis qu'apres le ravitaillement.
        result=CreationRoutine.state(self,5)+[self.signed_word(DATA+0x9B6C)]
        if shot:result += [self.signed_word(DATA+0x9B7C),self.signed_word(STACK-2-12),self.signed_word(STACK-2-14)]
        return result

    def on_instruction(self,cpu,address,size,u):
        if address==0xC3A8:cpu.reg_write(UC_X86_REG_IP,0xC406);return
        if address in (0x666B,0x6721):self.return_from_hook();return
        if address==0x609A:self.return_from_hook();return
        if address==0x0DA4:
            caller=self.signed_word(cpu.reg_read(UC_X86_REG_SP))&65535
            value=0
            if caller==0xBAFD:
                self.states.append(self.snapshot(True))
                if not self.keys:self.stop_reason='input';cpu.emu_stop();return
                self.pending=self.keys.pop(0);value=int(self.pending!=0)
            cpu.reg_write(UC_X86_REG_AX,value)
            flags=cpu.reg_read(UC_X86_REG_EFLAGS)
            cpu.reg_write(UC_X86_REG_EFLAGS,flags&~64 if value else flags|64)
            self.return_from_hook();return
        if address==0xBB05:
            parent=self.signed_word(cpu.reg_read(UC_X86_REG_BP)-2)&65535
            cpu.mem_write(parent-18,bytes([self.pending]));cpu.reg_write(UC_X86_REG_IP,0xBB1A);return
        if address==0x4277:
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key)
            self.return_from_hook(1);return
        if address==0x0C89:
            self.halt=True;self.stop_reason='halt';self.states.append(self.snapshot());cpu.emu_stop();return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xBA28,STOP,timeout=20000000,count=6000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:self.states.append(self.snapshot())
        elif self.stop_reason not in ('input','halt'):raise RuntimeError(f'Space IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(g,keys,pirates):
    p=subprocess.run([str(HERE/'build/cmake/tera_space_bridge.exe')],input=pack_exploration(g,0,0)+
        f'{pirates} 0\n'+''.join(f'{k}\n' for k in keys),text=True,capture_output=True,timeout=15)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or str(p.returncode))
    result=[];state=None;halt=False
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='X':
            snapshot=state+[v[1]]
            if v[0]==3:snapshot += [v[2],v[6],v[7]]
            result.append(snapshot);halt=v[0]==5
    return result,halt


def cases():
    result=[]
    for seed in range(16):
        for sound in (0,1):
            for pirates in (0,1,4,15):
                g=fixture(seed);g.update(sound=sound,energy=250,on_alfol=1)
                result.append((f'flight/{seed}/{sound}/{pirates}',g,pirates,[ord('N')]+list(map(ord,'123456789'*3))))
    for money in (-32768,-1,0,49,50,449,450,32766,32767):
        for digit in ('0','1','9'):
            g=fixture(1986);g.update(money=money,energy=5,sound=0,on_alfol=1)
            result.append((f'fuel/{money}/{digit}',g,4,list(map(ord,'O?\x1b'+digit))+[0]*8))
    for energy in (-32768,0,4,5,9,10,25,26,32767):
        for sound in (0,1):
            g=fixture(1986);g.update(energy=energy,sound=sound,days=32767,on_alfol=1)
            result.append((f'destruction/{energy}/{sound}',g,4,[13]+[0]*8))
    return result


class PilotRoutine(ExplorationRoutine):
    """TERAMAIN E222 : executer la selection puis s'arreter avant OVL00AE."""
    def __init__(self,image,g,text):
        super().__init__(image,g,list(map(ord,text)),1,ord('E'))
        self.cpu.mem_write(0xDA21,image[0xDA21-0x20FC:]);self.departed=False
        self.data_word(0x9B6C,4);self.data_word(0xAFD5,g['position'])

    def snapshot(self):
        return CreationRoutine.state(self,5)+[self.signed_word(DATA+0x9B6C),
            self.signed_word(DATA+0xAFD5),self.cpu.mem_read(DATA+0xA748,1)[0]]

    def on_instruction(self,cpu,address,size,u):
        if address==0xBA0F:
            self.departed=True;self.stop_reason='flight';self.states.append(self.snapshot());cpu.emu_stop();return
        if address==0x4277:
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key);self.return_from_hook(1);return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xE222,0xE405,timeout=10000000,count=1000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==0xE405:self.states.append(self.snapshot())
        elif self.stop_reason not in ('input','flight'):raise RuntimeError(f'Pilot IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def compare_pilots(image):
    failures=[];boundaries=calls=count=0
    for seed in range(24):
        for profile in range(5):
            g=fixture(seed);g.update(position=1450,on_alfol=0)
            for i,a in enumerate(g['actors']):
                a['active']=int(profile!=0 and (profile!=1 or i==0))
                if profile==2:a['condition']='HS'
                if profile==3:a.update(stats=[0]*10,skills=[0]*10)
                if profile==4:a.update(stats=[255]*10,skills=[255]*10)
            text='' if seed==0 else 'N' if seed==1 else '\r' if seed==2 else 'O'
            m=PilotRoutine(image,g,text);a=m.execute()
            p=subprocess.run([str(HERE/'build/cmake/tera_space_bridge.exe')],input=pack_exploration(g,0,1)+
                f"4 {g['position']}\n"+''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
            if p.returncode not in (0,4):raise RuntimeError(f'Pilot C {p.returncode}')
            b=[];state=None
            for line in p.stdout.splitlines():
                tag,*v=line.split();v=list(map(int,v))
                if tag=='S':state=v
                elif tag=='X':b.append(state+[v[1],v[3],v[11]])
            if a!=b:failures.append(dict(seed=seed,profile=profile,lengths=[len(a),len(b)],diffs=[(i,x,y) for i,(x,y) in enumerate(zip(a[-1],b[-1])) if x!=y][:10]))
            calls+=len(m.calls);boundaries+=len(b);count+=1
    return dict(cases=count,input_boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:10])


def main():
    image=SOURCE.read_bytes();started=time.monotonic()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:raise SystemExit('Empreinte originale differente')
    failures=[];calls=boundaries=0;corpus=cases()
    for index,(name,g,pirates,keys) in enumerate(corpus,1):
        m=SpaceRoutine(image,g,keys,pirates);a=m.execute();b,halt=execute_c(g,keys,pirates)
        if a!=b or halt!=m.halt:
            offset=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            diffs=[(i,x,y) for i,(x,y) in enumerate(zip(a[offset],b[offset])) if x!=y] if offset<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(a),len(b)],diffs=diffs[:12],halt=[m.halt,halt]))
        calls+=len(m.calls);boundaries+=len(b)
        if index%50==0:print(f'Spatial {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),input_boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:12],seconds=round(time.monotonic()-started,2),com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,scope='OVL00AE game mode; final shot poll with imposed keys, earlier polls empty; bitmap/video/deterministic sound abstracted')
    report['pilot_selection']=compare_pilots(image)
    (HERE/'build/audit/space_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures or report['pilot_selection']['mismatches']:raise SystemExit(1)


if __name__=='__main__':main()
