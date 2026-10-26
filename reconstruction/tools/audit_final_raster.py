"""Optional final raster oracle: execute native GRAPH and sprite blitters.

Only loading resources, text and keyboard are abstracted. Hashes refer to
actual B800 pixels, not a replay through SDL or the reconstructed primitives.
"""
import hashlib
import struct
from collections import deque
from audit_cga_native import NativeCga, fnv
from compare_combat_binary import SOURCE, EXPECTED_HASH, HERE
from unicorn.x86_const import UC_X86_REG_IP, UC_X86_REG_SP
from compare_exploration_binary import OVERLAYS, OVERLAY_HASH


class NativeScene(NativeCga):
    def __init__(self, image):
        self.recent=deque(maxlen=30)
        super().__init__(image)
        self.cpu.mem_write(0x10000+0x260,struct.pack('<HHHH',0,0x3000,0,0x3400))
        self.cpu.mem_write(0x10000+0x2b4,struct.pack('<HHHH',0,0x3000,0x2000,0x3000))
        raw = (HERE.parent/'data/compiled program/CRAPIM.IMA').read_bytes()
        at = 0
        plane_at = 1  # Pascal arrays are 1-based; width[82] occupies plane[0].
        for sprite in range(1, 83):
            w, h = raw[at:at+2]
            hs = vs = 1
            if sprite > 50:
                hs = 2
                if sprite == 59:
                    hs, vs = 1, 2
                if sprite > 59:
                    hs, vs = 2, 3 if sprite > 74 else 2
                if sprite > 76:
                    hs, vs = 3, 1
                if sprite == 81:
                    vs = 3
                if sprite == 82:
                    hs, vs = 2, 6
            n = w*h*hs*vs//2
            if h % 2 and sprite < 59:
                n = w*hs*vs*((h-1)//2)
            self.cpu.mem_write(0x10000+0x30d+sprite, bytes([w]))
            self.cpu.mem_write(0x10000+0x2bb+sprite, bytes([h]))
            self.write_word(0x10000+0x9b7c+2*sprite, plane_at)
            body = raw[at+2:at+2+2*n]
            self.cpu.mem_write(0x10000+0x35f+plane_at, body[0::2])
            self.cpu.mem_write(0x10000+0x30b3+plane_at, body[1::2])
            at += 2+2*n
            plane_at += n
        assert at == len(raw)

    def on_instruction(self, cpu, address, size, data):
        if address>=0x10000 or address<0x200:
            raise RuntimeError(f'Unexpected code segment after {list(self.recent)}')
        self.recent.append(hex(address))
        if address == 0x7237:  # text after the spaceport graphics
            cpu.reg_write(UC_X86_REG_IP, 0x7243)
        elif address == 0x724a:  # isolate monolith geometry from random writing
            self.return_from_hook(4)
        elif address == 0xa0dd:  # end of scene, rules are compared separately
            self.return_from_hook()
        elif address == 0x838a:  # combat starts after monster drawing
            self.return_from_hook(4)
        elif address in (0x4779,0x28b9):
            self.return_from_hook(40 if address == 0x4779 else 2)
        elif address == 0x2926:
            sp=cpu.reg_read(UC_X86_REG_SP)
            self.return_from_hook(cpu.mem_read(sp+2,1)[0]+1)
        elif address == 0x295d:
            sp=cpu.reg_read(UC_X86_REG_SP);inline=self.signed_word(sp)&65535
            self.write_word(sp,inline+cpu.mem_read(inline,1)[0]+1)
            self.return_from_hook()
        else:
            super().on_instruction(cpu, address, size, data)


def main():
    image = SOURCE.read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED_HASH
    assert hashlib.sha256(OVERLAYS.read_bytes()).hexdigest() == OVERLAY_HASH
    out = HERE/'build/audit/final-raster'
    out.mkdir(parents=True, exist_ok=True)
    cases = []
    # Native DRAW: both directions, slopes, ties, horizontal/vertical, clipping.
    for args in [(10,10,30,20,3),(30,20,10,10,3),(10,10,20,30,3),
                 (20,30,10,10,3),(10,30,30,10,3),(30,10,10,30,3),
                 (0,0,319,199,3),(319,199,0,0,3),(-20,50,340,70,3),
                 (50,-20,70,240,3),(0,199,319,0,3),(10,10,10,10,3),
                 (20,10,20,190,3),(0,40,319,40,3)]:
        cases.append(('line',0x2db7,args))
    cases += [('bytes',0x4544,[19,96,25,180,0]),
              ('catalyst',0x6dc5,[38,164,140,4,2,0]),
              ('large',0x77f0,[]),('hood',0x7b74,[0,1]),
              ('portrait',0x7a96,[12,3]),('spaceport',0x7136,[]),
              ('monolith',0x7309,[])]
    cases += [('sprite',0x5051,[sprite,x,y]) for sprite in range(1,83)
              for x,y in ((20,80),(73,137),(-5,190))]
    cases += [('country',0xba28,[scene,ord('8'),0]) for scene in range(23)]
    cases += [('country',0xba28,[scene,ord('6'),1]) for scene in (0,4,10,18,22)]
    cases += [('monster',0x9b4e,[monster,world]) for world in (0,11)
              for monster in range(1,23) if monster != 15]
    cases += [('place',0xba28,[town]) for town in (2,3,4,5)]
    cases += [('arch',0x7a55,[156,140,20,12,2])]
    cases += [('room',0xc9b7,[variant,left,front,right,1]) for variant in (1,2,3)
              for left,front,right in ((0,0,0),(1,0,0),(2,0,0),(0,1,0),(0,2,0),(0,3,0),
                                       (0,0,1),(0,0,2),(3,3,3),(2,1,3),(1,2,2),(3,0,1))]
    cases += [('ruins',0xc792,[left,front,(left+front)%4]) for left in range(4) for front in range(4)]
    cases += [(name,entry,[left,front,right]) for name,entry in (('meduz',0xbcdd),('alfol',0xcafc))
              for left in (0,1) for front in (0,1) for right in (0,1)]
    cases += [('ruins',0xc792,[left,front,(left+front)%4,heading]) for heading in (-1,10,-10)
              for left in range(4) for front in range(4)]
    lines = ['/* Native GRAPH/blitters; audit_final_raster.py; COM SHA checked. */',
             'typedef struct {unsigned kind;int a,b,c,d,e;uint32_t hash;} NativeRasterCase;',
             'static const NativeRasterCase native_raster_cases[]={']
    for index,(name,entry,args) in enumerate(cases):
        machine = NativeScene(image)
        # Structured non-black background detects masks and excessive clears.
        if name not in ('line','spaceport','monolith','country','place','arch','room','ruins','meduz','alfol'):
            machine.cpu.mem_write(0xb8000, bytes([0x66])*0x4000)
        call_args=[args[1],args[2],args[0]] if name == 'sprite' else args
        if name == 'country':
            scene,key,near=args
            position=1380+scene
            machine.cpu.mem_write(0xba21,OVERLAYS.read_bytes()[0xfb00:0x11300])
            for offset,count in ((0x753e,10),(0x7550,10),(0x7564,9),(0x7576,25),(0x75a8,20)):
                for j in range(count):machine.write_word(0x10000+offset+2*j,-3000)
            if near:machine.write_word(0x10000+0x7576,position)
            machine.write_word(0x10000+0x75d2,position-(30 if key==ord('6') else 1))
            machine.cpu.mem_write(0x10000+0xa679,bytes([key]))
            machine.cpu.mem_write(0x10000+0xa74a,b'\0')
            call_args=[]
        if name == 'monster':
            monster,world=args
            machine.write_word(0x10000+0x9b76,world)
            machine.cpu.mem_write(0x10000+0xa74a,b'\0')
            call_args=[monster]
        if name == 'place':
            town=args[0]
            start,end={2:(0xf300,0xfb00),3:(0x14200,0x14500),
                       4:(0xf000,0xf300),5:(0xec00,0xf000)}[town]
            machine.cpu.mem_write(0xba21,OVERLAYS.read_bytes()[start:end])
            machine.cpu.mem_write(0x10000+0xa74a,b'\0')
            call_args=[]
        if name == 'room':
            variant,left,front,right,heading=args
            machine.cpu.mem_write(0xba21,OVERLAYS.read_bytes()[0x4900:0x6900])
            for offset,value in ((0x75d2,145),(0x9b72,145-heading),(0x9b76,1)):
                machine.write_word(0x10000+offset,value)
            machine.cpu.mem_write(0x10000+0x75ee+145*7,bytes([front,left,0,right,255,0,variant]))
            machine.cpu.mem_write(0x10000+0xa742,b'\1')
            machine.cpu.mem_write(0x10000+0xa74a,b'\0')
            call_args=[]
        if name in ('ruins','meduz','alfol'):
            left,front,right=args[:3];heading=args[3] if len(args)>3 else 1
            first,last,world,position={'ruins':(0x2e00,0x4900,0,45),
                'meduz':(0xa400,0xae00,10,1000),'alfol':(0x8500,0xa400,9,945)}[name]
            machine.cpu.mem_write(0xba21,OVERLAYS.read_bytes()[first:last])
            for offset,value in ((0x75d2,position),(0x9b72,position-heading),(0x9b76,world)):
                machine.write_word(0x10000+offset,value)
            cell=[0,0,0,0,255 if name=='ruins' else 0,0,0]
            li,fi,ri={1:(1,0,3),-1:(3,2,1),10:(0,3,2),-10:(2,1,0)}[heading]
            cell[li]=left;cell[fi]=front;cell[ri]=right
            machine.cpu.mem_write(0x10000+0x75ee+position*7,bytes(cell))
            machine.cpu.mem_write(0x10000+0xa742,b'\1')
            machine.cpu.mem_write(0x10000+0xa74a,b'\0')
            call_args=[]
        try:
            machine.call(entry,call_args)
        except Exception as error:
            raise RuntimeError(f'{name} {args}: IP={machine.cpu.reg_read(UC_X86_REG_IP):04x}; {list(machine.recent)}') from error
        if name == 'spaceport':
            machine.call(0x5051,[73,36,82])
        pixels = machine.pixels()
        (out/f'{index}-{name}.raw').write_bytes(pixels)
        values = (list(args)+[0]*5)[:5]
        kind = {'line':1,'bytes':2,'catalyst':3,'large':4,'hood':5,
                'portrait':6,'spaceport':7,'monolith':8,'sprite':9,'country':10,'monster':11,'place':12,'arch':13,'room':14,'ruins':15,'meduz':16,'alfol':17}[name]
        lines.append('    {'+','.join(map(str,[kind,*values]))+f',UINT32_C(0x{fnv(pixels):08x})'+'},')
        print(index,name,hex(fnv(pixels)),flush=True)
    lines.append('};')
    (HERE/'tests/original_final_raster_vectors.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')


if __name__ == '__main__':
    main()
