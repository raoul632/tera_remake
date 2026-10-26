"""OVL00A4 : vraies instructions, menus, meditation, invocation et rencontres."""
import copy
import hashlib
import json
import subprocess
import time
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,OVERLAY_HASH,STACK,STOP,word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_IP,UC_X86_REG_DX
from meduz import pack_meduz


class MeduzRoutine(ExplorationRoutine):
    def __init__(self,image,g,key,text,initial):
        super().__init__(image,g,list(map(ord,text)),1,key)
        self.command=key;self.initial=initial;self.first=True
        self.cpu.mem_write(0xBA21,OVERLAYS.read_bytes()[0xA400:0xAE00])
        self.cpu.mem_write(DATA+0x9418,bytes([g.get('mental_extension',0),*g['tasks_done']]))
        self.data_word(0x75DC,g['tasks_counter']);self.data_word(0xAFD5,g.get('meduz_backup_position',1500))
        for i,value in enumerate(g['secret_words']):
            raw=value.encode('cp437');self.cpu.mem_write(DATA+0x9B3A+10*i,bytes([len(raw)])+raw.ljust(9,b'\0'))

    def snapshot(self):
        return CreationRoutine.state(self,5)+[self.cpu.mem_read(DATA+a,1)[0] for a in
            (0x9418,0x9419,0xA750,0xAFD2,0xAFCE,0xAFD1)]+[self.signed_word(DATA+0x75DC),self.signed_word(DATA+0x9B72)]

    def state(self,*args):return self.snapshot()

    def on_instruction(self,cpu,address,size,u):
        if address==0xBF32 and not self.initial:cpu.reg_write(UC_X86_REG_IP,0xBF5A);return
        if address==0x4277:
            caller=self.signed_word(cpu.reg_read(UC_X86_REG_SP))&65535
            if caller==0xBF7F:
                if self.initial or not self.first:
                    self.states.append(self.snapshot());self.stop_reason='command';cpu.emu_stop();return
                self.first=False;cpu.reg_write(UC_X86_REG_AX,self.command);self.return_from_hook(1);return
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key);self.return_from_hook(1);return
        if address==0x609A:self.return_from_hook();return
        if address==0x47B3:self.return_from_hook(80);return
        if address==0x4EFE:self.return_from_hook(2);return
        if address==0x4210:self.return_from_hook();return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xBA28,STOP,timeout=10000000,count=4000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:self.states.append(self.snapshot())
        elif self.stop_reason not in ('input','command'):raise RuntimeError(f'Meduz IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(g,key,text,initial):
    p=subprocess.run([str(HERE/'build/cmake/tera_meduz_bridge.exe')],input=pack_meduz(g,key,initial=initial,travel=False)+
        ''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or str(p.returncode))
    result=[];state=None
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='V':result.append(state+[v[i] for i in (3,5,7,8,9,10,4,11)])
    return result


class MeduzTravelRoutine(MeduzRoutine):
    """Branche TERAMAIN et chargement des vrais overlays 0145/A4/0142."""
    def __init__(self,image,g,text,returning=False):
        super().__init__(image,g,ord('2') if returning else ord('M'),text,not returning)
        self.cpu.mem_write(0xDA21,image[0xDA21-0x20FC:]);self.returning=returning

    def on_instruction(self,cpu,address,size,u):
        if address==0xBA0F:
            module=cpu.reg_read(UC_X86_REG_DX)
            a,b={0x145:(0x14500,0x14800),0x142:(0x14200,0x14500),0xA4:(0xA400,0xAE00)}[module]
            cpu.mem_write(0xBA21,OVERLAYS.read_bytes()[a:b]);cpu.ctl_remove_cache(0xBA21,0xDA21)
            cpu.reg_write(UC_X86_REG_IP,0xBA28);return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xE54F if self.returning else 0xE4C2,0xE570,timeout=10000000,count=5000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==0xE570:self.states.append(self.snapshot())
        elif self.stop_reason not in ('input','command'):raise RuntimeError(f'Travel IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def compare_travel(image):
    failures=[];boundaries=calls=count=0
    for seed in range(12):
        for sound in (0,1):
            for returning in (False,True):
                g=game(seed);g.update(sound=sound,position=1000 if returning else 1450,previous_position=999,
                    world_id=10 if returning else 11,meduz_backup_position=1450)
                g['maps'][1000]=[0,0,1,0,0,0,1]
                text='FO'*30 if returning else ('N' if seed==0 else 'OWRONG\r' if seed==1 else 'OTRAVEL\r'+'FO'*30)
                m=MeduzTravelRoutine(image,g,text,returning);a=m.execute()
                p=subprocess.run([str(HERE/'build/cmake/tera_meduz_bridge.exe')],input=pack_meduz(g,ord('2') if returning else ord('M'),enter=not returning,travel=True)+
                    ''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
                if p.returncode not in (0,4):raise RuntimeError('Travel C '+str(p.returncode))
                b=[];state=None
                for line in p.stdout.splitlines():
                    tag,*v=line.split();v=list(map(int,v))
                    if tag=='S':state=v
                    elif tag=='V':b.append(state+[v[i] for i in (3,5,7,8,9,10,4,11)])
                if a!=b:
                    at=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
                    diffs=[(i,x,y) for i,(x,y) in enumerate(zip(a[at],b[at])) if x!=y] if at<min(len(a),len(b)) else []
                    failures.append(dict(seed=seed,sound=sound,returning=returning,lengths=[len(a),len(b)],boundary=at,diffs=diffs[:10]))
                count+=1;boundaries+=len(b);calls+=len(m.calls)
    return dict(cases=count,input_boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:12])


def game(seed=1986):
    g=fixture(seed);g.update(world_id=10,position=1045,previous_position=1044,sound=0,
        mental_extension=0,tasks_done=[0]*4,tasks_counter=0,meduz_backup_position=1500,secret_words=['TEST','TRAVEL','UNION','SETH'])
    return g


def cases():
    result=[]
    for heading in (1,-1,10,-10):
        for key in '2468':
            for wall in (0,1):
                for seed in range(6):
                    g=game(seed);g['previous_position']=1045-heading;g['maps'][1045][:4]=[wall]*4
                    result.append((f'walk/{heading}/{key}/{wall}/{seed}',g,ord(key),'FO'*20,False))
    for content in (0,2,3):
        for seed in range(10):
            for sound in (0,1):
                g=game(seed);g['maps'][1000][4]=content;g['sound']=sound
                result.append((f'initial/{content}/{seed}/{sound}',g,0,'FO'*20,True))
    for extension in (0,1):
        for done in (0,1):
            for text in ('','N','O','\r','OUNION\r','OWRONG\r','OUN7I\bION\r','OABCDEFGHIJ'):
                for command,content in [('M',2),('I',3)]:
                    g=game();g['maps'][1045][4]=content;g['mental_extension']=extension;g['tasks_done'][0]=done
                    g['tasks_counter']=32767;g['actors'][0]['stats']=[255]*10
                    result.append((f'event/{command}/{extension}/{done}/{text!r}',g,ord(command),text,False))
    for sound in (0,1):
        g=game();g.update(position=1000,previous_position=999,sound=sound);g['maps'][1000][2]=1
        result.append((f'exit/{sound}',g,ord('2'),'FO'*20,False))
        g=game();g['sound']=sound
        for a in g['actors']:a.update(active=0,hp=0,condition='HS')
        result.append((f'initial_dead/{sound}',g,0,'',True))
        for text in ('','WRONG\r','ARIOCH\r'+'FO'*40):
            g=game();g['sound']=sound;g['maps'][1045][4]=0
            result.append((f'generic_invocation/{sound}/{text[:8]!r}',g,ord('I'),text,False))
    return result


def main():
    image=SOURCE.read_bytes();started=time.monotonic()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:raise SystemExit('Empreinte differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,g,key,text,initial) in enumerate(corpus,1):
        m=MeduzRoutine(image,g,key,text,initial);a=m.execute();b=execute_c(g,key,text,initial)
        if a!=b:
            at=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            diffs=[(i,x,y) for i,(x,y) in enumerate(zip(a[at],b[at])) if x!=y] if at<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=at,lengths=[len(a),len(b)],diffs=diffs[:12]))
        boundaries+=len(b);calls+=len(m.calls)
        if index%100==0:print(f'Meduz {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),input_boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:18],seconds=round(time.monotonic()-started,2),com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,scope='OVL00A4 initial/command loops, real combat/day/RNG, mental extension/task; video/delays abstracted')
    report['travel']=compare_travel(image)
    (HERE/'build/audit/meduz_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures or report['travel']['mismatches']:raise SystemExit(1)


if __name__=='__main__':main()
