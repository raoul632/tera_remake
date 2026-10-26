"""Comparer un tour integre au parent resident avec ses vraies sous-routines.

Menu et C_842A recoivent des sorties imposees. C_9419/C_8CE3/C_92FE,
C_9567/C_7CC8 et le recalcul C_4C6C de C_4D46 sont executes reellement.
CS et DS sont separes : les donnees originales chevauchent les adresses
du code, donc l'espace commun des bancs numeriques isoles ne convient pas.
"""
import hashlib
import json
import struct
import subprocess
import time

from compare_combat_binary import (OriginalRoutine, ROOT, HERE, SOURCE,
                                   EXPECTED_HASH, STACK, PARENT, STOP, word)
from unicorn.x86_const import (UC_X86_REG_DS, UC_X86_REG_AX,
                              UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_IP)

DATA = 0x10000


class IntegratedRoutine(OriginalRoutine):
    def __init__(self, image, values):
        super().__init__(image, [values[6]])
        self.cpu.mem_map(DATA, 0x10000)
        self.cpu.reg_write(UC_X86_REG_DS, DATA >> 4)
        self.values = values
        self.events = []
        self.round_entries = 0
        self.continues = None
        self.target_draws = 0
        self.poison_draws = 0

    def data_word(self, address, value):
        self.cpu.mem_write(DATA + address, word(value))

    def signed_data(self, address):
        return self.signed_word(DATA + address)

    def on_instruction(self, cpu, address, size, user_data):
        """Observer appels/fin du tour, injecter uniquement les frontieres prevues."""
        if address == 0x9757:
            self.round_entries += 1
            if self.round_entries == 2:
                self.continues = 1
                cpu.emu_stop()
        elif address == 0x9AAE:
            self.continues = 0
            cpu.emu_stop()
        elif address == 0x8D20:
            bp = cpu.reg_read(UC_X86_REG_BP)
            actor = self.signed_word(bp + 6)
            offset = 10 + actor*15
            letter, raw, matches = self.values[offset+6:offset+9]
            self.events.append(actor + 1)
            self.write_word(PARENT - 4, letter - 64)
            self.write_word(PARENT - 6, raw)
            self.cpu.mem_write(DATA + 0xA859, b"\x03YES")
            self.cpu.mem_write(DATA + 0x9B3A, b"\x03YES" if matches else b"\x02NO")
            if letter == ord('F'):
                self.cpu.mem_write(DATA + 0xAFCF, b"\x01")
            cpu.reg_write(UC_X86_REG_IP, 0x8D29)
        elif address == 0x92FE:
            sp = cpu.reg_read(UC_X86_REG_SP)
            actor = self.signed_word(sp + 4)
            self.events.append(-(actor + 1))
        elif address == 0x842A:
            sp = cpu.reg_read(UC_X86_REG_SP)
            actor = self.signed_word(sp + 4)
            cpu.reg_write(UC_X86_REG_AX, self.values[10 + actor*15 + 9] & 0xFFFF)
            self.return_from_hook(6)
        elif address == 0x10DA:
            # Identifier la provenance du tirage par l'adresse de retour,
            # pas par une formule C predisant si le BIN devrait le demander.
            sp = cpu.reg_read(UC_X86_REG_SP)
            caller = self.signed_word(sp) & 0xFFFF
            if caller in (0x9998, 0x99FF):
                self.target_draws += 1
                self.draws.append(self.values[7])
            elif caller == 0x8C8F:
                self.poison_draws += 1
                self.draws.append(0 if self.values[8] else 1)
            super().on_instruction(cpu, address, size, user_data)
        elif address in {0x7D36: 0x7D8D, 0x4D6B: 0x4D9C,
                         0x4DC2: 0x4DFC, 0x4E03: 0x4EC3}:
            # Ecritures HP/HS et C_4C6C restent executees ; presentation sautee.
            cpu.reg_write(UC_X86_REG_IP, {0x7D36: 0x7D8D, 0x4D6B: 0x4D9C,
                                         0x4DC2: 0x4DFC, 0x4E03: 0x4EC3}[address])
        else:
            super().on_instruction(cpu, address, size, user_data)


def generate_cases():
    """22 monstres, 1..4 membres, deux priorites et scenarios de fin/continuation."""
    rows = list(struct.iter_unpack("<6h", (ROOT / "data/compiled program/VALMON").read_bytes()))
    cases = []
    for kind, (hp, a, b, category, initiative, variance) in enumerate(rows, 1):
        for count in range(1, 5):
            for party_first in (False, True):
                for scenario in range(5):
                    values = [kind, hp, category, a, b, initiative, 2, count-1, int(scenario == 4), 1]
                    ids = (0, 2, 5, 9)[:count]
                    for actor in range(10):
                        active = int(actor in ids)
                        agility = 255 if party_first else 0
                        letter = {0: 'T', 1: 'T', 3: 'D', 4: 'P', 5: 'C', 6: 'A'}[category]
                        raw = (0, 180, 3000, 0, 0)[scenario]
                        if scenario == 3 and actor == ids[-1]:
                            letter = 'F'
                        actor_hp = 1 if scenario == 4 else 30
                        values.extend([active, 0, actor_hp, 9, 12, agility,
                                       ord(letter), raw, 1, 0, 10, 10, 10, 5, 7])
                    cases.append(values)
    # Un CASE cible absent consomme RANDOM(count), sans attaque/defense.
    for count in range(5, 11):
        for first in (False, True):
            for target in range(count):
                values = [7, 100, 0, 0, 0, 5, 2, target, 0, 1]
                for actor in range(10):
                    values.extend([int(actor < count), 0, 30, 9, 12,
                                   255 if first else 0, ord('T'), 0, 1, 0, 10, 10, 10, 5, 7])
                cases.append(values)
    return cases


def execute_original(image, values):
    """Executer du debut d'un tour jusqu'au test original de continuation."""
    machine = IntegratedRoutine(image, values)
    kind, hp, category, a, b, initiative = values[:6]
    for actor in range(10):
        v = values[10+15*actor:25+15*actor]
        active, condition, actor_hp, magic, maximum, agility = v[:6]
        machine.cpu.mem_write(DATA + 0x9402 + actor, bytes([active]))
        machine.cpu.mem_write(DATA + 0x952C + actor*3, b"\x02" + (b"HS" if condition == 2 else b"EM" if condition == 1 else b"OK"))
        machine.data_word(0x74DC + actor*2, actor_hp)
        machine.data_word(0x74F0 + actor*2, magic)
        machine.data_word(0x7518 + actor*2, maximum)
        stats = [0] * 10
        stats[7], stats[1], stats[4], stats[9], stats[2], stats[5] = agility, *v[10:15]
        machine.cpu.mem_write(DATA + 0x7414 + actor*10, bytes(stats))
    row = 0xA745 + kind*12
    for offset, value in ((2, a), (4, b), (6, category), (8, initiative)):
        machine.data_word(row + offset, value)
    machine.data_word(0x75E8, 1)
    machine.cpu.mem_write(DATA + 0xA745, bytes([values[9]]))
    machine.write_word(PARENT + 6, kind)
    machine.write_word(PARENT - 8, hp)
    machine.cpu.reg_write(UC_X86_REG_BP, PARENT)
    machine.cpu.reg_write(UC_X86_REG_SP, STACK)
    machine.cpu.emu_start(0x9757, STOP, timeout=1000000, count=40000)
    if machine.continues is None or machine.draws:
        raise RuntimeError("Tour sans frontiere de fin ou tirages non consommes")
    flags = [machine.cpu.mem_read(DATA + address, 1)[0] for address in (0xA746, 0xAFCF, 0xA745)]
    result = [machine.signed_word(PARENT - 8), *flags, machine.continues,
              machine.target_draws, len(machine.events), *machine.events, machine.poison_draws]
    for actor in range(10):
        condition = bytes(machine.cpu.mem_read(DATA + 0x952C + actor*3, 3))
        result.extend([machine.signed_data(0x74DC + actor*2), machine.signed_data(0x74F0 + actor*2),
                       machine.signed_data(0x7518 + actor*2), machine.signed_data(0x7504 + actor*2),
                       2 if condition == b"\x02HS" else 1 if condition == b"\x02EM" else 0])
    return result


def main():
    """Compiler le port et comparer traces et donnees d'un tour, pas seulement les degats."""
    started = time.monotonic()
    image = SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED_HASH:
        raise SystemExit("Empreinte TERA.COM differente")
    subprocess.run(["cmake", "--build", str(HERE / "build/cmake")], check=True)
    cases = generate_cases()
    protocol = "".join(f"I {len(v)} {' '.join(map(str, v))}\n" for v in cases)
    output = subprocess.run([str(HERE / "build/cmake/binary_comparison_runner.exe")], input=protocol,
                            text=True, capture_output=True, check=True).stdout.splitlines()
    if len(output) != len(cases):
        raise RuntimeError("Nombre de sorties C incorrect")
    failures = []
    for index, (values, line) in enumerate(zip(cases, output), 1):
        binary, c = execute_original(image, values), list(map(int, line.split()))
        if binary != c:
            failures.append({"inputs": values, "binary": binary, "c": c})
        if index % 200 == 0:
            print(f"Tours compares {index}/{len(cases)} ; ecarts {len(failures)}", flush=True)
    report = {"source_sha256": EXPECTED_HASH, "cases": len(cases), "mismatches": len(failures),
              "first_mismatches": failures[:3], "seconds": round(time.monotonic()-started, 2),
              "boundaries": "injected menu/raw defense/RANDOM; no fleeing relocation or rewards"}
    destination = HERE / "build/audit/encounter_comparison.json"
    destination.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
