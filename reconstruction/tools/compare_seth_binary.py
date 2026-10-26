"""OVL0069 : navigation/entree, coffres et invocation, instructions et RNG reels."""
import copy
import hashlib
import json
import random
import subprocess
import time
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,OVERLAY_HASH,STACK,STOP,word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_IP,UC_X86_REG_DX
from navigation import pack_navigation


class SethRoutine(ExplorationRoutine):
    def __init__(self,image,g,key,text,initial):
        super().__init__(image,g,list(map(ord,text)),1,key)
        self.initial=initial;self.command=key;self.first_command=True;self.render_seen=False
        self.data_word(0xAFD5,g.get('city_backup_position',1500));self.data_word(0x75DC,g['tasks_counter'])
        self.data_word(0x75E4,g['arioch_position'])
        self.cpu.mem_write(DATA+0x9419,bytes(g['tasks_done']))
        self.cpu.mem_write(DATA+0x940C,bytes(g['has_map']))
        raw=g['secret_words'][3].encode('cp437');self.cpu.mem_write(DATA+0x9B58,bytes([len(raw)])+raw.ljust(9,b'\0'))
        self.cpu.mem_write(0xBA21,OVERLAYS.read_bytes()[0x6900:0x8500])

    def snapshot(self):
        return CreationRoutine.state(self,5)+[self.cpu.mem_read(DATA+a,1)[0] for a in (0xAFD2,0xAFCE,0xAFD1)]+[
            self.signed_word(DATA+0x9B72),self.cpu.mem_read(DATA+0x941C,1)[0],self.signed_word(DATA+0x75DC),
            self.cpu.mem_read(DATA+0xA750,1)[0],self.cpu.mem_read(STACK-5,1)[0]]+list(self.cpu.mem_read(DATA+0x940C,11))+list(self.cpu.mem_read(DATA+0x75EE,7700))+[self.cpu.mem_read(DATA+0x941B,1)[0]]

    def state(self,*args):return self.snapshot()

    def on_instruction(self,cpu,address,size,u):
        if address==0xCC04 and not self.initial:
            cpu.reg_write(UC_X86_REG_SP,cpu.reg_read(UC_X86_REG_SP)-1)
            cpu.mem_write(STACK-5,bytes([self.game.get('chest_open',0)]))
            cpu.reg_write(UC_X86_REG_IP,0xCC26);return # Reprise a la commande dans l'overlay existant.
        if address==0x9B45:return
        if address==0x4277:
            sp=cpu.reg_read(UC_X86_REG_SP);caller=self.signed_word(sp)&65535
            if caller==0xCC4B:
                if self.initial or not self.first_command:
                    self.states.append(self.snapshot());self.stop_reason='command';cpu.emu_stop();return
                self.first_command=False;cpu.reg_write(UC_X86_REG_AX,self.command);self.return_from_hook(1);return
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0);cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key);self.return_from_hook(1);return
        if address==0x4210:self.return_from_hook();return
        if address==0x47B3:self.return_from_hook(80);return
        if address==0x4EFE:self.return_from_hook(2);return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xBA28,STOP,timeout=10000000,count=3000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==STOP:self.states.append(self.snapshot())
        elif self.stop_reason not in ('command','input'):raise RuntimeError(f'Seth interrompu {self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(g,key,text,initial,enter=False):
    p=subprocess.run([str(HERE/'build/cmake/tera_navigation_bridge.exe')],
        input=pack_navigation(g,key,initial_move=initial,enter=enter)+''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or f'Pont {p.returncode}')
    state=None;maps=copy.deepcopy(g['maps']);info=g.get('information_context',0);task=g['tasks_done'][3];opened=g.get('chest_open',0)
    result=[];flags=None;hasmap=g['has_map']
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='J':info=v[0]
        elif tag=='F':task=v[0]
        elif tag=='N':flags=v
        elif tag=='G':opened=v[0];hasmap=v[6:]
        elif tag=='B':maps[v[0]]=v[1:]
        elif tag=='Z':
            snapshot=state+flags[5:9]+[task,flags[20],info,opened]+hasmap+[x for row in maps for x in row]+[flags[19]]
            if enter:
                snapshot.pop(len(state)+7) # BP_03 n'existe pas avant l'appel de l'overlay.
                snapshot.append(flags[3])
            result.append(snapshot)
    return result


class SethEntryRoutine(SethRoutine):
    """Branche TERAMAIN E57B, puis vrai overlay 0069 jusqu'a son premier menu."""
    def __init__(self,image,g,text):
        super().__init__(image,g,ord('E'),text,True)
        self.cpu.mem_write(0xDA21,image[0xDA21-0x20FC:])

    def snapshot(self):
        values=super().snapshot()
        values.pop(len(CreationRoutine.state(self,5))+7)
        return values+[self.signed_word(DATA+0xAFD5)]

    def on_instruction(self,cpu,address,size,u):
        if address==0xBA0F:
            selector=cpu.reg_read(UC_X86_REG_DX)
            start,end={0x69:(0x6900,0x8500),0xF0:(0xF000,0xF300)}[selector]
            cpu.mem_write(0xBA21,OVERLAYS.read_bytes()[start:end]);cpu.ctl_remove_cache(0xBA21,0xDA21)
            cpu.reg_write(UC_X86_REG_IP,0xBA28);return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xE57B,0xE6E1,timeout=10000000,count=3000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==0xE6E1:self.states.append(self.snapshot())
        elif self.stop_reason not in ('command','input'):raise RuntimeError('Entree Seth interrompue')
        return self.states


def game(seed=1986):
    g=fixture(seed);g.update(world_id=8,position=845,previous_position=844,city_overlay=2,city_backup_position=1500,
        tasks_done=[0]*4,tasks_counter=0,arioch_position=777,has_map=[0]*11,secret_words=['','','','TEST'],sound=0)
    return g


def cases():
    result=[]
    for heading in (1,-1,10,-10):
        for key in '2468':
            for wall in range(4):
                g=game();g['previous_position']=845-heading;g['maps'][845][:4]=[wall]*4
                result.append((f'walk/{heading}/{key}/{wall}',g,ord(key),'OUFOFO',False))
    for seed in range(12):
        for sound in (0,1):
            g=game(seed);g.update(position=800,previous_position=799,sound=sound,fast_moves=1)
            result.append((f'arrival/{seed}/{sound}',g,0,'FOFO',True))
    for command in ('O','P','L','I','D','M'):
        for content in (0,18,35,89,90,110):
            g=game();g['maps'][845][4]=content;g['chest_open']=int(command=='P' and content<90)
            result.append((f'action/{command}/{content}',g,ord(command),'OA0TEST\rFOFO',False))
    for seed in range(60):
        for sound in (0,1):
            g=game(seed);g['maps'][845][4]=18;g['sound']=sound
            g['actors'][0].update(stats=[0]*10,skills=[0]*10,equipment=[0]*10)
            result.append((f'trap/{seed}/{sound}',g,ord('O'),'OA',False))
    for kind in (1,21):
        for sound in (0,1):
            g=game();g.update(sound=sound,arioch_position=845)
            g.update(position=844,previous_position=843);g['maps'][844][0]=1
            g['maps'][845][4:]=[0,kind,1]
            g['actors'][0].update(hp=10000,magic=200,stats=[0]+[10]*9,skills=[0]+[3]*9)
            result.append((f'fixed_victory/{kind}/{sound}',g,ord('8'),('TO' if kind==1 else 'COTEST\r')*16,False))
    for sound in (0,1):
        g=game();g.update(sound=sound,position=800,previous_position=799)
        for a in g['actors']:a.update(active=0,hp=0,condition='HS')
        result.append((f'defeat/{sound}',g,0,'',True))
    for word in ('TEST','WRONG','te7s\bST','ABCDEFGHIJ'):
        for done in (0,1):
            g=game();g['maps'][845][4]=110;g['tasks_done'][3]=done
            result.append((f'invoke/{word}/{done}',g,ord('I'),'O'+word+'\rFOFO',False))
    for sound in (0,1):
        for done in (0,1):
            g=game();g.update(sound=sound,tasks_counter=32767);g['maps'][845][4]=110;g['tasks_done'][3]=done
            g['actors'][0].update(hp=10000,magic=200,stats=[0]+[10]*9,skills=[0]+[3]*9)
            result.append((f'victory/{sound}/{done}',g,ord('I'),'OTEST\rCOTEST\r'*1+'COTEST\r'*15,False))
    for key in ('2','L'):
        g=game();g.update(position=800,previous_position=799);g['maps'][800][:4]=[1]*4
        result.append((f'exit_or_read/{key}',g,ord(key),'O',False))
    for sound in (0,1):
        for text in ('','N','\x1b','O','\r'):
            g=game(1986);g.update(world_id=11,position=1450,previous_position=1449,
                town_found=1,town_id=4,city_overlay=0,city_backup_position=1234,sound=sound,fast_moves=1)
            g['maps'][800]=[0,0,1,0,0,0,1]
            result.append((f'entry/{sound}/{text!r}',g,ord('E'),text,False))
    return result


def main():
    image=SOURCE.read_bytes();started=time.monotonic()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:
        raise SystemExit('Empreinte originale differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,g,key,text,initial) in enumerate(corpus,1):
        entry=name.startswith('entry/')
        m=SethEntryRoutine(image,g,text) if entry else SethRoutine(image,g,key,text,initial)
        a=m.execute();b=execute_c(g,key,text,initial,enter=entry)
        if a!=b:
            offset=next((i for i,p in enumerate(zip(a,b)) if p[0]!=p[1]),min(len(a),len(b)))
            diff=[(i,x,y) for i,(x,y) in enumerate(zip(a[offset],b[offset])) if x!=y] if offset<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=offset,lengths=[len(a),len(b)],diff=diff[:12]))
        boundaries+=len(b);calls+=len(m.calls)
        if index%50==0:print(f'Seth {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),
        first_mismatches=failures[:15],com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,
        seconds=round(time.monotonic()-started,2),scope='OVL0069, C7EF6/C7DBD, coffres, C9B4E/A0DD, invocation et objectif 3; clavier/BIOS abstraits')
    (HERE/'build/audit/seth_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
