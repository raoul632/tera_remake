"""Profil nominal 8088 des chemins executes du pilote CGA original.

Tables Intel : couts d'execution + EA + transferts de mots sur bus 8 bits.
Pas une simulation de la file de prelecture, du refresh DMA ou des wait states.
MUL/DIV prennent le haut de leur plage documentee. Aucun temps Unicorn/mur.
"""
import re
import struct
import hashlib
from functools import lru_cache
import subprocess
from pathlib import Path
from audit_cga_native import NativeCga
from compare_combat_binary import SOURCE, EXPECTED_HASH, HERE, STOP, STACK, word
from compare_exploration_binary import OVERLAYS, OVERLAY_HASH
from unicorn.x86_const import (UC_X86_REG_CX, UC_X86_REG_AX, UC_X86_REG_BP,
    UC_X86_REG_SP, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SI,
    UC_X86_REG_BX, UC_X86_REG_DX)


def disassembly():
    out = {}
    overlay = HERE/'build/audit/0145.bin'
    overlay.write_bytes(OVERLAYS.read_bytes()[0x14500:0x14800])
    for path,base in ((SOURCE,0x100),(overlay,0xba21)):
        text = subprocess.check_output(['E:/msys64/ucrt64/bin/objdump.exe','-D','-b','binary',
            '-m','i8086','-Mintel',f'--adjust-vma={base}',str(path)],text=True)
        for line in text.splitlines():
            fields=line.split('\t')
            if len(fields)>=3 and re.match(r'^\s*[0-9a-f]+:$',fields[0]):
                address=int(fields[0].strip()[:-1],16)
                raw=bytes.fromhex(fields[1]);inst=fields[2].strip().split(None,1)
                if inst: out[address]=(len(raw),inst[0],inst[1] if len(inst)>1 else '')
    return out


def ea(arg):
    arg=arg.split('[')[-1].split(']')[0]
    regs=re.findall(r'\b(?:bx|bp|si|di)\b',arg)
    if len(regs)==2: base=7 if set(regs) in ({'bx','si'},{'bp','di'}) else 8
    elif regs: base=5
    else: return 6
    return base+(4 if re.search(r'0x',arg) else 0)


@lru_cache(maxsize=None)
def clocks(inst,next_address,address,cx):
    size,op,args=inst;parts=args.split(',');mem='[' in args
    word_mem='WORD PTR' in args;transfer=4 if word_mem else 0
    e=ea(args) if mem else 0
    prefix=2 if re.search(r'\b(?:cs|es|ss):',args) else 0
    taken=next_address!=address+size
    dst_mem=bool(parts and '[' in parts[0])
    immediate=bool(parts and re.match(r'^(?:0x|[0-9-])',parts[-1].strip()))
    if op=='mov': value=(10+e+transfer if immediate else (9 if dst_mem else 8)+e+transfer) if mem else (4 if immediate else 2)
    elif op in ('push','pop'):
        value=(24 if op=='push' else 25)+e if mem else (15 if op=='push' else 12)
    elif op=='call': value=23
    elif op=='ret': value=24 if args else 20
    elif op=='jmp': value=15
    elif op in ('loop','loope','loopne'): value=17 if taken else 5
    elif op=='jcxz': value=18 if taken else 6
    elif op.startswith('j'): value=16 if taken else 4
    elif op in ('mul','imul','div','idiv'):
        value={'mul':133,'imul':154,'div':162,'idiv':184}[op]+(6+e+transfer if mem else 0)
    elif op in ('shl','shr','sar','rol','ror','rcl','rcr'):
        variable=parts[-1].strip()=='cl'
        value=((20 if variable else 15)+e+2*transfer if mem else (8 if variable else 2))+(4*(cx&255) if variable else 0)
    elif op in ('add','adc','sub','sbb','and','or','xor','cmp','test'):
        if mem: value=(10 if immediate else 9)+e+transfer if op in ('cmp','test') or not dst_mem else (17 if immediate else 16)+e+2*transfer
        else: value=4 if immediate else 3
    elif op in ('inc','dec','neg','not'):
        value=(15 if op in ('inc','dec') else 16)+e+2*transfer if mem else (3 if op in ('inc','dec','neg','not') else 2)
    elif op=='xchg': value=17+e+2*transfer if mem else 3
    elif op=='lea': value=2+e
    elif op in ('les','lds'): value=24+e
    elif op=='cwd': value=5
    elif op=='cbw': value=2
    elif op in ('clc','stc','cmc','cld','std','cli','sti'): value=2
    elif op=='xlat': value=11
    elif op=='int': value=51
    elif op in ('in','out'): value=8 if 'dx' in args else 10
    elif op=='nop': value=3
    elif op=='stos': value=11
    elif op in ('rep','repz'): value=10  # hook Unicorn par iteration STOSB
    else: raise ValueError((hex(address),op,args))
    return value+prefix


class XTProfile(NativeCga):
    def __init__(self,image,code):
        self.enabled=False;self.code=code;self.stop_at=None
        super().__init__(image)
        self.cpu.mem_write(0xba21,OVERLAYS.read_bytes()[0x14500:0x14800])

    def reset_clock(self):
        self.previous=None;self.cycles=0;self.pixel_cycles=0;self.circle_cycles=0
        self.in_pixel=False;self.in_circle=False;self.enabled=True

    def finish_instruction(self,next_address):
        if self.previous:
            address,cx,pixel,circle=self.previous
            value=clocks(self.code[address],next_address,address,cx)
            self.cycles+=value
            if pixel:self.pixel_cycles+=value
            if circle:self.circle_cycles+=value

    def on_instruction(self,cpu,address,size,data):
        if self.enabled:
            self.finish_instruction(address)
            if address==self.stop_at:
                self.previous=None;cpu.emu_stop();return
            if self.previous and self.previous[0]==0x3b06:self.in_pixel=False
            if address==0x3ad0:self.in_pixel=True
            if address==0x2dc0:self.in_circle=True
            if address in (0xbbcb,0xbc03):self.in_circle=False
            self.previous=(address,cpu.reg_read(UC_X86_REG_CX),self.in_pixel,self.in_circle)
        super().on_instruction(cpu,address,size,data)

    def profile(self,entry,args):
        self.reset_clock();self.call(entry,args);self.finish_instruction(STOP);self.enabled=False
        return self.cycles

    def point(self,x,y):
        self.cpu.reg_write(UC_X86_REG_DS,0);self.cpu.reg_write(UC_X86_REG_ES,0xb800)
        self.cpu.reg_write(UC_X86_REG_SI,0x3d5c)
        self.cpu.reg_write(UC_X86_REG_BX,x&65535);self.cpu.reg_write(UC_X86_REG_DX,(199-y)&65535)
        value=self.profile(0x3ad0,[])
        self.cpu.reg_write(UC_X86_REG_DS,0x1000)
        return value


def main():
    image=SOURCE.read_bytes();code=disassembly();m=XTProfile(image,code)
    assert hashlib.sha256(image).hexdigest()==EXPECTED_HASH
    assert hashlib.sha256(OVERLAYS.read_bytes()).hexdigest()==OVERLAY_HASH
    table=[0]
    for radius in range(1,101):
        m.profile(0x2dc0,[160,100,radius,3]);table.append(m.cycles-m.pixel_cycles)
    points=[m.point(100+i,100+j) for j in range(2) for i in range(4)]
    clips=[m.point(-1,50),m.point(320,50),m.point(50,-1),m.point(50,200)]
    overhead=[]
    for flags in range(16):
        pal,bg,erase,sound=[(flags>>i)&1 for i in range(4)]
        # Choisir une graine dont les appels RANDOM suivent ces branches.
        for seed in range(10000):
            state=seed;draws=[]
            for bound in ([4,3,16] if pal else [4,16]):
                state=(129*state+0x361962e9)&0xffffffff;draws.append((state>>17)%bound)
            if (draws[0]==1)==bool(pal) and (draws[-1]==1)==bool(bg): break
        m.cpu.mem_write(0x10000+0x1fc,struct.pack('<I',seed))
        m.cpu.mem_write(0x10000+0xa74a,bytes([sound]))
        bp=0xd000;m.cpu.reg_write(UC_X86_REG_BP,bp)
        for offset,value in ((4,0),(6,4),(8,2),(10,3),(12,100),(14,160),
                             (-8,160),(-10,100),(-12,100),(-14,50 if erase else 100),(-16,0),(-18,0)):
            m.write_word(bp+offset,value)
        m.cpu.reg_write(UC_X86_REG_SP,STACK)
        m.reset_clock();m.stop_at=0xbc1a;m.cpu.emu_start(0xbafa,0xbc1a,count=2000000);m.stop_at=None
        m.finish_instruction(0xbc1a);m.enabled=False
        overhead.append(m.cycles-m.circle_cycles)
    # Preparation d'un tunnel : deux directions, 24 etoiles, boucles et mode.
    # Cout nominal du clear BIOS : 8192 STOSW sur bus 8 bits, 18 cycles/mot.
    m.cpu.mem_write(0x10000+0x1fc,struct.pack('<I',1986));m.cpu.reg_write(UC_X86_REG_BP,0xd000)
    m.cpu.mem_write(STACK,b''.join(word(v) for v in [STOP,0,4,2,3,100,160]))
    m.cpu.reg_write(UC_X86_REG_SP,STACK)
    m.reset_clock();m.stop_at=0xbafa;m.cpu.emu_start(0xba2f,0xbafa,count=2000000);m.stop_at=None
    m.finish_instruction(0xbafa);m.enabled=False
    setup=m.cycles-m.pixel_cycles+8192*18
    m.cpu.reg_write(UC_X86_REG_AX,3)
    setup+=3*m.profile(0x10da,[])  # couleur, pas et longueur avant INNER_BA2F
    lines=['/* Profil nominal 8088 execute, audit_xt_timing.py ; pas une mesure murale. */',
           'static const unsigned long xt_circle_core[101]={'+','.join(map(str,table))+'};',
           'static const unsigned xt_point_visible[8]={'+','.join(map(str,points))+'};',
           'static const unsigned xt_point_clip[4]={'+','.join(map(str,clips))+'};',
           'static const unsigned xt_frame_overhead[16]={'+','.join(map(str,overhead))+'};',
           f'#define TERA_XT_TUNNEL_SETUP {setup}u']
    (HERE/'frontend/sdl3/xt_profile.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    checks=[]
    for x,y,r in ((160,100,1),(160,100,4),(160,100,50),(160,100,99),(-10,50,80),(300,200,100)):
        checks.append((x,y,r,m.profile(0x2dc0,[x,y,r,3])))
    (HERE/'tests/original_xt_vectors.h').write_text(
        'static const struct {int x,y;unsigned radius,cycles;} original_xt_circles[]={\n'+
        ''.join('    {'+','.join(map(str,c))+'},\n' for c in checks)+'};\n',encoding='utf-8')
    print('XT : couts de 100 rayons, 8 pixels, 4 clippings, 16 chemins ; setup',setup)
    print('Points',points,'clipping',clips,'overhead',overhead)
    # Controle independant : la sequence entiere, sans remplacer ses cercles.
    m=XTProfile(image,code);m.cpu.mem_write(STACK,word(STOP))
    m.cpu.reg_write(UC_X86_REG_SP,STACK);m.reset_clock()
    m.cpu.emu_start(0xba28,STOP,timeout=120000000,count=30000000)
    m.finish_instruction(STOP);m.enabled=False
    # Sept changements de mode internes + le changement de mode externe.
    native_total=m.cycles+8*8192*18
    print('XT sequence entiere :',native_total,'cycles nominaux,',native_total/4772727,'s')
    with (HERE/'tests/original_xt_vectors.h').open('a',encoding='utf-8') as output:
        output.write('#define ORIGINAL_XT_TUNNEL_CYCLES UINT64_C(%d)\n'%native_total)


if __name__=='__main__':main()
