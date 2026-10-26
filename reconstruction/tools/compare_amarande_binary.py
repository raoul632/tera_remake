"""CAFE8 et panneaux TERAMAIN : commandes et rencontres originales executees."""
import hashlib
import json
import subprocess
import time
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,OVERLAY_HASH,STACK,STOP,word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_AX,UC_X86_REG_SP,UC_X86_REG_IP
from amarande import pack_amarande


class AmarandeRoutine(ExplorationRoutine):
    def __init__(self,image,g,key,text,panel=False):
        super().__init__(image,g,list(map(ord,text)),1,key)
        self.panel=panel;self.ack=False;self.shown=0
        self.response=[0,0,0];self.response_pending=False
        self.data_word(0x9B66,g.get('alfol_status_actor',9))
        self.cpu.mem_write(DATA+0xABD7,bytes([g.get('npc_variant',0)]))
        self.cpu.mem_write(DATA+0x9417,bytes([g.get('initial_flag',1)]))
        self.cpu.mem_write(DATA+0xA74B,b'\0')
        if panel:self.cpu.mem_write(0xDA21,image[0xDA21-0x20FC:])

    def snapshot(self):
        return CreationRoutine.state(self,5)+[self.cpu.mem_read(DATA+a,1)[0] for a in
            (0xA74A,0xA74D,0xA750,0xAFD2,0xAFCE,0xAFD1)]+[self.signed_word(DATA+0x9B72),
            self.signed_word(DATA+0x9B66),self.cpu.mem_read(DATA+0xA74B,1)[0]]+self.response

    def state(self,*args):return self.snapshot()

    def on_instruction(self,cpu,address,size,u):
        if address==0x4473:
            sp=cpu.reg_read(UC_X86_REG_SP);caller=self.signed_word(sp)&65535
            self.response_pending=0xA700<=caller<0xAA13 or 0xB2A0<=caller<0xBA0F
            if self.response_pending:self.response[0]=self.signed_word(sp+2)&255
        if address==0x4480 and self.response_pending:
            self.response[1]=cpu.reg_read(UC_X86_REG_AX);self.response[2]+=1
            self.response_pending=False
        if address in (0x4277,0x4210):
            self.states.append(self.snapshot())
            if not self.keys:self.stop_reason='input';cpu.emu_stop();return
            key=self.keys.pop(0)
            if address==0x4277:cpu.reg_write(UC_X86_REG_AX,key-32 if 97<=key<=122 else key);self.return_from_hook(1)
            else:self.return_from_hook()
            return
        if address==0x47B3:self.return_from_hook(80);return
        if address==0x4EFE:self.return_from_hook(2);return
        if address==0x5495:
            # La carte n'a pas de RANDOM ni effet durable hormis son indice global.
            self.data_word(0x9B66,9);self.return_from_hook();return
        if address==0x5274:
            # C4D46 vient d'etre execute ; sauter seulement le dessin de la fiche.
            cpu.reg_write(UC_X86_REG_IP,0x548C);return
        super().on_instruction(cpu,address,size,u)

    def execute(self):
        self.cpu.mem_write(STACK,word(STOP)+word(self.key));self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xE5C7 if self.panel else 0xAFE8,0xE6E1 if self.panel else STOP,timeout=10000000,count=5000000)
        if self.cpu.reg_read(UC_X86_REG_IP)==(0xE6E1 if self.panel else STOP):self.states.append(self.snapshot())
        elif self.stop_reason!='input':raise RuntimeError(f'Amarande IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(g,key,text):
    p=subprocess.run([str(HERE/'build/cmake/tera_amarande_bridge.exe')],input=pack_amarande(g,key)+
        ''.join(f'{ord(k)}\n' for k in text),text=True,capture_output=True,timeout=10)
    if p.returncode not in (0,4):raise RuntimeError(p.stderr or str(p.returncode))
    states=[];state=None;flags=None;quit_flag=0;response=[0,0,0]
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':state=v
        elif tag=='Y':flags=v
        elif tag=='Q':quit_flag=v[0]
        elif tag=='R':response=v
        elif tag=='Z':states.append(state+flags[1:8]+[flags[16],quit_flag]+response)
    return states


def cases():
    result=[]
    for command in 'C#UDEFLMOP':
        for text in ('','N','O','\r'):
            g=fixture(1986);result.append((f'confirm/{command}/{text!r}',g,ord(command),text,False))
    for npc in range(1,10):
        for active in (0,1):
            for condition in ('OK','HS'):
                for sound in (0,1):
                    g=fixture(npc);g['locations'][npc]=g['position'];g['actors'][npc].update(active=active,condition=condition)
                    g['sound']=sound
                    result.append((f'npc/{npc}/{active}/{condition}/{sound}',g,ord('C'),'O'+'FO'*40,False))
    for npc in range(1,10):
        for variant in range(3):
            for info in (0,1):
                g=fixture(npc);g.update(npc_variant=variant,information_context=info)
                g['extra_locations'][npc]=g['position']
                result.append((f'extra/{npc}/{variant}/{info}',g,ord('C'),'O'+'FO'*40,False))
    for text in ('','WRONG\r','ARIOCH\r'+'FO'*30):
        for town in (False,True):
            g=fixture(1986)
            if town:g['towns'][0]=g['position']
            result.append((f'invoke/{town}/{text[:6]}',g,ord('I'),text,False))
    for text in ('','?','A','AN','AO','C','D','DN','DO'+'FO'*30,'H','I','M','MO'+'FO'*30,'S','SN','SO'+'X'*10,'\x1bN','\x1bO'):
        for seed in (0,1,1986):
            g=fixture(seed);g.update(initial_flag=1,alfol_status_actor=5)
            g['actors'][5]['active']=1
            result.append((f'camp/{seed}/{text[:5]!r}',g,ord('H'),text,False))
    for has_map in (0,1):
        for fast in (0,1):
            for text in ('C','AN','AO','A\r'):
                g=fixture(1986);g.update(initial_flag=has_map,fast_moves=fast)
                result.append((f'camp-flags/{has_map}/{fast}/{text!r}',g,ord('H'),text,False))
    for text in ('O','OX','OXY','\rXY'):
        result.append((f'u-reads/{text!r}',fixture(1986),ord('U'),text,False))
    for npc,attack in ((1,'T'),(4,'T'),(7,'T'),(8,'T'),(9,'J')):
        g=fixture(1986);g['locations'][npc]=g['position']
        g['actors'][0].update(hp=10000,magic=200,stats=[0]+[10]*9,skills=[0]+[3]*9,
            equipment=[3,6,9,12,15,18,21,0,0,0])
        result.append((f'npc-victory/{npc}',g,ord('C'),'O'+(attack+'O')*100,False))
    for position in (1000,1001,1450,1899):
        for krono in (1000,1200,1899):
            for text in ('','N','O','OX','\rX'):
                g=fixture(1986);g.update(position=position,town_found=1,town_id=10);g['towns'][0]=krono
                result.append((f'panel/{position}/{krono}/{text!r}',g,ord('L'),text,True))
    return result


def main():
    image=SOURCE.read_bytes();started=time.monotonic()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()!=OVERLAY_HASH:raise SystemExit('Empreinte differente')
    failures=[];boundaries=calls=0;corpus=cases()
    for index,(name,g,key,text,panel) in enumerate(corpus,1):
        m=AmarandeRoutine(image,g,key,text,panel);a=m.execute();b=execute_c(g,key,text)
        if a!=b:
            at=next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
            diffs=[(i,x,y) for i,(x,y) in enumerate(zip(a[at],b[at])) if x!=y] if at<min(len(a),len(b)) else []
            failures.append(dict(name=name,boundary=at,lengths=[len(a),len(b)],diffs=diffs[:12]))
        boundaries+=len(b);calls+=len(m.calls)
        if index%100==0:print(f'Amarande {index}/{len(corpus)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(corpus),input_boundaries=boundaries,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:20],seconds=round(time.monotonic()-started,2),com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,scope='resident C/I/H/# and TERAMAIN L panels; real combat/day/RNG; map/stats video abstracted')
    (HERE/'build/audit/amarande_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
