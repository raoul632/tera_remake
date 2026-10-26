"""Comparer les commandes solliciter/seduire au resident original, RNG reel."""
import hashlib
import json
import random
import subprocess
import time
from compare_creation_binary import CreationRoutine, SOURCE, OVERLAYS, HERE, ROOT, EXPECTED_HASH, DATA
from compare_combat_binary import PARENT
from unicorn.x86_const import UC_X86_REG_SP, UC_X86_REG_IP
from creation import empty_roster, pack_roster


class RecruitmentRoutine(CreationRoutine):
    def __init__(self,image,overlays,game,request,ready,key):
        super().__init__(image,overlays,game,[ord('S'),key] if request==0 else [key])
        self.request=request;self.outcome=0;self.negotiator=-1
        self.initial_count=0;self.available=0;self.joined=0
        self.cpu.mem_write(DATA+0xAFCE,bytes([ready]))
        for id in range(1,10):self.data_word(0x7550+id*2,game['locations'][id])
        for file,address in [('VERBES',0x5F98),('MESSAGE',0x63C0),('MESSAGE2',0x9C24),('MESSAGE3',0xA14C)]:
            self.cpu.mem_write(DATA+address,(ROOT/'data/compiled program'/file).read_bytes())

    def on_instruction(self,cpu,address,size,user_data):
        if address==0x4277:
            if not self.keys:raise RuntimeError('Clavier supplementaire inattendu')
            key=self.keys.pop(0)
            if 97<=key<=122:key-=32
            from unicorn.x86_const import UC_X86_REG_AX
            cpu.reg_write(UC_X86_REG_AX,key);self.return_from_hook(1)
        elif address==0x6052:self.return_from_hook()
        elif address==0x47B3:self.return_from_hook(80)
        elif address==0x4807:cpu.reg_write(UC_X86_REG_IP,0x4820)
        elif address in (0xB22A,0xB7DC,0xB89E,0xB8FD,0xB90F,0xB921):
            self.outcome={0xB22A:3,0xB7DC:5,0xB89E:6,0xB8FD:7,0xB90F:4,0xB921:8}[address]
            if address==0xB7DC:self.negotiator=self.signed_word(DATA+0x9B6A)
        elif address==0xB746:self.initial_count=self.signed_word(DATA+0x9B7C)
        elif address==0xB743:self.initial_count=self.signed_word(DATA+0x9B7C)
        elif address==0xB252:
            self.available |= 1<<self.signed_word(DATA+0xAFC3);self.outcome=2
        elif address==0xB827:
            id=self.signed_word(DATA+0xAFC3)
            self.available |= 1<<id;self.joined |= 1<<id
        elif address==0xB8DE:
            id=self.signed_word(DATA+0xAFC3)
            if not cpu.mem_read(DATA+0x9402+id,1)[0]:self.joined |= 1<<id
        else:super().on_instruction(cpu,address,size,user_data)

    def execute(self):
        if self.request==0:
            self.write_word(PARENT+4,ord('S'))
            self.call(0xAFEF,[],has_result=False)
        else:self.call(0xAFE8,[ord('S')],has_result=False)
        if self.keys:raise RuntimeError('Touches non consommees')
        return [self.outcome or 1,self.cpu.mem_read(DATA+0xAFCE,1)[0],self.negotiator,
                len(self.draw_bounds),self.initial_count,self.available,self.joined],self.state(5)


def execute_c(game,request,ready,key):
    packet=f'{request} {ready} {key}\n'+pack_roster(game,False)+' '.join(map(str,game['locations'][1:]))+'\n'
    out=subprocess.run([str(HERE/'build/cmake/tera_recruitment_bridge.exe')],input=packet,
                       text=True,capture_output=True,check=True).stdout
    rows={line.split()[0]:list(map(int,line.split()[1:])) for line in out.splitlines()}
    return rows['R'],rows['S']


def generate_cases():
    cases=[]
    for request in (0,1):
        for ready in (0,1):
            for key in (ord('O'),13,ord('N'),27):
                for party_count in (1,3,4,5):
                    for money in (249,250,500):
                        for mode in range(3):
                            game=empty_roster(1986+party_count)
                            game.update(money=money,experience=32767,position=132)
                            game['locations']=[0,132,132,333,444,555,666,777,132 if mode==2 else 888,999]
                            for id,actor in enumerate(game['actors']):
                                actor.update(active=int(id<party_count),condition='HS' if mode==1 else 'OK')
                                actor['stats']=[10 if mode==0 else 0]*10
                                actor['skills']=[3 if mode==0 else 0]*10
                            cases.append((game,request,ready,key))
    rng=random.Random(1986)
    for index in range(120):
        game=empty_roster(rng.getrandbits(32))
        game.update(money=rng.choice([-32768,0,249,250,32767]),experience=32767,position=132)
        game['locations']=[rng.choice([132,99,-211]) for _ in range(10)]
        for actor in game['actors']:
            actor.update(active=rng.randrange(2),condition=rng.choice(['OK','HS','EM']),hp=rng.choice([-1,0,1,20]))
            actor['stats']=[rng.randrange(256) for _ in range(10)]
            actor['skills']=[rng.randrange(256) for _ in range(10)]
        cases.append((game,index%2,(index//2)%2,ord('o')))
    return cases


def main():
    started=time.monotonic();image=SOURCE.read_bytes();overlays=OVERLAYS.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH:raise SystemExit('Empreinte COM differente')
    subprocess.run(['cmake','--build',str(HERE/'build/cmake')],check=True)
    cases=generate_cases();failures=[];calls=0
    for index,(game,request,ready,key) in enumerate(cases,1):
        machine=RecruitmentRoutine(image,overlays,game,request,ready,key)
        original=machine.execute();c=execute_c(game,request,ready,key);calls+=len(machine.draw_bounds)
        if original!=c:
            failures.append(dict(index=index,request=request,ready=ready,key=key,original_report=original[0],c_report=c[0],
                state_differences=[(i,a,b) for i,(a,b) in enumerate(zip(original[1],c[1])) if a!=b][:10]))
        if index%200==0:print(f'Recrutements compares {index}/{len(cases)} ; ecarts {len(failures)}',flush=True)
    report=dict(cases=len(cases),mismatches=len(failures),original_random_calls=calls,first_mismatches=failures[:5],
                com_sha256=EXPECTED_HASH,seconds=round(time.monotonic()-started,2),scope='actual A074, AFEF(S), AFE8(S), 8315 and RNG; keys injected, presentation bypassed')
    (HERE/'build/audit/recruitment_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
