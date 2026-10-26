"""Generation originale 0000/0010 puis creation du heros 0022 dans le C.

Les donnees exportees ne constituent pas encore une boucle d'exploration.
Les cartes, placements, mots et profils sont generes sans fixture de jeu.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time
from combat import HERE, RESOURCE
from creation import empty_roster, apply_snapshot, run_creation


def generate_world(seed, difficulty, resource=RESOURCE, max_draws=2000000):
    """Appeler le vrai C, puis decoder le transport decimal du monde."""
    raw=(resource/'NOMPERS').read_bytes()
    if len(raw)!=270: raise ValueError('NOMPERS doit contenir 270 octets')
    if not 0<=seed<=0xFFFFFFFF or not 0<=difficulty<=9:
        raise ValueError('Graine 32 bits et difficulte 0..9 attendues')
    game=empty_roster(seed,resource)
    packet=f'{seed&65535} {seed>>16} {difficulty} 1 {max_draws} '+' '.join(map(str,raw))+'\n'
    exe=HERE/'build/cmake'/('tera_world_bridge.exe' if sys.platform=='win32' else 'tera_world_bridge')
    result=subprocess.run([str(exe)],input=packet,text=True,capture_output=True,timeout=30)
    if result.returncode: raise ValueError(result.stderr.strip() or 'Generation interrompue')
    rows={line.split()[0]:list(map(int,line.split()[1:])) for line in result.stdout.splitlines()}
    w=rows['W'];f=rows['F']
    snapshot=[0,w[0],w[6],0,0,0,0,w[7],w[4],w[5],0,0,*f[:10],*rows['A']]
    apply_snapshot(game,snapshot)
    game.update(schema='tera-world-state-v1',scope='generated_world',difficulty=difficulty,
                maps=[rows['M'][7*i:7*i+7] for i in range(1100)],
                maps_column7=rows['M'][6::7],helpers=rows['K'][40:44],
                towns=w[47:67],locations=[0]+w[67:76],extra_locations=[0]+w[76:85],
                horizontal_mountains=w[13:22],vertical_mountains=w[22:47],
                hidden_positions=w[11:13],arioch_position=w[9],alfol_position=w[10],
                has_map=f[10:21],tasks_done=f[21:25],fast_moves=f[25],initial_flag=f[26],
                mental_extension=f[27],tasks_counter=f[28],
                generation_report=dict(random_calls=w[1],map_attempts=w[2],placement_collisions=w[3]),
                on_alfol=0,previous_position=0,location_changed=0,aggressor=0,recruit_ready=0)
    game['hints']=[bytes(rows['H'][40*i:40*i+40]).split(b'\0')[0].decode('cp437') for i in range(38)]
    game['secret_words']=[bytes(rows['K'][10*i:10*i+10]).split(b'\0')[0].decode('cp437') for i in range(4)]
    game['secret_word']=game['secret_words'][0]
    return game


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed',type=lambda v:int(v,0))
    parser.add_argument('--difficulty',type=int,choices=range(10),default=1)
    parser.add_argument('--resources',type=Path,default=RESOURCE)
    parser.add_argument('--output',type=Path,default=HERE/'build/new_game.json')
    parser.add_argument('--no-create',action='store_true',help='Audit : exporter avant creation du heros')
    args=parser.parse_args()
    instant=time.time_ns();clock=time.localtime(instant//1000000000)
    seed=args.seed if args.seed is not None else (clock.tm_hour<<24)|(clock.tm_min<<16)|(clock.tm_sec<<8)|int(instant//10000000%100)
    print('Generation des mondes et des personnages...')
    game=generate_world(seed,args.difficulty,args.resources)
    print('Generation terminee. Creation du heros avec la graine poursuivie.')
    if args.no_create:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(game,ensure_ascii=False,indent=2),encoding='utf-8')
        print('Etat avant heros :',args.output)
    else:
        game['party_available']=1 # TERAMAIN entre 0010 et 0022, aucun tirage.
        run_creation(game,False,args.output)
        print('Etat genere disponible ; exploration et rendu du jeu complet encore distincts.')


if __name__=='__main__':
    try: main()
    except (ValueError,OSError,EOFError,KeyboardInterrupt) as error:
        print(str(error) or 'Generation interrompue.',file=sys.stderr);raise SystemExit(1)
