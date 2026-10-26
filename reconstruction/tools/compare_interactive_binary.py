"""Combats entiers COM/C, memes touches et graines, RNG original execute.

Les calculs, le menu, les confirmations, le mot, les cartes et les fins
sont executes. Seul le clavier BIOS et les sorties texte/video/son/delai
sont abstraits. Comparer l'etat a CHAQUE touche, y compris les annulations.
"""
import hashlib
import json
import random
import struct
import subprocess
import sys
import time

from compare_combat_binary import OriginalRoutine, ROOT, HERE, SOURCE, EXPECTED_HASH, STACK, STOP, word, DISPLAY_SKIPS
from unicorn.x86_const import UC_X86_REG_DS, UC_X86_REG_AX, UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_CX, UC_X86_REG_DX
sys.path.insert(0, str(HERE / 'frontend'))
from combat import demo_game, pack_game

DATA = 0x10000
FRAME = STACK - 2
ROWS = list(struct.iter_unpack('<6h', (ROOT / 'data/compiled program/VALMON').read_bytes()))


class LiveRoutine(OriginalRoutine):
    """Machine isolee ; aucun hook ne remplace RANDOM ou une regle de combat."""
    def __init__(self, image, game, keys):
        super().__init__(image)
        self.cpu.mem_map(DATA, 0x10000)
        self.cpu.reg_write(UC_X86_REG_DS, DATA >> 4)
        self.keys = list(keys)
        self.states = []
        self.calls = []
        self.rounds = 0
        self.game = game
        self.stop_reason = None
        self.cpu.mem_write(DATA + 0x1FC, struct.pack('<I', game['seed']))
        for file, address in [('VERBES', 0x5F98), ('MESSAGE', 0x63C0),
                              ('MESSAGE2', 0x9C24), ('MESSAGE3', 0xA14C), ('VALMON', 0xA751)]:
            self.cpu.mem_write(DATA + address, (ROOT / 'data/compiled program' / file).read_bytes())
        for id, actor in enumerate(game['actors']):
            self.cpu.mem_write(DATA + 0x9402 + id, bytes([actor['active']]))
            for name, address in [('stats', 0x7414), ('skills', 0x7478), ('equipment', 0x73B0)]:
                self.cpu.mem_write(DATA + address + id*10, bytes(actor[name]))
            state = actor['condition'].encode('ascii')
            self.cpu.mem_write(DATA + 0x952C + id*3, bytes([len(state)]) + state.ljust(2, b'\0'))
            for name, address in [('hp', 0x74DC), ('magic', 0x74F0), ('hp_limit', 0x7504), ('magic_limit', 0x7518)]:
                self.data_word(address + 2*id, actor[name])
        for name, address in [('world_id', 0x9B76), ('difficulty', 0x75E8),
                              ('position', 0x75D2), ('previous_position', 0x9B72),
                              ('money', 0x75D4), ('experience', 0x75D6)]:
            self.data_word(address, game[name])
        self.cpu.mem_write(DATA + 0xA748, bytes([game['on_alfol']]))
        self.cpu.mem_write(DATA + 0xAFD1, bytes([game['location_changed']]))
        self.cpu.mem_write(DATA + 0xAFCE, bytes([game.get('recruit_ready',0)]))
        self.cpu.mem_write(DATA + 0xA74A, bytes([game.get('sound',0)]))
        self.cpu.mem_write(DATA + 0x75EA, bytes(game['helpers']))
        secret = game['secret_word'].encode('ascii')
        self.cpu.mem_write(DATA + 0x9B3A, bytes([len(secret)])+secret.ljust(9, b'\0'))
        for position, value in enumerate(game['maps_column7']):
            self.cpu.mem_write(DATA + 0x75F4 + position*7, bytes([value]))

    def data_word(self, address, value):
        self.cpu.mem_write(DATA+address, word(value))

    def state(self, phase, actor, attempt):
        """Etat durable commun aux deux versions ; presentation exclue."""
        seed = struct.unpack('<I', self.cpu.mem_read(DATA+0x1FC, 4))[0]
        values = [phase, actor, attempt, seed, self.signed_word(FRAME-0x1C), self.signed_word(FRAME-8)]
        values += [self.cpu.mem_read(DATA+a, 1)[0] for a in (0xA746, 0xAFCF, 0xA745)]
        values += [self.signed_word(DATA+a) for a in (0x75D2, 0x9B72, 0x75D4, 0x75D6)]
        values += [self.cpu.mem_read(DATA+0xAFD1, 1)[0], self.rounds]
        for id in range(10):
            values += [self.signed_word(DATA+a+2*id) for a in (0x74DC, 0x74F0, 0x7504, 0x7518)]
            raw = bytes(self.cpu.mem_read(DATA+0x952C+3*id, 3))
            values += list(raw[1:1+raw[0]].ljust(2, b'\0'))
        values += [self.cpu.mem_read(DATA+0xAFCC, 1)[0]]
        values += [self.cpu.mem_read(DATA+0xAFCE, 1)[0]]
        return values

    def on_instruction(self, cpu, address, size, user_data):
        """Abstraire uniquement les I/O identifiees ; observer les frontieres."""
        if address == 0x10DA:
            self.calls.append(cpu.reg_read(UC_X86_REG_AX))  # Execute, sans injection.
        elif address == 0x4277:
            sp = cpu.reg_read(UC_X86_REG_SP)
            caller = self.signed_word(sp) & 65535
            phase = {0x87E0: 0, 0x4824: 1, 0x42D4: 2}.get(caller)
            if phase is None:
                raise RuntimeError(f'Lecture clavier inattendue {caller:04X}')
            actor = self.signed_word(DATA+0xAFC9)
            attempt = self.cpu.mem_read(DATA+0xAFCD, 1)[0]
            self.states.append(self.state(phase, actor, attempt))
            if not self.keys:
                self.stop_reason = 'input'
                cpu.emu_stop()
                return
            key = self.keys.pop(0)
            if ord('a') <= key <= ord('z'):
                key -= 32
            cpu.reg_write(UC_X86_REG_AX, key)
            self.return_from_hook(1)
        elif address == 0x9757:
            self.rounds += 1
        elif address == 0x9B45:
            self.stop_reason = 'done'
            self.states.append(self.state(3, 0, 0))
            cpu.emu_stop()
        elif address == 0x8CE3:
            sp = cpu.reg_read(UC_X86_REG_SP)
            self.data_word(0xAFC9, self.signed_word(sp+4))
        elif address in (0x2558, 0x1022, 0x4503, 0x0213, 0x059C, 0x05C3):
            self.return_from_hook()
        elif address in (0x0371, 0x037F):
            cpu.reg_write(UC_X86_REG_AX, 1)
            self.return_from_hook()
        elif address == 0x2926:
            sp = cpu.reg_read(UC_X86_REG_SP)
            self.return_from_hook(cpu.mem_read(sp+2, 1)[0]+1)
        elif address in {0x4779: 40, 0x289D: 2, 0x28B9: 2, 0x2DAE: 2, 0x2DAB: 2, 0x0DAA: 2}:
            self.return_from_hook({0x4779: 40, 0x289D: 2, 0x28B9: 2, 0x2DAE: 2, 0x2DAB: 2, 0x0DAA: 2}[address])
        elif address == 0x295D:
            sp = cpu.reg_read(UC_X86_REG_SP)
            inline = self.signed_word(sp) & 65535
            length = cpu.mem_read(inline, 1)[0]
            self.write_word(sp, inline+length+1)
            self.return_from_hook()
        elif address == 0x4807:
            cpu.reg_write(UC_X86_REG_IP, 0x4820)
        elif address in {0x7D36: 0x7D8D, 0x4D6B: 0x4D9C,
                         0x4DC2: 0x4DFC, 0x4E03: 0x4EC3, 0x8765: 0x8783,
                         0x9AC3: 0x9AE6, 0x9AF2: 0x9B0F}:
            cpu.reg_write(UC_X86_REG_IP, {0x7D36: 0x7D8D, 0x4D6B: 0x4D9C,
                0x4DC2: 0x4DFC, 0x4E03: 0x4EC3, 0x8765: 0x8783,
                0x9AC3: 0x9AE6, 0x9AF2: 0x9B0F}[address])
        elif address == 0x9364:
            cpu.reg_write(UC_X86_REG_IP, 0x938D) # Garder SoundFlag/C6221, pas les delais.
        elif address == 0x6221:
            self.return_from_hook(6) # Onde deterministe, aucun RANDOM.
        elif address in DISPLAY_SKIPS and address not in (0x8B8A, 0x9400, 0x9364):
            # Ces deux branches appellent C6D32 : sa couleur consomme le RNG.
            cpu.reg_write(UC_X86_REG_IP, 0x8BF7 if address == 0x8BAC else DISPLAY_SKIPS[address])

    def run(self):
        """Appel reel C_838A ; s'arreter a la fin ou a la derniere entree fournie."""
        self.cpu.mem_write(STACK, word(STOP)+word(self.game['aggressor'])+word(self.game['monster_type']))
        self.cpu.reg_write(UC_X86_REG_SP, STACK)
        self.cpu.emu_start(0x838A, STOP, timeout=10000000, count=2000000)
        if not self.states or self.stop_reason is None:
            raise RuntimeError(f'Aucune frontiere, IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def c_states(game, keys):
    """Memes donnees/touches au pont du vrai moteur, sans formule Python."""
    result = subprocess.run([str(HERE/'build/cmake/tera_combat_bridge.exe')],
                            input=pack_game(game, ROWS)+''.join(f'{key}\n' for key in keys),
                            text=True, capture_output=True, timeout=10)
    if result.returncode not in (0, 4):
        raise RuntimeError(result.stderr or f'Pont C {result.returncode}')
    states = []
    for line in result.stdout.splitlines():
        if line.startswith('S '):
            v = list(map(int, line.split()[1:]))
            # Le dernier acteur/essai du C reste un contexte de presentation.
            # Le COM n'en definit aucun apres son retour : neutraliser a DONE.
            if v[0] == 3:
                v[1:3] = [0, 0]
            states.append(v[:3]+v[4:])
    return states


def compare_rng(image):
    """Graines avec bits hauts/overflow, bornes du jeu et runtime original."""
    rng = random.Random(1986)
    seeds = [0, 1, 0xFFFFFFFF, 0x80000000, 0x01000000, 0xFFFFFF] + [rng.getrandbits(32) for _ in range(64)]
    cases = [(seed, bound) for seed in seeds for bound in (1, 3, 5, 10, 15, 50, 90, 99, 255, 32768, 65535)]
    protocol = ''.join(f'G 3 {seed & 65535} {seed >> 16} {bound}\n' for seed, bound in cases)
    lines = subprocess.run([str(HERE/'build/cmake/binary_comparison_runner.exe')], input=protocol,
                           text=True, capture_output=True, check=True).stdout.splitlines()
    for (seed, bound), line in zip(cases, lines):
        machine = LiveRoutine(image, demo_game(1, seed), [])
        machine.cpu.reg_write(UC_X86_REG_AX, bound)
        value = machine.call(0x10DA, [], has_result=False)
        actual = [struct.unpack('<I', machine.cpu.mem_read(DATA+0x1FC, 4))[0], value]
        if actual != list(map(int, line.split())):
            raise AssertionError((seed, bound, actual, line))
    return len(cases)


def generate_cases():
    """22 types, 1..4 membres, priorites, menu, mots, cartes et compteurs."""
    cases = []
    prefixes = ['', '?', '??', 'tn', 'tntn', 'TNT\r', 'coAB\bC\r',
                'COABCDEFGHIJ', 'CO12test\r', 'CO\r', 'FO', 'f\r', 'FNTNTN']
    for kind, row in enumerate(ROWS, 1):
        letter = {0: 'T', 1: 'T', 3: 'D', 4: 'P', 5: 'C', 6: 'A'}[row[3]]
        action = letter+'O'+('TEST\r' if letter == 'C' else '')
        for count in range(1, 5):
            for first in (False, True):
                game = demo_game(kind, 1986 + kind*17 + count)
                for i, actor in enumerate(game['actors']):
                    actor['active'] = int(i in (0, 2, 5, 9)[:count])
                    actor['stats'][7] = 255 if first else 0
                cases.append((f'type{kind}/party{count}/first{first}', game, action*40))
        for prefix in prefixes:
            game = demo_game(kind, kind*65537)
            cases.append((f'type{kind}/menu{prefix!r}', game, prefix+action*20))
        # Tous les choix face a chaque categorie, pas seulement son attaque efficace.
        for key in 'TLJADPC':
            game = demo_game(kind, 0xFFFFFFFF-kind)
            game['helpers'] = [12, 9, 6, 3]
            if key == 'T':
                game['actors'][0]['equipment'] = [0]*10
            prefix = key+'O'+('WRONG\r' if key == 'C' else '')
            cases.append((f'type{kind}/choice{key}', game, prefix+action*20))
    # Chaque monde, carte creuse avec de nombreux rejets ; branches Alfol.
    for world in range(12):
        for alfol in (0, 1):
            game = demo_game(7, 0x12345678+world)
            game.update(world_id=world, on_alfol=alfol, previous_position=-123)
            game['maps_column7'] = [0]*1100
            if world < 11:
                game['maps_column7'][world*100+94] = 3
            cases.append((f'escape/world{world}/alfol{alfol}', game, 'FO'))
    # Le recrutement active parfois plusieurs compagnons a la fois :
    # verifier les groupes excedant les quatre emplacements du CASE DOS.
    for count in range(5, 11):
        for seed in range(12):
            game = demo_game(7, seed)
            for i, actor in enumerate(game['actors']):
                actor['active'] = int(i < count)
                actor['stats'][7] = 0 if seed % 2 else 255
            cases.append((f'overflow/party{count}/seed{seed}', game, 'TOTOFO'))
    generator = random.Random(0x1986)
    for index in range(80):
        game = demo_game(generator.randrange(1, 23), generator.getrandbits(32))
        game['aggressor'] = generator.randrange(10)
        game['difficulty'] = generator.randrange(11)
        game['money'], game['experience'] = 32767, 32767
        for i, actor in enumerate(game['actors']):
            actor['stats'] = [generator.randrange(12) for _ in range(10)]
            actor['skills'] = [generator.randrange(6) for _ in range(10)]
            actor['equipment'] = [generator.randrange(26) for _ in range(10)]
            actor['hp'] = generator.choice([1, 5, 30, -1])
            actor['magic'] = generator.choice([-1, 0, 2, 3, 5, 6, 20, 32767])
            actor['condition'] = generator.choice(['OK', 'EM', 'HS'])
            actor['active'] = int(i in (0, 2, 5, 9)[:1+index%4])
        keys = ''.join(generator.choice(['TO', 'LO', 'JO', 'AO', 'DO', 'PO',
                                        'COtest\r', 'TN', '?', 'FO']) for _ in range(20))
        cases.append((f'profiles{index}', game, keys))
    # Sons actifs et couleurs : leurs tirages doivent survivre a l'abstraction
    # des sorties DOS. Chaque action, chaque type et les victoires sont couverts.
    for kind in range(1, 23):
        for letter in 'TLJADPC':
            game = demo_game(kind, 1986 + kind)
            game['sound'] = 1
            game['actors'][0]['hp'] = 10000
            game['actors'][0]['magic'] = 200
            game['actors'][0]['stats'][7] = 255
            action = letter + 'O' + ('TEST\r' if letter == 'C' else '')
            cases.append((f'audio/type{kind}/{letter}', game, action * 4 + 'FO'))
    return cases


def main():
    started = time.monotonic()
    image = SOURCE.read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED_HASH:
        raise SystemExit('Empreinte COM differente')
    subprocess.run(['cmake', '--build', str(HERE/'build/cmake')], check=True)
    rng_cases = compare_rng(image)
    failures = []
    boundaries = completed = rejected_draws = 0
    cases = generate_cases()
    for index, (name, game, text) in enumerate(cases, 1):
        keys = list(map(ord, text))
        machine = LiveRoutine(image, game, keys)
        binary, c = machine.run(), c_states(game, keys)
        boundaries += len(c)
        completed += int(c[-1][0] == 3)
        if name.startswith('escape/') and game['world_id'] < 11:
            rejected_draws += machine.calls.count(90)-1
        if binary != c:
            mismatch = next((i for i, pair in enumerate(zip(binary, c)) if pair[0] != pair[1]), min(len(binary), len(c)))
            failures.append(dict(name=name, boundary=mismatch, lengths=[len(binary), len(c)],
                                 binary=binary[mismatch:mismatch+1], c=c[mismatch:mismatch+1]))
        if index % 100 == 0:
            print(f'Combats compares {index}/{len(cases)} ; ecarts {len(failures)}', flush=True)
    report = dict(source_sha256=EXPECTED_HASH, rng_cases=rng_cases,
                  encounters=len(cases), completed=completed, input_boundaries=boundaries,
                  escape_rejected_draws=rejected_draws, mismatches=len(failures), first_mismatches=failures[:5],
                  seconds=round(time.monotonic()-started, 2),
                  boundaries='BIOS keyboard injected; text/video/audio/delays bypassed; ORIGINAL RNG executed')
    (HERE/'build/audit/interactive_comparison.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
