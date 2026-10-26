"""Execute original C4210: queued keys are discarded before the new read.

Only Pascal file/BIOS keyboard I/O and housekeeping are hooked. The original
purge loop and its branches execute. This is not a DOSBox timing measurement.
"""
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(HERE / 'tools'))
from compare_combat_binary import (OriginalRoutine, SOURCE, EXPECTED_HASH,
                                   STACK, STOP, word)
from unicorn.x86_const import UC_X86_REG_EFLAGS, UC_X86_REG_IP, UC_X86_REG_SP


class PurgeRoutine(OriginalRoutine):
    def __init__(self, image, old_keys):
        super().__init__(image)
        self.pending = old_keys
        self.discarded = 0
        self.empty_seen = False
        self.polls = 0

    def on_instruction(self, cpu, address, size, data):
        if address == 0xda4:
            self.polls += 1
            available = self.pending > 0 or self.empty_seen
            if not available:
                self.empty_seen = True
            flags = cpu.reg_read(UC_X86_REG_EFLAGS)
            cpu.reg_write(UC_X86_REG_EFLAGS, flags & ~64 if available else flags | 64)
            self.return_from_hook()
        elif address == 0x253b:
            self.return_from_hook(2)  # Select Pascal keyboard file.
        elif address == 0x2762:
            if self.pending <= 0:
                raise AssertionError('Purge consumed the new key')
            self.pending -= 1
            self.discarded += 1
            self.return_from_hook(2)
        elif address == 0x1022:
            self.return_from_hook()  # Runtime I/O status check.


def main():
    image = SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED_HASH:
        raise RuntimeError('Original COM hash mismatch')
    result = []
    for count in (0, 1, 3, 15):
        machine = PurgeRoutine(image, count)
        machine.cpu.mem_write(STACK, word(STOP))
        machine.cpu.reg_write(UC_X86_REG_SP, STACK)
        machine.cpu.emu_start(0x4210, STOP, count=10000)
        if machine.cpu.reg_read(UC_X86_REG_IP) != STOP or machine.discarded != count:
            raise AssertionError('Original purge incomplete')
        result.append({'queued': count, 'discarded': machine.discarded,
                       'new_key_consumed': False, 'polls': machine.polls})
    output = HERE / 'build/port-audit/keyboard-purge.json'
    output.write_text(json.dumps({'com_sha256': EXPECTED_HASH, 'cases': result}, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
