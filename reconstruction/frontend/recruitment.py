"""Banc terminal des commandes originales solliciter / seduire.

Le lieu vient de l'etat fourni. --at est uniquement une position d'audit,
jamais une nouvelle commande de deplacement du jeu.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
from combat import HERE, RESOURCE, read_key, records
from creation import pack_roster, apply_snapshot


def execute_request(game,request,key):
    """Transmettre la commande au C ; l'hote ne predit pas la negociation."""
    ready=game.get('recruit_ready',0)
    packet=f'{request} {ready} {key}\n'+pack_roster(game,False)+' '.join(map(str,game['locations'][1:]))+'\n'
    exe=HERE/'build/cmake'/('tera_recruitment_bridge.exe' if sys.platform=='win32' else 'tera_recruitment_bridge')
    result=subprocess.run([str(exe)],input=packet,text=True,capture_output=True,timeout=10)
    if result.returncode: raise ValueError(result.stderr.strip() or 'Etat de recrutement invalide')
    rows={line.split()[0]:list(map(int,line.split()[1:])) for line in result.stdout.splitlines()}
    apply_snapshot(game,rows['S'])
    report=rows['R']
    game['recruit_ready']=report[1]
    game['recruitment_result']=dict(outcome=report[0],ready=report[1],negotiator=report[2],
                                   rolls=report[3],initial_party_count=report[4],
                                   available_mask=report[5],joined_mask=report[6])
    return game['recruitment_result']


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--state',type=Path,required=True)
    parser.add_argument('--at',type=int,help='Position imposee pour ce banc uniquement')
    parser.add_argument('--request',choices=('solliciter','seduire','both'),default='both')
    parser.add_argument('--resources',type=Path,default=RESOURCE)
    parser.add_argument('--output',type=Path,default=HERE/'build/recruited_game.json')
    args=parser.parse_args()
    game=json.loads(args.state.read_text(encoding='utf-8'))
    if game.get('schema')!='tera-world-state-v1' or game.get('creation_result',{}).get('phase')!=5:
        raise ValueError('Etat genere avec heros termine attendu')
    if args.at is not None:
        print('BANC D\'AUDIT : position imposee, sans simulation de deplacement.')
        game['position']=args.at
    verbs=records(args.resources/'VERBES',19,56)
    requests=(0,1) if args.request=='both' else (0,) if args.request=='solliciter' else (1,)
    labels={1:'Annule.',2:'Un compagnon peut etre sollicite ici.',3:'Personne ne repond.',
            4:'Groupe complet.',5:'Recrutement par negociation.',6:'Recrutement contre 250.',
            7:'Argent insuffisant.',8:'Aucune sollicitation en attente.'}
    for request in requests:
        print(verbs[(25 if request==0 else 14)-1]+' ? O ou Entree confirme : ',end='',flush=True)
        key=read_key();print()
        report=execute_request(game,request,key)
        print(labels[report['outcome']])
        if request==0 and not report['ready']: break
        for id in range(1,10):
            if report['joined_mask']&(1<<id):print(game['actors'][id]['name']+' rejoint le groupe.')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(game,ensure_ascii=False,indent=2),encoding='utf-8')
    print('Etat exporte :',args.output)


if __name__=='__main__':
    try: main()
    except (ValueError,OSError,EOFError,KeyboardInterrupt) as error:
        print(str(error) or 'Interaction interrompue.',file=sys.stderr);raise SystemExit(1)
