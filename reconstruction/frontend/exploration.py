"""Exploration terminal d'Amarande, Cite, Seth, Alfol et Meduz.

La sortie JSON est ecrite seulement apres une operation complete.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys
from combat import HERE, RESOURCE, read_key, records
from creation import pack_roster, apply_snapshot


def pack_exploration(game, mode, key, resource=RESOURCE):
    """Transport decimal complet ; les regles et tous les tirages restent en C."""
    values=[game.get('sound',1),game.get('moved',0),game.get('town_found',0),game.get('town_id',0),
            game.get('force_day',0),game.get('encounter_suppressed',0),game.get('recruit_ready',0),
            game.get('on_alfol',0),game.get('location_changed',0),game.get('previous_position',0),
            game['difficulty'],game['alfol_position']]
    for name in ('horizontal_mountains','vertical_mountains','towns'):
        values+=game[name]
    values+=game['locations'][1:]+game['extra_locations'][1:]
    values += [v for row in game['maps'] for v in row]
    raw=(resource/'VALMON').read_bytes()
    if len(raw)!=264: raise ValueError('VALMON doit contenir 264 octets')
    values+=list(struct.unpack('<132h',raw))+game['helpers']
    values+=list(game['secret_word'].encode('cp437').ljust(10,b'\0'))
    values.append(game.get('fast_moves',0))
    values.append(game.get('information_context',0))
    return f'{mode} {key}\n'+pack_roster(game,False)+' '.join(map(str,values))+'\n'


def apply_exploration(game, data):
    """Reintegrer uniquement les champs transportes, en preservant le monde."""
    fields=('day_phase','day_stage','encounters','day_advanced','level_advanced','moved',
            'town_found','town_id','force_day','encounter_suppressed','recruit_ready',
            'on_alfol','location_changed','monster_type','combat_phase','combat_actor',
            'combat_attempt','fled','victory','previous_position')
    game.update(zip(fields,data))


def execute_operation(game,mode,key,resource=RESOURCE):
    """Piloter un pas/jour et ses combats jusqu'au retour effectif du C."""
    exe=HERE/'build/cmake'/('tera_exploration_bridge.exe' if sys.platform=='win32' else 'tera_exploration_bridge')
    names=records(resource/'NOMON',40,22)
    process=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,text=True,bufsize=1)
    try:
        process.stdin.write(pack_exploration(game,mode,key,resource));process.stdin.flush()
        announced=0
        for line in process.stdout:
            tag,*values=line.split();values=list(map(int,values))
            if tag=='S': apply_snapshot(game,values)
            elif tag=='J':game['information_context']=values[0]
            elif tag=='Y':game['npc_variant']=values[0]
            elif tag=='M': game['move_result']=dict(zip(('moved','near_mountain','scene','random_calls'),values))
            elif tag=='D':
                apply_exploration(game,values)
                if game['day_phase']==1:
                    if game['encounters']!=announced:
                        print('Rencontre :',names[game['monster_type']-1]);announced=game['encounters']
                    phase=game['combat_phase'];actor=game['combat_actor']
                    if phase==0: print(game['actors'][actor]['name']+' : T L J A D P C F ? ',end='',flush=True)
                    elif phase==1: print('O/Entree confirme, autre touche annule : ',end='',flush=True)
                    else: print('Mot : ',end='',flush=True)
                    process.stdin.write(str(read_key())+'\n');process.stdin.flush()
            elif tag=='E' and values[0] in (3,4):
                print(f"{game['actors'][values[1]]['name']} : {values[3]} degats")
        process.stdin.close()
        code=process.wait(timeout=5)
        if code!=0: raise ValueError(process.stderr.read().strip() or f'Operation C interrompue ({code})')
    finally:
        if process.poll() is None: process.kill();process.wait()
    return game


def complete_arrival(game,resource):
    """TERAMAIN : repeter l'arrivee apres chaque fuite avant une touche de marche."""
    while game.get('party_available',1) and (game.get('arrival_pending',0) or game.get('fled',0)
          or game.get('combat_result',{}).get('fled',0)):
        game['position']-=1;game['arrival_pending']=0;game['fled']=0
        if 'combat_result' in game:game['combat_result']['fled']=0
        execute_operation(game,2,ord('8'),resource)
        from commerce import present_place
        present_place(game,resource)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--state',type=Path,required=True)
    parser.add_argument('--output',type=Path,default=HERE/'build/exploration_result.json')
    parser.add_argument('--resources',type=Path,default=RESOURCE)
    args=parser.parse_args()
    game=json.loads(args.state.read_text(encoding='utf-8'))
    if game.get('schema')!='tera-world-state-v1' or game.get('creation_result',{}).get('phase')!=5:
        raise ValueError('Monde genere et heros termine attendus')
    if game['world_id'] not in range(12) or (game['world_id']==9 and not game.get('on_alfol',0)):
        raise ValueError('Monde non porte ou contexte Alfol incoherent')
    from navigation import execute_navigation
    from inventory import execute_inventory
    from dialogue import execute_dialogue
    from lifecycle import execute_lifecycle
    from commerce import execute_commerce,present_place
    from space import execute_space
    from alfol import execute_alfol
    from meduz import execute_meduz
    from ending import execute_ending
    from amarande import execute_amarande,TOWNS
    if game['world_id']==11 and game.get('party_available',1) and not game.get('on_alfol',0) and not game.get('program_halted',0) and not all(game['tasks_done']):
        complete_arrival(game,args.resources)
    print('Amarande : 8 nord, 2 sud, 6 est, 4 ouest ; E entrer a Kronopolis, au spatioport, dans la Cite ou chez Seth.')
    print('Cite : 8 avancer, 2 reculer, 6 droite, 4 gauche ; D descendre, M monter, L lire, I invoquer.')
    print('Cite : O ouvrir, P prendre ; A abandonner, B boire, N negocier (I/A/V/D/S) ; S seduire ; Q exporter/quitter.')
    print('Spatioport : E embarquer. Alfol : 2468 marcher, E entrer dans les batiments, R rembarquer au point zero.')
    print('Pyramide : M pour voyager. Meduz : 2468 marcher, M mediter au catalyseur, I invoquer au sanctuaire.')
    while True:
        if game.get('program_halted',0):break
        if game.get('ending_complete',0):break
        if all(game['tasks_done']) and game['world_id']==11 and not game.get('on_alfol',0):
            execute_ending(game,args.resources);break
        if not game.get('party_available',1) and not game.get('temple_entry_pending',0) and not game.get('alfol_entry_pending',0) and not game.get('meduz_entry_pending',0):
            if game['world_id']==10:execute_meduz(game,resume=True,resource=args.resources)
            if game['world_id']==9:execute_alfol(game,resource=args.resources,leave=True)
            elif game['world_id']!=11:
                execute_navigation(game,0,resource=args.resources,leave=True)
                present_place(game,args.resources,force=True)
            if all(game['tasks_done']):
                execute_ending(game,args.resources);break
            execute_lifecycle(game,args.resources)
            if game['lifecycle_phase']==3:break
            complete_arrival(game,args.resources)
            continue
        place='Meduz' if game['world_id']==10 else 'Alfol (batiments)' if game['world_id']==9 else 'Alfol' if game.get('on_alfol',0) else 'Amarande' if game['world_id']==11 else 'Cite/temple'
        print(f"{place} | position {game['position']} | jours {game['days']} | argent {game['money']}")
        pending_temple=game.pop('temple_entry_pending',0)
        pending_alfol=game.pop('alfol_entry_pending',0)
        pending_meduz=game.pop('meduz_entry_pending',0)
        key=read_key()
        if key in (ord('q'),ord('Q')):
            if pending_temple:game['temple_entry_pending']=pending_temple
            if pending_alfol:game['alfol_entry_pending']=pending_alfol
            if pending_meduz:game['meduz_entry_pending']=pending_meduz
            break
        if game['world_id']==10 and key in map(ord,'2468ERIDMLOPeridmlop'):
            execute_meduz(game,key-32 if 97<=key<=122 else key,resource=args.resources)
        elif game.get('on_alfol',0) and key in map(ord,'2468ERIDMLOPeridmlop'):
            execute_alfol(game,key-32 if 97<=key<=122 else key,resource=args.resources)
            if game['alfol_phase']==7 and game['party_available']:
                execute_space(game,returning=True,resource=args.resources)
                if game['space_phase']==4:present_place(game,args.resources,force=True)
        elif game['world_id']!=11 and key in map(ord,'2468DMLIOPdmliop'):
            if 97<=key<=122:key-=32
            execute_navigation(game,key,resource=args.resources)
            if not game['grid_moved'] and key in map(ord,'2468'):print('Passage bloque.')
            if (game['position']==0 and not game['city_overlay']) or not game['party_available']:
                if game['world_id']!=11:execute_navigation(game,0,resource=args.resources,leave=True)
            if game['world_id']==11:present_place(game,args.resources,force=True)
        elif key in map(ord,'NSns'):
            execute_dialogue(game,key-32 if 97<=key<=122 else key,args.resources)
        elif key in map(ord,'ABab'):
            execute_inventory(game,key-32 if 97<=key<=122 else key,args.resources)
        elif game['world_id']==11 and key in map(ord,'2468'):
            execute_operation(game,2,key,args.resources)
            if not game['moved']:print('Passage bloque par une montagne.')
            elif game['town_found']:
                town=game['town_id']
                print('Pyramide : M pour voyager.' if town==3 else TOWNS[town]+': E pour entrer.' if town in (0,1,2,4) else TOWNS[town]+': N negocier, S seduire, C combattre.' if town<10 else 'Panneau : L pour lire la direction de Kronopolis.')
            if game['level_advanced']:print('Nouveau niveau :',game['level'])
            present_place(game,args.resources)
            complete_arrival(game,args.resources)
        elif game['world_id']==11 and game.get('town_found',0) and game.get('town_id')==3 and key in map(ord,'Mm'):
            execute_meduz(game,ord('M'),enter=True,resource=args.resources)
        elif game['world_id']==11 and key in (ord('e'),ord('E')):
            if not game.get('town_found',0) or game['town_id'] not in (0,1,2,4):execute_amarande(game,ord('E'),args.resources)
            elif game['town_id']==0:execute_commerce(game,args.resources)
            elif game['town_id']==1:
                execute_space(game,resource=args.resources)
                if game['space_departed'] and game['space_phase']==4:execute_alfol(game,initial=True,resource=args.resources)
            else:
                execute_navigation(game,ord('E'),enter=True,resource=args.resources)
                if not game['party_available'] and not game.get('temple_entry_pending',0):
                    if game['world_id']!=11:execute_navigation(game,0,resource=args.resources,leave=True)
                if game['world_id']==11 and game.get('grid_moved'):
                    present_place(game,args.resources,force=True)
        elif game['world_id']==11 and not game.get('on_alfol',0) and key in map(ord,'CHI#DFLMOPUchidflmopu'):
            execute_amarande(game,key-32 if 97<=key<=122 else key,args.resources)
            if game.get('quit_requested',0):break
            complete_arrival(game,args.resources)
        elif key in (ord('?'),ord('+')):
            print('A abandonner ; B boire ; C combattre ; H halte ; I invoquer ; N negocier ; S seduire ; # son ; L lire un panneau.')
        else: print('Commande non portee.')
        if pending_temple and game['world_id']==8 and not game['party_available']:
            execute_navigation(game,0,resource=args.resources,leave=True)
            present_place(game,args.resources,force=True)
        if pending_alfol and game['world_id']==9 and not game['party_available']:
            execute_alfol(game,resource=args.resources,leave=True)
        if pending_meduz and game['world_id']==10 and not game['party_available']:
            execute_meduz(game,resume=True,resource=args.resources)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(game,ensure_ascii=False,indent=2),encoding='utf-8')
    print('Etat exporte :',args.output)


if __name__=='__main__':
    try:main()
    except (ValueError,OSError,EOFError,KeyboardInterrupt) as error:
        print(str(error) or 'Exploration interrompue.',file=sys.stderr);raise SystemExit(1)
