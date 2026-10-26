"""Executer preparation/cible du tour et mise a jour HS dans TERA.COM.

Reutilise le chargement et l'injection de RANDOM du banc de combat.
Arrete le parent a l'appel de riposte : aucune formule de preparation
n'est remplacee. Les attaques individuelles ne font pas partie de ce test.
"""
from collections import Counter
import hashlib
import json
import random
import struct
import subprocess
import time

from compare_combat_binary import (OriginalRoutine, SOURCE, EXPECTED_HASH,
                                   HERE, PARENT, STACK, STOP, word)
from unicorn.x86_const import UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_IP


class RoundRoutine(OriginalRoutine):
    def __init__(self, image, draws):
        super().__init__(image, draws)
        self.party_first = None
        self.target = None

    def on_instruction(self, cpu, address, size, user_data):
        """Observer les branches ; remplacer uniquement les attaques externes."""
        if address == 0x9977:
            self.party_first = 1
        elif address == 0x99F9:
            self.party_first = 0
        if address == 0x9419:
            # Le groupe ne fait rien dans ce test : aucun degat/fuite.
            self.return_from_hook(2)
        elif address == 0x92FE:
            sp = cpu.reg_read(UC_X86_REG_SP)
            self.target = struct.unpack("<H", cpu.mem_read(sp + 4, 2))[0]
            cpu.emu_stop()
        elif address == 0x7D36:
            # Affichage apres ecriture HP/HS, avant accumulation A745.
            cpu.reg_write(UC_X86_REG_IP, 0x7D8D)
        else:
            super().on_instruction(cpu, address, size, user_data)


def generate_cases():
    """Groupes disperses, effectifs 1..4, egalites, MP aux bornes et etats HS."""
    cases, rng = [], random.Random(1986)
    for profile in (0, 10, 255):
        cases.append(("V", [profile] * 10))
    for _ in range(100):
        cases.append(("V", [rng.randrange(256) for _ in range(10)]))
    for hp in (-32768, -3, 0, 1, 12, 32767):
        for condition in range(3):
            for previous in range(2):
                cases.append(("S", [hp, condition, previous]))
    for count in range(1, 5):
        for initiative in (-32768, 0, 5, 255, 32767):
            for draw in range(5):
                for target in range(count):
                    ids = sorted(rng.sample(range(10), count))
                    values = [initiative, draw, target]
                    for actor_id in range(10):
                        condition = rng.randrange(2) if actor_id in ids else rng.randrange(3)
                        active = 1 if actor_id in ids else int(condition == 2)
                        magic = rng.choice([-32768, -1, 0, 2, 5, 32767])
                        limit = rng.choice([-32768, 0, 2, 5, 32767])
                        values.extend([active, condition, rng.choice([0, 5, 10, 255]), magic, limit])
                    cases.append(("R", values))
    # Egalite explicite : moyenne 7 et seuil 7 ; non participants rapides.
    values = [5, 4, 0]
    for actor_id in range(10):
        values.extend([int(actor_id in (1, 4, 9)), 0,
                       {1: 4, 4: 8, 9: 11}.get(actor_id, 255), 2, 5])
    cases.append(("R", values))
    return cases


def execute_original(image, mode, v):
    """Placer les entrees en memoire et lire des sorties ecrites par le BIN."""
    if mode == "V":
        machine = RoundRoutine(image, [])
        machine.cpu.mem_write(0x7414 + 10, bytes(v))
        machine.call(0x4C6C, [1], has_result=False)
        return [machine.signed_word(0x7506), machine.signed_word(0x751A)]
    if mode == "S":
        hp, condition, previous = v
        machine = RoundRoutine(image, [])
        machine.write_word(0x74DE, hp)
        machine.cpu.mem_write(0x952F, b"\x02" + (b"HS" if condition == 2 else b"EM" if condition == 1 else b"OK"))
        machine.cpu.mem_write(0xA745, bytes([previous]))
        machine.call(0x7CC8, [1], has_result=False)
        final_condition = bytes(machine.cpu.mem_read(0x952F, 3))
        return [machine.signed_word(0x74DE), 2 if final_condition == b"\x02HS" else condition,
                machine.cpu.mem_read(0xA745, 1)[0]]
    machine = RoundRoutine(image, [v[1], v[2]])
    for actor_id in range(10):
        active, condition, agility, magic, limit = v[3+5*actor_id:8+5*actor_id]
        machine.cpu.mem_write(0x9402 + actor_id, bytes([active]))
        machine.cpu.mem_write(0x952C + actor_id*3, b"\x02" +
                              (b"HS" if condition == 2 else b"EM" if condition == 1 else b"OK"))
        machine.cpu.mem_write(0x741B + actor_id*10, bytes([agility]))
        machine.write_word(0x74F0 + actor_id*2, magic)
        machine.write_word(0x7518 + actor_id*2, limit)
    machine.write_word(PARENT + 6, 1)
    machine.write_word(PARENT - 8, 100)  # empeche la victoire avant riposte
    machine.write_word(0xA759, v[0])    # colonne 5, monstre 1
    machine.cpu.reg_write(UC_X86_REG_BP, PARENT)
    machine.cpu.reg_write(UC_X86_REG_SP, STACK)
    machine.cpu.emu_start(0x9757, STOP, timeout=1000000, count=20000)
    if machine.target is None or machine.draws:
        raise RuntimeError("Tour sans cible ou suite de tirages non consommee")
    if machine.random_bounds[0] != 5:
        raise RuntimeError("Borne d'initiative differente de 5")
    count = machine.signed_word(PARENT - 0x18)
    if machine.random_bounds[1] != count:
        raise RuntimeError("Borne de cible differente de l'effectif")
    ids = [machine.signed_word(PARENT - 0x10 - slot*2) for slot in range(count)]
    return [count, *ids, machine.signed_word(PARENT - 10), machine.party_first,
            machine.target, *[machine.signed_word(0x74F0 + actor_id*2) for actor_id in range(10)]]


def main():
    """Comparer au pont C compile et enregistrer uniquement un rapport local."""
    started = time.monotonic()
    image = SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED_HASH:
        raise SystemExit("Empreinte TERA.COM differente : audit a revalider")
    subprocess.run(["cmake", "--build", str(HERE / "build/cmake")], check=True)
    cases = generate_cases()
    protocol = "".join(f"{mode} {len(v)} {' '.join(map(str, v))}\n" for mode, v in cases)
    output = subprocess.run([str(HERE / "build/cmake/binary_comparison_runner.exe")],
                            input=protocol, text=True, capture_output=True, check=True).stdout.splitlines()
    if len(output) != len(cases):
        raise RuntimeError("Nombre de sorties C incorrect")
    counts, failures = Counter(), []
    for (mode, values), line in zip(cases, output):
        binary, c = execute_original(image, mode, values), list(map(int, line.split()))
        counts[mode] += 1
        if binary != c:
            failures.append({"mode": mode, "inputs": values, "binary": binary, "c": c})
    report = {"source_sha256": EXPECTED_HASH, "cases": dict(counts),
              "mismatches": len(failures), "first_mismatches": failures[:10],
              "seconds": round(time.monotonic() - started, 2),
              "boundaries": "attacks bypassed, stop before retaliation, injected RANDOM"}
    destination = HERE / "build/audit/round_comparison.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
