"""OVL0145 execute : oracle de toutes les images, sans rendu SDL."""
import hashlib
from audit_media_binary import MediaRoutine
from compare_combat_binary import SOURCE, EXPECTED_HASH, HERE
from compare_exploration_binary import OVERLAYS, OVERLAY_HASH
from unicorn.x86_const import UC_X86_REG_DS, UC_X86_REG_SP


class TunnelRoutine(MediaRoutine):
    def __init__(self, image):
        super().__init__(image)
        self.cpu.mem_map(0x10000, 65536)
        self.cpu.reg_write(UC_X86_REG_DS, 0x1000)
        self.cpu.mem_write(0xBA21, OVERLAYS.read_bytes()[0x14500:0x14800])
        self.frames = []
        self.stars = []
        self.reset = 0
        self.palette = self.background = 0

    def on_instruction(self, cpu, address, size, data):
        counts = {0x4520: 1, 0x2DB4: 3, 0x2DAB: 1, 0x2DAE: 1, 0x2DC0: 4}
        if address not in counts:
            return super().on_instruction(cpu, address, size, data)
        count = counts[address]
        sp = cpu.reg_read(UC_X86_REG_SP)
        args = [self.signed_word(sp+2*i) for i in range(count, 0, -1)]
        if address == 0x4520:
            self.stars = []
            self.reset = 1
        elif address == 0x2DB4:
            self.stars.extend(args[:2])
        elif address == 0x2DAB:
            self.palette = args[0]
        elif address == 0x2DAE:
            self.background = args[0]
        elif args[3]:
            self.frames.append(dict(reset=self.reset, palette=self.palette,
                background=self.background, x=args[0], y=args[1], radius=args[2],
                color=args[3], stars=self.stars[:] if self.reset else []))
            self.reset = 0
        else:
            self.frames[-1].update(erase_x=args[0], erase_y=args[1], erase_radius=args[2])
        self.return_from_hook(count*2)


def main():
    image = SOURCE.read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED_HASH
    assert hashlib.sha256(OVERLAYS.read_bytes()).hexdigest() == OVERLAY_HASH
    machine = TunnelRoutine(image)
    machine.call(0xBA28, [])
    lines = ['/* OVL0145 execute, graine 1986 ; audit_tunnel_binary.py. */',
             'static const TeraTunnelFrame original_tunnel_frames[]={']
    for frame in machine.frames:
        fields = []
        for name, value in frame.items():
            literal = '{'+','.join(map(str, value))+'}' if isinstance(value, list) else str(value)
            if value != []:
                fields.append('.'+name+'='+literal)
        lines.append('    {'+','.join(fields)+'},')
    lines.append('};')
    (HERE/'tests/original_tunnel_vectors.h').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    print(len(machine.frames), 'images natives ; graine finale', hex(machine.seed))


if __name__ == '__main__':
    main()
