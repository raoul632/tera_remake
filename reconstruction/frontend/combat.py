"""Frontend terminal du combat C ; aucune regle ou RNG du jeu en Python.

Charge les ressources locales originales sans les recopier dans le depot.
Un etat JSON ou un bloc tSavedata apporte personnages et cartes du jeu.
Le mode --demo est un banc manuel synthetique, pas une nouvelle partie TERA.
"""
import argparse
import copy
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parent
RESOURCE = ROOT / "data/compiled program"
VERB = {'T': 26, 'L': 27, 'J': 28, 'D': 29, 'A': 31, 'P': 32, 'C': 33, 'F': 6}


def records(path, size, count):
    """Lire exactement count chaines Pascal de taille fixe, en CP437."""
    data = path.read_bytes()
    if len(data) != size * count:
        raise ValueError(f"Taille inattendue : {path}")
    result = []
    for start in range(0, len(data), size):
        length = data[start]
        if length >= size:
            raise ValueError(f"Chaine Pascal invalide : {path}")
        result.append(data[start+1:start+1+length].decode('cp437'))
    return result


def demo_game(monster, seed):
    """Fixture volontairement artificielle pour manipuler le combat seul."""
    actors = []
    for actor in range(10):
        actors.append(dict(active=int(actor == 0), hp=30, magic=15,
                           hp_limit=30, magic_limit=20, condition='OK',
                           stats=[0]+[10]*9, skills=[0]+[3]*9,
                           equipment=[3, 6, 9, 12, 15, 18, 21, 0, 0, 0],
                           name=f'Banc {actor}'))
    return dict(monster_type=monster, aggressor=0, seed=seed, world_id=0,
                difficulty=1, on_alfol=0, position=5, previous_position=4,
                money=0, experience=0, location_changed=0, helpers=[3, 6, 9, 12],
                secret_word='TEST', maps_column7=[1]*1100, actors=actors)


def load_savedata(path, monster, seed, world, on_alfol, previous):
    """Importer le bloc tSavedata 27B2h, offsets BIN/PAS, lecture seule.

    WorldId/OnAlfol/9B72 et RandSeed sont hors du record : l'hote doit les
    fournir explicitement, sans les deviner a partir de la position.
    """
    data = path.read_bytes()
    if len(data) != 0x27B2:
        raise ValueError('Le bloc tSavedata doit contenir exactement 10162 octets')
    def block(address, size):
        return data[address-0x73B0:address-0x73B0+size]
    def integer(address):
        return struct.unpack('<h', block(address, 2))[0]
    def string(address, size):
        raw = block(address, size)
        if raw[0] >= size:
            raise ValueError('Chaine du bloc tSavedata invalide')
        return raw[1:raw[0]+1].decode('cp437')
    actors = []
    for i in range(10):
        actors.append(dict(active=block(0x9402+i, 1)[0], hp=integer(0x74DC+2*i),
                           magic=integer(0x74F0+2*i), hp_limit=integer(0x7504+2*i),
                           magic_limit=integer(0x7518+2*i), condition=string(0x952C+3*i, 3),
                           stats=list(block(0x7414+10*i, 10)),
                           skills=list(block(0x7478+10*i, 10)),
                           equipment=list(block(0x73B0+10*i, 10)), name=string(0x94C8+10*i, 10)))
    return dict(monster_type=monster, aggressor=0, seed=seed, world_id=world,
                difficulty=integer(0x75E8), on_alfol=int(on_alfol),
                position=integer(0x75D2), previous_position=previous,
                money=integer(0x75D4), experience=integer(0x75D6), location_changed=0,
                helpers=list(block(0x75EA, 4)), secret_word=string(0x9B3A, 10),
                maps_column7=list(block(0x75EE, 7700)[6::7]), actors=actors)


def pack_game(game, rows):
    """Encoder le protocole du pont C ; verifier longueurs et caracteres.

    Les bornes numeriques sont egalement controlees par le pont avant toute
    execution. Les tableaux de cartes/statistiques sont fournis, non generes.
    """
    if len(game['actors']) != 10 or len(game['maps_column7']) != 1100 or len(game['helpers']) != 4:
        raise ValueError('Il faut 10 personnages, 1100 cases et 4 helpers')
    if not 0 <= game['seed'] <= 0xFFFFFFFF:
        raise ValueError('Graine hors du mot 32 bits original')
    secret = game['secret_word'].encode('ascii')
    if len(secret) > 9 or b'\0' in secret:
        raise ValueError('Mot secret ASCII de neuf caracteres maximum')
    values = [game['monster_type'], game['aggressor'], game['seed'] & 65535,
              game['seed'] >> 16, game['world_id'], game['difficulty'], game['on_alfol'],
              game['position'], game['previous_position'], game['money'], game['experience'],
              game['location_changed'], *game['helpers'], *secret.ljust(10, b'\0')]
    if len(rows) != 22 or any(len(row) != 6 for row in rows):
        raise ValueError('VALMON doit avoir 22 lignes de 6 mots')
    values.extend(value for row in rows for value in row)
    for actor in game['actors']:
        state = actor['condition'].encode('ascii')
        if len(state) > 2 or b'\0' in state:
            raise ValueError('Etat ASCII de deux caracteres maximum')
        if any(len(actor[name]) != 10 for name in ('stats', 'skills', 'equipment')):
            raise ValueError('Chaque ligne stats/skills/equipment doit avoir 10 octets')
        values.extend([actor['active'], actor['hp'], actor['magic'], actor['hp_limit'],
                       actor['magic_limit'], *state.ljust(2, b'\0'), *actor['stats'],
                       *actor['skills'], *actor['equipment']])
    values.extend(game['maps_column7'])
    values.append(game.get('recruit_ready',0))
    values.append(game.get('sound',0))
    if any(type(value) is not int for value in values):
        raise ValueError('Le protocole exige des entiers')
    return ' '.join(map(str, values)) + '\n'


def apply_state(game, values):
    """Reintegrer exclusivement les sorties du C dans l'etat a exporter."""
    phase, actor, attempt, letter, seed, initial, hp, victory, fled, available, position, previous, money, xp, changed, rounds = values[:16]
    game.update(seed=seed, position=position, previous_position=previous, money=money,
                experience=xp, location_changed=changed)
    game['combat_result'] = dict(phase=phase, monster_hp=hp, initial_monster_hp=initial,
                                victory=victory, fled=fled, party_available=available, rounds=rounds)
    game['recruit_ready'] = values[-1]
    for i, item in enumerate(game['actors']):
        item['hp'], item['magic'], item['hp_limit'], item['magic_limit'], a, b = values[16+i*6:22+i*6]
        item['condition'] = bytes([a, b]).rstrip(b'\0').decode('ascii')
    return phase, actor, attempt, chr(letter)


def read_key():
    """Remplacer le clavier BIOS par une touche native, sans entree de ligne.

    En pipe, chaque octet reste une touche ; LF est adapte a Entree DOS.
    """
    if sys.stdin.isatty():
        if sys.platform == 'win32':
            import msvcrt
            key = msvcrt.getwch()
            if key in ('\0', '\xe0'):
                msvcrt.getwch()
                return 0  # Premiere touche etendue du clavier DOS.
        else:
            import termios
            import tty
            old = termios.tcgetattr(sys.stdin.fileno())
            try:
                tty.setraw(sys.stdin.fileno())
                key = sys.stdin.read(1)
            finally:
                termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, old)
    else:
        key = sys.stdin.read(1)
    if not key:
        raise EOFError('Entree interrompue : combat non termine')
    if key == '\x03':
        raise KeyboardInterrupt
    return 13 if key == '\n' else 8 if key == '\x7f' else ord(key) if ord(key) < 256 else 0


def run_terminal(game, resource, output):
    """Piloter la machine C jusqu'a DONE et exporter l'etat final complet."""
    raw = (resource / 'VALMON').read_bytes()
    if len(raw) != 264:
        raise ValueError('VALMON original attendu : 264 octets')
    rows = list(struct.iter_unpack('<6h', raw))
    verbs = records(resource / 'VERBES', 19, 56)
    names = records(resource / 'NOMON', 40, 22)
    objects = records(resource / 'OBJETS', 16, 25)
    phrases = [records(resource / file, 40, count) for file, count in
               [('MESSAGE', 102), ('MESSAGE2', 33), ('MESSAGE3', 33)]]
    bridge = HERE / 'build/cmake' / ('tera_combat_bridge.exe' if sys.platform == 'win32' else 'tera_combat_bridge')
    if not bridge.exists():
        raise ValueError('Compiler reconstruction avec CMake avant de lancer le combat')
    transport = pack_game(game, rows)
    print(f"Rencontre : {names[game['monster_type']-1]}")
    process = subprocess.Popen([str(bridge)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True, bufsize=1)
    try:
        previous_phase = None
        process.stdin.write(transport); process.stdin.flush()
        for line in process.stdout:
            tag, *values = line.split()
            values = list(map(int, values))
            if tag == 'E':
                if previous_phase == 2:
                    print()
                    previous_phase = None
                kind, actor, letter, value = values
                name = game['actors'][actor].get('name', f'Personnage {actor}')
                if kind == 0:
                    print(('danger !', ' aargh ! ', 'aie.. ')[value] if value >= 0 else f'Agression de {name} !')
                elif kind == 1:
                    print('\n--- Tour ---')
                elif kind == 2:
                    if value < 0:
                        print(f"{name} n'a pas d'arme de tir")
                    else:
                        print(f"{name} {phrases[value][VERB[chr(letter)]-1]}")
                elif kind in (3, 4):
                    print(f"{'Monstre' if kind == 3 else name} : {value} degat(s)")
                elif kind == 5:
                    print(f'Fuite vers la case {value}')
                elif kind == 6:
                    print('Annulation.' + (' Tour consomme.' if value else ''))
                elif kind == 7:
                    print('Passe son tour.' if value else 'Touche non reconnue.')
                elif kind == 9:
                    print(f'({objects[value-1]})')
            elif tag == 'S':
                phase, actor, attempt, letter = apply_state(game, values)
                if phase == 3:
                    print('Victoire.' if game['combat_result']['victory'] else
                          'Combat quitte.' if game['combat_result']['fled'] else 'Groupe hors de combat.')
                    break
                if phase == 4:
                    raise ValueError('Carte incoherente : aucune destination de fuite admissible')
                if phase == 0:
                    item = game['actors'][actor]
                    print(f"{item.get('name', actor)} : PV {item['hp']}, PM {item['magic']}, {item['condition']}")
                    print(' / '.join(f'{key} {verbs[number-1]}' for key, number in VERB.items()))
                elif phase == 1:
                    print(f"{verbs[VERB[letter]-1]} ? O / Entree pour confirmer")
                elif phase == 2 and previous_phase != 2:
                    print('mot -> ', end='', flush=True)
                key = read_key()
                previous_phase = phase
                if phase == 2:
                    if 65 <= key <= 90 or 97 <= key <= 122:
                        print(chr(key).upper(), end='', flush=True)
                    elif key == 8:
                        print('\b \b', end='', flush=True)
                    elif key == 13:
                        print()
                        previous_phase = None
                process.stdin.write(f'{key}\n'); process.stdin.flush()
        process.stdin.close()
        code = process.wait(timeout=5)
        if code:
            raise ValueError(process.stderr.read().strip() or f'Pont C termine avec code {code}')
        if game.get('combat_result', {}).get('phase') != 3:
            raise ValueError('Le moteur est sorti sans terminer le combat')
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(game, ensure_ascii=False, indent=2), encoding='utf-8')
        print(f'Etat final : {output}')
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=5)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--state', type=Path)
    source.add_argument('--savedata', type=Path)
    source.add_argument('--demo', action='store_true')
    parser.add_argument('--characters', type=Path, help='Roster termine par creation.py, reintegre avant le combat')
    parser.add_argument('--monster', type=int, choices=range(1, 23))
    parser.add_argument('--seed', type=lambda value: int(value, 0))
    parser.add_argument('--world', type=int)
    parser.add_argument('--on-alfol', type=int, choices=(0, 1))
    parser.add_argument('--previous-position', type=int)
    parser.add_argument('--resources', type=Path, default=RESOURCE)
    parser.add_argument('--output', type=Path, default=HERE / 'build/combat_result.json')
    args = parser.parse_args()
    instant = time.time_ns()
    now = time.localtime(instant // 1000000000)
    seed = args.seed if args.seed is not None else (now.tm_hour << 24) | (now.tm_min << 16) | (now.tm_sec << 8) | int(instant // 10000000 % 100)
    if args.state:
        game = json.loads(args.state.read_text(encoding='utf-8'))
        if args.seed is not None:
            game['seed'] = seed
        if args.monster is not None:
            game['monster_type'] = args.monster
    elif args.savedata:
        if None in (args.world, args.on_alfol, args.previous_position):
            parser.error('--savedata exige --world, --on-alfol et --previous-position (hors record)')
        game = load_savedata(args.savedata, args.monster or 1, seed, args.world, args.on_alfol, args.previous_position)
    else:
        print('BANC DE COMBAT : contexte de test, pas une nouvelle partie TERA.' if args.characters else
              'BANC SYNTHETIQUE : personnages et cartes de test, pas une nouvelle partie TERA.')
        game = demo_game(args.monster or 1, seed)
    if args.characters:
        roster = json.loads(args.characters.read_text(encoding='utf-8'))
        if roster.get('schema') != 'tera-character-roster-v1' or roster.get('creation_result', {}).get('phase') != 5:
            raise ValueError('Roster de creation termine attendu')
        game['actors'] = copy.deepcopy(roster['actors'])
        for field in ('seed', 'money', 'experience', 'world_id', 'position', 'days', 'level',
                      'dummy_level', 'energy', 'keys', 'arrival_pending'):
            game[field] = copy.deepcopy(roster[field])
        if args.seed is not None:
            game['seed'] = seed
    if 'monster_type' not in game:
        raise ValueError('Cet etat exige --monster 1..22 pour le banc de combat.')
    run_terminal(game, args.resources, args.output)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, EOFError, KeyboardInterrupt) as error:
        print(str(error) or 'Combat interrompu.', file=sys.stderr)
        raise SystemExit(1)
