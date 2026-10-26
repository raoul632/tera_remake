"""Surface et batiments d'Alfol ; presentation des frontieres du moteur C."""
import subprocess
import sys
from combat import HERE,RESOURCE,read_key,records
from creation import apply_snapshot
from exploration import pack_exploration


def pack_alfol(game,key,initial=False,resource=RESOURCE,leave=False):
    values=[game.get('alfol_status_actor',9),game['tasks_counter'],*game['tasks_done']]
    hint=game.get('hints',[''])[0].encode('cp437')
    if len(hint)>39:raise ValueError('Indice Pascal limite a 39 octets')
    values+=game.get('alfol_hint_bytes',list(hint.ljust(40,b'\0')))
    return pack_exploration(game,2 if leave else int(initial),key,resource)+' '.join(map(str,values))+'\n'


def execute_alfol(game,key=0,initial=False,resource=RESOURCE,leave=False):
    """Executer une commande et ses dialogues/jours/combats jusqu'au retour.

    Un EOF au milieu d'une lecture echoue : le terminal n'exporte alors pas
    d'etat partiel. Les deltas de carte viennent exclusivement du pont C.
    """
    exe=HERE/'build/cmake'/('tera_alfol_bridge.exe' if sys.platform=='win32' else 'tera_alfol_bridge')
    names=records(resource/'NOMON',40,22)
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    task_before=game['tasks_done'][1]
    try:
        p.stdin.write(pack_alfol(game,key,initial,resource,leave));p.stdin.flush()
        for line in p.stdout:
            tag,*v=line.split();v=list(map(int,v))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='Y':
                fields=('alfol_phase','grid_moved','on_alfol','alfol_status_actor','information_context',
                    'encounter_suppressed','recruit_ready','location_changed','previous_position','tasks_counter')
                game.update(zip(fields,v[:10]));game['tasks_done'][1]=v[10]
                game.update(zip(('combat_phase','combat_actor','monster_type','alfol_entry_pending'),v[11:]))
            elif tag=='B':
                game['maps'][v[0]]=v[1:];game['maps_column7'][v[0]]=v[7]
            elif tag=='H':
                game['alfol_hint_bytes']=v # Garder aussi les octets apres le terminateur.
                game['hints'][0]=bytes(v).split(b'\0')[0].decode('cp437')
            elif tag=='E' and v[0] in (3,4):print(f"{game['actors'][v[1]]['name']} : {v[3]} degats")
            elif tag=='Z':
                phase=game['alfol_phase']
                if phase>=6:continue
                prompts={0:'Entrer dans les batiments ? O/Entree : ',1:'Rembarquer ? O/Entree : ',
                    2:'Bureau des medias : payer 1000 ? O : ',3:'Porte : O ouvrir, U utiliser la cle : ',5:'Invoquer qui ? '}
                if phase==4:
                    combat=game['combat_phase']
                    prompt=(names[game['monster_type']-1]+f" ; {game['actors'][game['combat_actor']]['name']} : T L J A D P C F ? "
                        if combat==0 else 'O/Entree confirme : ' if combat==1 else 'Mot : ')
                else:prompt=prompts[phase]
                print(prompt,end='',flush=True);p.stdin.write(f'{read_key()}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Alfol interrompu ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
    if not task_before and game['tasks_done'][1]:print(f"Objectif des medias accompli ; {game['tasks_counter']} taches sur 4.")
    return game
