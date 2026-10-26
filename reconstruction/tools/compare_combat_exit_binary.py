"""Comparer les compteurs de recompense et la relocation de fuite au COM.

Les cartes sont des fixtures de colonne 7, pas des cartes redistribuees.
Les branches et les tentatives rejetees sont executees par le binaire.
"""
import hashlib
import json
import subprocess

from compare_combat_binary import OriginalRoutine, HERE, SOURCE, EXPECTED_HASH, PARENT, STACK, word
from unicorn.x86_const import UC_X86_REG_DS, UC_X86_REG_SP, UC_X86_REG_IP

DATA = 0x10000


class ExitRoutine(OriginalRoutine):
    def __init__(self, image, draws):
        super().__init__(image, draws)
        self.cpu.mem_map(DATA, 0x10000)
        self.cpu.reg_write(UC_X86_REG_DS, DATA >> 4)

    def data_word(self, address, value):
        self.cpu.mem_write(DATA + address, word(value))

    def on_instruction(self, cpu, address, size, user_data):
        """Conserver tirages/calculs/ecritures ; sauter presentation et sons."""
        skips = {0x9AC3: 0x9AE6, 0x9AF2: 0x9B0F,
                 0x9B18: 0x9B3C, 0x8765: 0x8783}
        if address in skips:
            cpu.reg_write(UC_X86_REG_IP, skips[address])
        else:
            super().on_instruction(cpu, address, size, user_data)


def generate_cases():
    """Seuils/overflow de compteurs, chaque monde et rejets avant fuite."""
    cases = []
    for victory in (0, 1):
        for available in (0, 1):
            for money, experience, hp in ((0, 0, 20), (32767, 32767, 100), (-32768, -32768, -1)):
                for draw in (0, 49):
                    cases.append(('W', [victory, available, money, experience, hp, draw]))
    for world in range(11):
        for flag in (0, 1):
            for first, last, count in ((0, 0, 1), (89, 89, 1), (0, 89, 2), (89, 0, 2)):
                cases.append(('X', [world, flag, -123, first, last, count]))
    for flag, maximum in ((0, 254), (1, 98)):
        for draw in (0, 1, maximum):
            cases.append(('X', [11, flag, -123, draw, draw, 1]))
    return cases


def execute_original(image, mode, v):
    """Initialiser la memoire DS et lire les seules sorties couvertes."""
    if mode == 'W':
        victory, available, money, experience, hp, draw = v
        machine = ExitRoutine(image, [draw] if victory and available else [])
        machine.cpu.mem_write(DATA + 0xA746, bytes([victory]))
        machine.cpu.mem_write(DATA + 0xA745, bytes([available]))
        machine.data_word(0x75D4, money); machine.data_word(0x75D6, experience)
        machine.write_word(PARENT - 0x1C, hp)
        machine.cpu.reg_write(UC_X86_REG_SP, STACK)
        machine.cpu.emu_start(0x9AAE, 0x9B48, timeout=1000000, count=10000)
        if machine.cpu.reg_read(UC_X86_REG_IP) != 0x9B48 or machine.draws:
            raise RuntimeError("Sortie de recompense incomplete")
        return [len(machine.random_bounds), machine.signed_word(DATA + 0x75D4), machine.signed_word(DATA + 0x75D6)]
    world, flag, previous, first, last, count = v
    machine = ExitRoutine(image, [first, last][:count])
    machine.data_word(0x9B76, world)
    machine.data_word(0x9B72, previous)
    machine.cpu.mem_write(DATA + 0xA748, bytes([flag]))
    if world < 11:
        position = world*100 + last + 5
        machine.cpu.mem_write(DATA + 0x75F4 + position*7, b"\x01")
    machine.call(0x86E1, [], has_result=False)
    return [machine.signed_word(DATA + 0x75D2), machine.signed_word(DATA + 0x9B72), len(machine.random_bounds)]


def main():
    """Executer la campagne isolee et publier son rapport reproductible."""
    image = SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED_HASH:
        raise SystemExit("Empreinte TERA.COM differente")
    subprocess.run(["cmake", "--build", str(HERE / "build/cmake")], check=True)
    cases = generate_cases()
    protocol = ''.join(f"{mode} {len(v)} {' '.join(map(str, v))}\n" for mode, v in cases)
    output = subprocess.run([str(HERE / "build/cmake/binary_comparison_runner.exe")], input=protocol,
                            text=True, capture_output=True, check=True).stdout.splitlines()
    if len(output) != len(cases):
        raise RuntimeError("Nombre de sorties C incorrect")
    failures = []
    for (mode, values), line in zip(cases, output):
        binary, c = execute_original(image, mode, values), list(map(int, line.split()))
        if binary != c:
            failures.append({'mode': mode, 'inputs': values, 'binary': binary, 'c': c})
    report = {'source_sha256': EXPECTED_HASH, 'cases': len(cases), 'mismatches': len(failures),
              'first_mismatches': failures[:10], 'boundaries': 'RANDOM injected, presentation/audio bypassed'}
    (HERE / "build/audit/exit_comparison.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
