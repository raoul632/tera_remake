"""Les dix lieux d'Amarande : RNG et condition originale de presentation PNJ."""
import hashlib
import json
import subprocess
from compare_exploration_binary import ExplorationRoutine,fixture,DATA,HERE,SOURCE,EXPECTED_HASH,OVERLAYS,OVERLAY_HASH,STACK,STOP,word
from compare_creation_binary import CreationRoutine
from unicorn.x86_const import UC_X86_REG_SP,UC_X86_REG_IP
from exploration import pack_exploration


def main():
    image=SOURCE.read_bytes();raw=OVERLAYS.read_bytes();failures=[];calls=count=0
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(raw).hexdigest()!=OVERLAY_HASH:raise SystemExit('Empreinte differente')
    for town in range(10):
        for sound in (0,1):
            for seed in (0,1,1986,0xffffffff):
                for profile in range(4):
                    g=fixture(seed);g.update(town_found=1,town_id=town,sound=sound,moved=1,position=1450)
                    g['towns'][town]=1450
                    for a in g['actors']:a['active']=profile%2;a['condition']='HS' if profile>1 else 'OK'
                    if profile>0:
                        for i in range(1,10):g['locations'][i]=1450
                    m=ExplorationRoutine(image,g,[],1,0)
                    if town in (1,2,3,4,5):
                        a,b={1:(0x14800,0x14900),2:(0xF300,0xFB00),3:(0x14200,0x14500),4:(0xF000,0xF300),5:(0xEC00,0xF000)}[town]
                        m.cpu.mem_write(0xBA21,raw[a:b]);start,end=0xBA28,STOP
                    else:
                        m.cpu.mem_write(0xDA21,image[0xDA21-0x20FC:])
                        start,end={0:(0xDCEF,0xDD6B),6:(0xDE7D,0xE1A6),7:(0xDF72,0xE1A6),8:(0xE042,0xE1A6),9:(0xE12D,0xE1A6)}[town]
                    # TERAMAIN occupe DE00..E1xx : une pile E000 ecraserait
                    # le code de la facade 7 pendant les chaines temporaires.
                    m.cpu.mem_write(0xF000,word(STOP));m.cpu.reg_write(UC_X86_REG_SP,0xF000)
                    try:m.cpu.emu_start(start,end,timeout=10000000,count=2000000)
                    except Exception as error:raise RuntimeError(f'Facade {town}, IP={m.cpu.reg_read(UC_X86_REG_IP):04X}: {error}') from error
                    if m.cpu.reg_read(UC_X86_REG_IP)!=end:raise RuntimeError(f'Facade {town} IP {m.cpu.reg_read(UC_X86_REG_IP):04X}')
                    expected=CreationRoutine.state(m,5)+[m.cpu.mem_read(DATA+0xA750,1)[0]]
                    p=subprocess.run([str(HERE/'build/cmake/tera_places_bridge.exe')],input=pack_exploration(g,0,0),text=True,capture_output=True,check=True)
                    actual=None;info=0
                    for line in p.stdout.splitlines():
                        tag,*v=line.split();v=list(map(int,v))
                        if tag=='S':actual=v
                        elif tag=='J':info=v[0]
                    actual+=[info]
                    if actual!=expected:failures.append(dict(town=town,sound=sound,seed=seed,profile=profile,diffs=[(i,x,y) for i,(x,y) in enumerate(zip(expected,actual)) if x!=y][:10]))
                    calls+=len(m.calls);count+=1
    report=dict(cases=count,original_rng_calls=calls,mismatches=len(failures),first_mismatches=failures[:15],com_sha256=EXPECTED_HASH,overlay_sha256=OVERLAY_HASH,scope='all ten facades, original NPC predicate, RNG; bitmap DOS loading abstracted')
    (HERE/'build/audit/amarande_facades_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    if failures:raise SystemExit(1)


if __name__=='__main__':main()
