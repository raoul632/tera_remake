"""Comparer le C a des routines originales executees par Unicorn x86-16.

Le fichier COM est charge sans modification sur disque. Les seuls hooks
imposent des entrees/tirages et sautent des sequences d'affichage listees.
Les calculs, comparaisons, divisions et ensembles Pascal sont executes.
Ce banc n'est pas une emulation DOS complete ni une validation du jeu.
"""
from collections import Counter
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parent
sys.path.insert(0, str(HERE / "build/emulator_deps"))
try:
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, __version__
    from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_BP,
                                  UC_X86_REG_IP, UC_X86_REG_SP)
except ImportError:
    raise SystemExit("Installer unicorn dans reconstruction/build/emulator_deps")

SOURCE = ROOT / "data/compiled program/TERA.COM"
EXPECTED_HASH = "22fb836aa93f46dc2437e8b9586415d13552bfac35e8ac6647ba6157fd9b1a37"
STOP, STACK, PARENT = 0xF100, 0xE000, 0xD000

# Intervalles d'affichage/temporisation uniquement, entree incluse,
# destination exclue. Aucune formule ou ecriture HP/MP n'est sautee.
DISPLAY_SKIPS = {0x8B6A: 0x8B70, 0x8B8A: 0x8B9A,
                 0x8BAC: 0x8C33, 0x8C1E: 0x8C33,
                 0x8CEF: 0x8D20, 0x9298: 0x92EF,
                 0x9364: 0x9410, 0x9400: 0x9410}


def word(value):
    """Encoder un mot de la machine, y compris une valeur signee."""
    return struct.pack("<H", value & 0xFFFF)


class OriginalRoutine:
    """Charger la meme image pour chaque cas : aucun etat ne fuit entre tests."""

    def __init__(self, image, draws=(), menu=None, defense=None):
        self.cpu = Uc(UC_ARCH_X86, UC_MODE_16)
        self.cpu.mem_map(0, 0x10000)
        self.cpu.mem_write(0x100, image)
        self.draws = list(draws)
        self.random_bounds = []
        self.menu = menu
        self.defense = defense
        self.skips = Counter()
        self.cpu.reg_write(UC_X86_REG_BP, PARENT)
        self.write_word(PARENT - 2, PARENT)
        self.cpu.hook_add(UC_HOOK_CODE, self.on_instruction)

    def write_word(self, address, value):
        """Initialiser une donnee ou un cadre Pascal en memoire emulee."""
        self.cpu.mem_write(address, word(value))

    def signed_word(self, address):
        return struct.unpack("<h", self.cpu.mem_read(address, 2))[0]

    def return_from_hook(self, cleanup=0):
        """Reproduire RET n pour un appel volontairement remplace."""
        sp = self.cpu.reg_read(UC_X86_REG_SP)
        target = struct.unpack("<H", self.cpu.mem_read(sp, 2))[0]
        self.cpu.reg_write(UC_X86_REG_SP, sp + 2 + cleanup)
        self.cpu.reg_write(UC_X86_REG_IP, target)

    def on_instruction(self, cpu, address, size, user_data):
        """Conserver les regles BIN ; remplacer seulement les frontieres annoncees."""
        if address == 0x10DA:
            bound = cpu.reg_read(UC_X86_REG_AX)
            if not self.draws:
                raise RuntimeError("Tirage original inattendu")
            draw = self.draws.pop(0)
            if not 0 <= draw < bound:
                raise RuntimeError(f"Tirage {draw} hors RANDOM({bound})")
            self.random_bounds.append(bound)
            cpu.reg_write(UC_X86_REG_AX, draw)
            self.return_from_hook()
        elif address == 0x8D20 and self.menu is not None:
            # Saute la saisie et C_842A, dont la sortie est un input de ce test.
            cpu.reg_write(UC_X86_REG_IP, 0x8D29)
        elif address == 0x842A and self.defense is not None:
            # La defense brute est une entree de l'API de resolution C.
            cpu.reg_write(UC_X86_REG_AX, self.defense & 0xFFFF)
            self.return_from_hook(6)
        elif address in DISPLAY_SKIPS:
            self.skips[f"{address:04X}"] += 1
            cpu.reg_write(UC_X86_REG_IP, DISPLAY_SKIPS[address])

    def call(self, entry, args, has_result=True, allow_unused_draw=False):
        """Construire l'ABI Pascal : arguments inverses, emplacement resultat."""
        values = [STOP, *reversed(args)] + ([0] if has_result else [])
        self.cpu.mem_write(STACK, b"".join(word(v) for v in values))
        self.cpu.reg_write(UC_X86_REG_SP, STACK)
        self.cpu.emu_start(entry, STOP, timeout=1000000, count=20000)
        if self.cpu.reg_read(UC_X86_REG_IP) != STOP:
            raise RuntimeError(f"Routine {entry:04X} sans retour apres la limite")
        if self.draws and not allow_unused_draw:
            raise RuntimeError("Tirages fournis non consommes")
        expected_sp = STACK + len(values) * 2
        if self.cpu.reg_read(UC_X86_REG_SP) != expected_sp:
            raise RuntimeError("Pile Pascal desequilibree")
        return struct.unpack("<h", word(self.cpu.reg_read(UC_X86_REG_AX)))[0]


def generate_cases():
    """Cas reproductibles, frontieres et donnees VALMON locales en lecture seule."""
    rng = random.Random(1986)  # generation du banc, jamais le RNG du jeu
    cases = []
    for action in range(1, 10):
        for bonus in range(4):
            for factor in (3, 5, 17):
                for profile in (10, 0, 255):
                    stats = [0] + [profile] * 9
                    skills = [0] + [3 if profile == 10 else profile] * 9
                    cases.append(("A", [action, bonus, factor, factor, *stats, *skills]))
    for _ in range(200):
        cases.append(("A", [rng.randint(1, 9), rng.randrange(4), rng.randint(3, 17),
                             rng.randint(3, 17), *[rng.randrange(256) for _ in range(20)]]))
    for _ in range(120):
        cases.append(("B", [rng.randint(1, 9), rng.randint(1, 9), rng.randint(3, 17), 0,
                             *[rng.randrange(256) for _ in range(20)]]))
    # Frontieres C_8315 : 11*4=44 puis 15*3=45.
    for stat, skill, system_d, factor in ((8, 0, 5, 4), (11, 1, 3, 3)):
        stats, skills = [0] * 10, [0] * 10
        stats[6], skills[6], skills[7] = stat, skill, system_d
        cases.append(("B", [6, 6, factor, 0, *stats, *skills]))
    for required in (3, 6, 9, 12, 15, 18, 21, 25):
        for offset in range(4):
            slots = [0] * 10
            if offset < 3:
                slots[9] = required - offset
            cases.append(("E", [required, *slots]))
        # Un objet faible en premiere position ne masque pas le meilleur.
        cases.append(("E", [required, required - 2, *([0] * 8), required]))
    data = (ROOT / "data/compiled program/VALMON").read_bytes()
    for hp, a, b, category, initiative, variance in struct.iter_unpack("<6h", data):
        for letter in "LTJADPC":
            for magic in (2, 3, 5, 6, 9):
                for raw in (0, 180, -7171):
                    for matches in (0, 1):
                        cases.append(("H", [category, a, b, ord(letter), raw, magic, hp, matches]))
    # Donnees synthetiques pour debordements et seuils : complement aux
    # valeurs VALMON, sans presenter ces etats comme une partie atteignable.
    for a in (-32768, 0, 32767):
        for raw in (-32768, 0, 32767):
            for hp in (-32768, 2, 3, 32767):
                cases.append(("H", [1, a, 11, ord('T'), raw, 9, hp, 0]))
    for kind in range(1, 23):
        for hp in (0, 2, 20, 32767, -32768):
            for defense in (0, 198, 264, -7171):
                for zero in (0, 1):
                    cases.append(("M", [kind, hp, defense, zero]))
    return cases


def execute_original(image, mode, v):
    """Adapter les entrees C a la memoire originale sans recalculer sa sortie."""
    machine = OriginalRoutine(image)
    if mode in ("A", "B"):
        machine.cpu.mem_write(0x7414 + 10, bytes(v[4:14]))
        machine.cpu.mem_write(0x7478 + 10, bytes(v[14:24]))
        if mode == "B":
            machine.draws = [v[2] - 3]
            return [machine.call(0x8315, [1, v[0], v[1]])], machine
        action, bonus, entry, base = v[:4]
        # Fixer une permutation d'aides admissible ; le C recoit le bonus.
        machine.cpu.mem_write(0x75EA, b"\x03\x06\x09\x0c")
        required = {1: 15, 2: 18, 3: 12, 4: 3, 5: 9, 6: 6, 7: 21}.get(action)
        slots = [0] * 10
        if required and bonus:
            slots[0] = required - (3 - bonus)
        machine.cpu.mem_write(0x73B0 + 10, bytes(slots))
        machine.draws = [entry - 3] + ([base - 3] if action <= 5 or action == 9 else [])
        value = machine.call(0x842A, [1, action])
        return [value], machine
    if mode == "E":
        machine.cpu.mem_write(0x73B0 + 10, bytes(v[1:]))
        return [machine.call(0x8391, [1, v[0]])], machine
    if mode == "H":
        category, a, b, letter, raw, magic, hp, matches = v
        machine.menu = True
        machine.write_word(0xA751 + 2, a)
        machine.write_word(0xA751 + 4, b)
        machine.write_word(0xA751 + 6, category)
        machine.write_word(0x74F0 + 2, magic)
        machine.write_word(PARENT - 4, letter - 64)
        machine.write_word(PARENT - 6, raw)
        machine.write_word(PARENT - 8, hp)
        machine.cpu.mem_write(0xAFCF, b"\x00")
        machine.cpu.mem_write(0xA859, b"\x03YES")
        machine.cpu.mem_write(0x9B3A, b"\x03YES" if matches else b"\x02NO")
        machine.call(0x8CE3, [1, 1], has_result=False)
        after = machine.signed_word(PARENT - 8)
        damage = struct.unpack("<h", word(hp - after))[0]
        return [machine.cpu.mem_read(0xA74C, 1)[0], damage,
                machine.signed_word(0x74F2), after], machine
    kind, hp, defense, zero = v
    machine.defense = defense
    machine.write_word(PARENT - 8, hp)
    machine.write_word(0x74DE, 100)
    machine.write_word(0x75E8, 1)
    machine.cpu.mem_write(0x952F, b"\x02OK")
    # Fournir un tirage disponible : le BIN decide seul s'il le consomme.
    # Un tirage non consomme est admissible pour une riposte manquee.
    machine.draws = [0 if zero else 1]
    machine.call(0x92FE, [1, kind], has_result=False, allow_unused_draw=True)
    damage = struct.unpack("<h", word(100 - machine.signed_word(0x74DE)))[0]
    return [damage, len(machine.random_bounds), int(bytes(machine.cpu.mem_read(0x952F, 3)) == b"\x02EM")], machine


def main():
    """Compiler le pont, comparer chaque cas et conserver un rapport local."""
    started = time.monotonic()
    image = SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED_HASH:
        raise SystemExit("Empreinte TERA.COM differente : audit a revalider")
    subprocess.run(["cmake", "--build", str(HERE / "build/cmake")], check=True)
    cases = generate_cases()
    protocol = "".join(f"{mode} {len(v)} {' '.join(map(str, v))}\n" for mode, v in cases)
    runner = HERE / "build/cmake/binary_comparison_runner.exe"
    result = subprocess.run([str(runner)], input=protocol, text=True, capture_output=True, check=True)
    outputs = result.stdout.splitlines()
    if len(outputs) != len(cases):
        raise RuntimeError("Nombre de sorties C incorrect")
    counts, skips, failures = Counter(), Counter(), []
    for index, ((mode, values), output) in enumerate(zip(cases, outputs), 1):
        actual, machine = execute_original(image, mode, values)
        expected = list(map(int, output.split()))
        counts[mode] += 1
        skips.update(machine.skips)
        if actual != expected:
            failures.append({"mode": mode, "inputs": values, "binary": actual, "c": expected})
        if index % 1000 == 0:
            print(f"Comparaison {index}/{len(cases)} ; ecarts {len(failures)}", flush=True)
    report = {"source_sha256": EXPECTED_HASH, "emulator": "unicorn",
              "emulator_version": __version__,
              "cases": dict(counts), "total": sum(counts.values()),
              "mismatches": len(failures), "first_mismatches": failures[:20],
              "display_skips": dict(skips), "rng": "injected, algorithm not validated",
              "seconds": round(time.monotonic() - started, 2)}
    destination = HERE / "build/audit/binary_comparison.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
