"""Optional BIN/C audit: original RANDOMIZE and full generated world states.

Python only drives the oracle and transports integers. No Python gameplay
implementation supplies expected values. Never writes original game files.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(HERE / 'tools'))
from compare_combat_binary import SOURCE, EXPECTED_HASH, STACK, STOP, word
from compare_world_binary import WorldRoutine, OVERLAY, NAMES
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_CX, UC_X86_REG_DX,
                              UC_X86_REG_SP, UC_X86_REG_IP)


def run(exe, packet):
    return subprocess.run([str(exe)], input=packet, text=True,
                          capture_output=True, check=True).stdout


def original_clock(image, clock):
    cpu = Uc(UC_ARCH_X86, UC_MODE_16)
    cpu.mem_map(0, 65536)
    cpu.mem_write(0x100, image)
    cpu.mem_write(STACK, word(STOP))
    cpu.reg_write(UC_X86_REG_SP, STACK)
    interrupts = []

    def dos_time(machine, interrupt, _):
        if interrupt != 0x21 or machine.reg_read(UC_X86_REG_AX) >> 8 != 0x2c:
            raise RuntimeError('Unexpected interrupt in RANDOMIZE')
        interrupts.append(interrupt)
        hour, minute, second, hundredth = clock
        machine.reg_write(UC_X86_REG_CX, (hour << 8) | minute)
        machine.reg_write(UC_X86_REG_DX, (second << 8) | hundredth)

    cpu.hook_add(UC_HOOK_INTR, dos_time)
    cpu.emu_start(0xf14, STOP, count=100)
    if cpu.reg_read(UC_X86_REG_IP) != STOP or interrupts != [0x21]:
        raise RuntimeError('Incomplete original RANDOMIZE')
    return struct.unpack('<I', cpu.mem_read(0x1fc, 4))[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    build = args.build_dir.resolve()
    image, overlays = SOURCE.read_bytes(), OVERLAY.read_bytes()
    hashes = {'TERA.COM': hashlib.sha256(image).hexdigest(),
              'TERA.000': hashlib.sha256(overlays).hexdigest()}
    if hashes['TERA.COM'] != EXPECTED_HASH or hashes['TERA.000'] != (
            '539d95781f9296d40ecd05698eb98255835a45cb03e60b49cf581e3ada7038f4'):
        raise RuntimeError('Original binary hash mismatch')
    clocks = [(0, 0, 0, 0), (12, 34, 56, 78), (23, 59, 59, 99), (1, 2, 3, 4)]
    clock_results = []
    for clock in clocks:
        expected = original_clock(image, clock)
        actual = int(run(build / 'binary_comparison_runner.exe',
                         'Z 4 ' + ' '.join(map(str, clock)) + '\n'))
        if actual != expected:
            raise AssertionError(('RANDOMIZE', clock, actual, expected))
        clock_results.append({'clock': clock, 'original_seed': expected})
    names = ' '.join(map(str, NAMES.read_bytes()))
    cases = []
    for seed in (0, 23, 1986, 0xffffffff):
        for difficulty in (0, 5, 9):
            machine = WorldRoutine(image, overlays, seed, difficulty)
            expected = machine.execute(True)
            packet = f'{seed & 65535} {seed >> 16} {difficulty} 1 2000000 {names}\n'
            output = run(build / 'tera_world_bridge.exe', packet)
            actual = {line.split()[0]: list(map(int, line.split()[1:]))
                      for line in output.splitlines()}
            if actual != expected:
                raise AssertionError(('world', seed, difficulty,
                                      [key for key in expected if actual.get(key) != expected[key]]))
            cases.append({'seed': seed, 'difficulty': difficulty,
                          'original_draws': machine.calls, 'attempts': machine.attempts})
            print(f'BIN/C world seed={seed} difficulty={difficulty}: OK', flush=True)
    report = {'hashes': hashes, 'clock_cases': clock_results, 'world_cases': cases,
              'mismatches': 0,
              'scope': 'BIN RANDOMIZE with DOS time input; OVL0000 after inputs and full OVL0010; all snapshot fields'}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('BIN/C: 4 RANDOMIZE and 12 full world cases, zero mismatch')


if __name__ == '__main__':
    main()
