"""Creation interactive des personnages dans le moteur C.

Ce frontend ne genere pas encore le monde : il exporte un roster de
personnages pour la future boucle generale ou un banc de combat explicite.
Les calculs et tous les tirages de jeu appartiennent au C.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time

from combat import HERE, RESOURCE, records, read_key

STAT_NAMES = ['-', 'Force', 'Intelligence', 'Chance', 'Endurance', 'Volonte',
              'Vitalite', 'Agilite', 'Charme', 'Habilete']


def empty_roster(seed, resource=RESOURCE):
    """Champs pre-creation : donnees NOMPERS et constantes OVL_0000.

    Les tableaux effaces sont vraiment vierges ; aucune caracteristique,
    competence, arme, carte, aide ou mot secret de test n'est ajoute.
    La graine est celle d'ENTREE du bloc personnages, pas du debut de
    generation mondiale : le calendrier complet sera branche ulterieurement.
    """
    groups = records(resource / 'NOMPERS', 10, 27)
    actors = [dict(active=0, hp=0, magic=0, hp_limit=0, magic_limit=0,
                   condition='', name='' if id == 0 else groups[id-1],
                   stats=[0]*10, skills=[0]*10, equipment=[0]*10) for id in range(10)]
    variants = [[''] + groups[9:18], [''] + groups[18:27]]
    return dict(schema='tera-character-roster-v1', scope='characters_only', seed=seed,
                money=500, experience=0, days=0, level=0, dummy_level=0, energy=50,
                position=0, world_id=0, arrival_pending=0, party_available=0,
                keys=[10]*10, actors=actors, name_variants=variants)


def name_bytes(name):
    """Transporter CP437 exactement, sans convertir les noms du jeu en ASCII."""
    raw = name.encode('cp437')
    if len(raw) > 9 or b'\0' in raw:
        raise ValueError('Nom original de neuf octets CP437 maximum')
    return list(raw.ljust(10, b'\0'))


def pack_roster(game, initialize):
    """Protocole prive creation_bridge.c, valide avant le premier effet C."""
    if len(game['actors']) != 10 or len(game['keys']) != 10:
        raise ValueError('Le roster exige dix personnages et dix cles')
    if not 0 <= game['seed'] <= 0xFFFFFFFF:
        raise ValueError('Graine 32 bits invalide')
    if len(game['name_variants']) != 2 or any(len(g) != 10 for g in game['name_variants']):
        raise ValueError('Deux groupes de dix variantes de noms attendus')
    values = [int(initialize), game['seed'] & 65535, game['seed'] >> 16,
              *[game[k] for k in ('money', 'experience', 'days', 'level', 'dummy_level',
                                 'energy', 'position', 'world_id', 'arrival_pending', 'party_available')],
              *game['keys']]
    for actor in game['actors']:
        condition = actor['condition'].encode('cp437')
        if len(condition) > 2 or b'\0' in condition:
            raise ValueError('Etat original de deux octets maximum')
        if any(len(actor[k]) != 10 for k in ('stats', 'skills', 'equipment')):
            raise ValueError('Les lignes personnage comportent dix octets')
        values.extend([actor['active'], actor['hp'], actor['magic'], actor['hp_limit'],
                       actor['magic_limit'], *condition.ljust(2, b'\0'), *name_bytes(actor['name']),
                       *actor['stats'], *actor['skills'], *actor['equipment']])
    for group in game['name_variants']:
        for name in group:
            values.extend(name_bytes(name))
    if any(type(value) is not int for value in values):
        raise ValueError('Le transport exige des entiers')
    return ' '.join(map(str, values)) + '\n'


def apply_snapshot(game, values):
    """Lire les seules sorties du C, sans completer ou reequilibrer le heros."""
    phase = values[0]
    for key, value in zip(('seed', 'money', 'experience', 'days', 'level', 'dummy_level',
                           'energy', 'position', 'world_id', 'arrival_pending', 'party_available'), values[1:12]):
        game[key] = value
    game['keys'] = values[12:22]
    for id, actor in enumerate(game['actors']):
        row = values[22+47*id:69+47*id]
        actor.update(active=row[0], hp=row[1], magic=row[2], hp_limit=row[3], magic_limit=row[4],
                     condition=bytes(row[5:7]).rstrip(b'\0').decode('cp437'),
                     name=bytes(row[7:17]).split(b'\0')[0].decode('cp437'),
                     stats=row[17:27], skills=row[27:37], equipment=row[37:47])
    game['creation_result'] = dict(phase=phase)
    return phase


def show_stats(actor):
    print(' | '.join(f'{STAT_NAMES[i]} {actor["stats"][i]}' for i in range(1, 10)))


def run_creation(game, initialize, output):
    """Piloter la creation C jusqu'a validation puis exporter le roster."""
    packet = pack_roster(game, initialize)
    exe = HERE/'build/cmake'/('tera_creation_bridge.exe' if sys.platform == 'win32' else 'tera_creation_bridge')
    process = subprocess.Popen([str(exe)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True, bufsize=1)
    phase, previous_phase = None, None
    try:
        process.stdin.write(packet); process.stdin.flush()
        for line in process.stdout:
            tag, *data = line.split()
            values = list(map(int, data))
            if tag == 'S':
                phase = apply_snapshot(game, values)
            elif tag == 'P':
                split, left, name_length, attempts = values
                hero = game['actors'][0]
                if phase == 5:
                    print('\nPersonnage valide :', hero['name'])
                    show_stats(hero)
                    print(f"PV {hero['hp']} / PM {hero['magic']} ; competences {hero['skills'][1:]}")
                    print('Equipement :', hero['equipment'])
                    break
                if phase == 0:
                    if previous_phase != 0:
                        print(f'\nCreation numero {attempts}. Inscris son nom : ', end='', flush=True)
                elif phase == 1:
                    print('\nH(omme) ou F(emme) ? ', end='', flush=True)
                elif phase == 2:
                    if previous_phase != 2:
                        print(); show_stats(hero)
                    target = ('Endurance', 'Volonte', 'Vitalite')[split-1]
                    print(f'{hero["stats"][split]} points a repartir : {target} (invite originale 1-{hero["stats"][split]-1}) : ', end='', flush=True)
                elif phase == 3:
                    print(); show_stats(hero)
                    print(f'Bonus restants {left}. F/I/N/E/V/T/A/C/H : ', end='', flush=True)
                elif phase == 4:
                    print(); show_stats(hero)
                    print('Gardes-tu ce personnage ? N recommence, toute autre touche accepte : ', end='', flush=True)
                key = read_key()
                if phase == 0:
                    if 65 <= key <= 90 or 97 <= key <= 122:
                        print(chr(key).upper(), end='', flush=True)
                    elif key == 8 and name_length:
                        print('\b \b', end='', flush=True)
                elif key >= 32:
                    print(chr(key))
                previous_phase = phase
                process.stdin.write(f'{key}\n'); process.stdin.flush()
        process.stdin.close()
        code = process.wait(timeout=5)
        if code or phase != 5:
            raise ValueError(process.stderr.read().strip() or 'Creation interrompue ou invalide')
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(game, ensure_ascii=False, indent=2), encoding='utf-8')
        print(f'Roster exporte : {output}')
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=5)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--roster', type=Path, help='Recreer le heros en conservant un roster existant')
    parser.add_argument('--seed', type=lambda value: int(value, 0))
    parser.add_argument('--resources', type=Path, default=RESOURCE)
    parser.add_argument('--output', type=Path, default=HERE/'build/characters.json')
    args = parser.parse_args()
    if args.roster:
        game = json.loads(args.roster.read_text(encoding='utf-8'))
        if game.get('schema') != 'tera-character-roster-v1':
            raise ValueError('Fichier roster de creation attendu')
        if args.seed is not None:
            game['seed'] = args.seed
    else:
        instant = time.time_ns()
        clock = time.localtime(instant//1000000000)
        seed = args.seed if args.seed is not None else (clock.tm_hour<<24) | (clock.tm_min<<16) | (clock.tm_sec<<8) | int(instant//10000000 % 100)
        game = empty_roster(seed, args.resources)
    print('Creation des personnages selon les regles originales ; generation du monde encore distincte.')
    run_creation(game, not args.roster, args.output)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, EOFError, KeyboardInterrupt) as error:
        print(str(error) or 'Creation interrompue.', file=sys.stderr)
        raise SystemExit(1)
