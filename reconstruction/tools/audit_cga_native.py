"""Execute le pilote CGA du COM : ni DRAW, ni Circle ne sont abstraits."""
import hashlib
import struct
from collections import Counter
from audit_media_binary import MediaRoutine
from compare_combat_binary import SOURCE, EXPECTED_HASH, HERE
from compare_exploration_binary import OVERLAYS
from unicorn import UC_HOOK_INTR
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_DS, UC_X86_REG_IP, UC_X86_REG_EFLAGS,
                              UC_X86_REG_SP)


class NativeCga(MediaRoutine):
    def __init__(self, image):
        super().__init__(image)
        self.cpu.mem_map(0x10000, 0xf0000)
        self.cpu.reg_write(UC_X86_REG_DS, 0x1000)
        self.cpu.hook_add(UC_HOOK_INTR, self.bios)
        self.cpu.mem_write(0x10000+0x1fc, struct.pack('<I', 1986))
        self.instructions = Counter()
        self.cursor = (0, 0)
        self.key_queries = 0
        self.blink = 0
        self.call(0x2DA2, [])

    def bios(self, cpu, number, data):
        assert number == 0x10, (number, hex(cpu.reg_read(UC_X86_REG_IP)))
        ax = cpu.reg_read(UC_X86_REG_AX)
        if ax >> 8 == 0:
            cpu.mem_write(0x449, bytes([ax & 255]))
            cpu.mem_write(0xb8000, bytes(0x4000))

    def on_instruction(self, cpu, address, size, data):
        self.instructions[address] += 1
        if address == 0x3BB:  # CRT TextColor, pas du dessin GRAPH
            self.return_from_hook()
        elif address == 0x0DA4:
            self.key_queries += 1
            pressed=self.key_queries > self.blink+1
            cpu.reg_write(UC_X86_REG_AX, int(pressed))
            flags=cpu.reg_read(UC_X86_REG_EFLAGS)
            cpu.reg_write(UC_X86_REG_EFLAGS,(flags&~64) if pressed else (flags|64))
            self.return_from_hook()
        elif address == 0x0DAA:
            sp = cpu.reg_read(UC_X86_REG_SP)
            self.cursor = (self.signed_word(sp+2)-1, cpu.reg_read(UC_X86_REG_AX)-1)
            self.return_from_hook(2)
        elif address == 0x289D:
            sp = cpu.reg_read(UC_X86_REG_SP)
            char = self.signed_word(sp+2)
            self.text(char)
            self.return_from_hook(2)
        elif address in (0x2558, 0x1022, 0x213, 0x59C, 0x5C3):
            self.return_from_hook()
        # RANDOM et toutes les primitives residentes s'executent vraiment.

    def text(self, char):
        font = (HERE/'build/audit/BIOS.F08').read_bytes()
        x, y = self.cursor
        for row, bits in enumerate(font[char*8:char*8+8]):
            for col in range(8):
                if bits & (128 >> col):
                    px, py = x*8+col, y*8+row
                    address = 0xb8000+(py%2)*8192+(py//2)*80+px//4
                    old = self.cpu.mem_read(address, 1)[0]
                    shift = 6-2*(px%4)
                    self.cpu.mem_write(address, bytes([(old & ~(3 << shift)) | (3 << shift)]))

    def pixels(self):
        raw = self.cpu.mem_read(0xb8000, 0x4000)
        return bytes((raw[(y%2)*8192+(y//2)*80+x//4] >> (6-2*(x%4))) & 3
                     for y in range(200) for x in range(320))


class NativeTunnel(NativeCga):
    def __init__(self,image):
        super().__init__(image)
        self.hashes=[]
        self.cpu.mem_write(0xba21,OVERLAYS.read_bytes()[0x14500:0x14800])

    def on_instruction(self,cpu,address,size,data):
        if address==0xbc1a:self.hashes.append(fnv(self.pixels()))
        super().on_instruction(cpu,address,size,data)


def main():
    image = SOURCE.read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED_HASH
    machine = NativeCga(image)
    machine.call(0x2DC0, [160, 100, 50, 3])
    (HERE/'build/audit/native-circle.raw').write_bytes(machine.pixels())
    print('Cercle natif :', sum(machine.instructions.values()), 'instructions',
          sum(bool(p) for p in machine.pixels()), 'pixels')
    lines = ['/* Pilote CGA original execute, fixtures synthetiques ; audit_cga_native.py. */',
             'typedef struct {unsigned world;int position,heading,alfol;unsigned difficulty,blink,flag;uint32_t hash;} OriginalMapCase;',
             'static const OriginalMapCase original_map_cases[]={']
    cases = [(11, p, 1, 0, 1, 0, flag) for p in (1000,1200,1899) for flag in (0,1)]
    cases += [(11,44,1,1,1,0,0), (11,0,1,1,1,0,1)]
    cases += [(world,world*100+45,h,0,d,b,0) for world in (0,8,10)
              for h in (1,-1,10,-10) for d,b in ((1,0),(1,1),(5,0))]
    for world,pos,heading,alfol,difficulty,blink,flag in cases:
        machine = NativeCga(image)
        machine.blink = blink
        for offset,value in ((0x9b76,world),(0x75d2,pos),(0x9b72,pos-heading),
                             (0x75e8,difficulty),(0x75d0,45)):
            machine.write_word(0x10000+offset,value)
        machine.cpu.mem_write(0x10000+0xa748,bytes([alfol]))
        machine.cpu.mem_write(0x10000+0xa744,bytes([flag]))
        maps = bytes(v for i in range(1100) for v in
                     ((i+0)%4,(i+1)%4,(i+2)%4,(i+3)%4,0,0,int(i%11!=0)))
        machine.cpu.mem_write(0x10000+0x75ee,maps)
        for i in range(20): machine.write_word(0x10000+0x75a8+2*i,1000+(47*i)%900)
        for i in range(9): machine.write_word(0x10000+0x7564+2*i,1000+30*i)
        for i in range(25): machine.write_word(0x10000+0x7576+2*i,1000+3*i)
        machine.call(0x5495,[])
        pixels = machine.pixels()
        digest = fnv(pixels)
        lines.append('    {'+','.join(map(str,(world,pos,heading,alfol,difficulty,blink,flag)))+
                     ',UINT32_C(0x%08x)},'%digest)
        (HERE/f'build/audit/map-native-{len(lines)-4}.raw').write_bytes(pixels)
    lines.append('};')
    lines.append('typedef struct {int x,y;unsigned radius;uint32_t hash;} OriginalCircleCase;')
    lines.append('static const OriginalCircleCase original_circle_cases[]={')
    for x,y,r in ((160,100,50),(160,100,4),(290,70,4),(-10,50,80),(300,200,100),(160,100,1)):
        machine=NativeCga(image);machine.call(0x2dc0,[x,y,r,3])
        lines.append('    {%d,%d,%d,UINT32_C(0x%08x)},'%(x,y,r,fnv(machine.pixels())))
    lines.append('};')
    (HERE/'tests/original_map_raster_vectors.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(len(cases),'cartes et 6 cercles natifs verifies par empreintes de B800')
    from compare_combat_binary import STOP,STACK,word
    machine=NativeTunnel(image)
    machine.cpu.mem_write(STACK,word(STOP));machine.cpu.reg_write(UC_X86_REG_SP,STACK)
    machine.cpu.emu_start(0xba28,STOP,timeout=60000000,count=20000000)
    assert machine.cpu.reg_read(UC_X86_REG_IP)==STOP
    assert len(machine.hashes)==245
    (HERE/'tests/original_tunnel_raster_vectors.h').write_text(
        '/* B800 reel apres chaque image OVL0145, graine 1986. */\n'+
        'static const uint32_t original_tunnel_hashes[]={\n'+
        ''.join('    UINT32_C(0x%08x),\n'%v for v in machine.hashes)+'};\n',encoding='utf-8')
    print('245 empreintes du raster natif des tunnels')


def fnv(data):
    value=2166136261
    for byte in data: value=((value^byte)*16777619)&0xffffffff
    return value


if __name__ == '__main__':
    main()
