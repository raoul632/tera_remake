"""Pyramide et Meduz : presentation des lectures et du combat natifs."""
import subprocess
import sys
from combat import HERE,RESOURCE,read_key,records
from creation import apply_snapshot
from exploration import pack_exploration


def pack_meduz(game,key=0,enter=False,initial=False,resume=False,travel=True,resource=RESOURCE):
    values=[game.get('meduz_backup_position',game['position']),int(travel),int(enter),
        game.get('mental_extension',0),game['tasks_counter'],*game['tasks_done']]
    for word in game['secret_words']:
        raw=word.encode('cp437')
        if len(raw)>9:raise ValueError('Mot secret limite a neuf lettres')
        values+=list(raw.ljust(10,b'\0'))
    return pack_exploration(game,2 if resume else int(initial),key,resource)+' '.join(map(str,values))+'\n'


def execute_meduz(game,key=0,enter=False,resume=False,resource=RESOURCE):
    """Une operation complete ; interruption sans export de l'etat partiel."""
    exe=HERE/'build/cmake'/('tera_meduz_bridge.exe' if sys.platform=='win32' else 'tera_meduz_bridge')
    names=records(resource/'NOMON',40,22);before=game['tasks_done'][0];mental=game.get('mental_extension',0)
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    try:
        p.stdin.write(pack_meduz(game,key,enter,resume=resume,resource=resource));p.stdin.flush()
        for line in p.stdout:
            tag,*raw=line.split();v=list(map(int,raw))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='V':
                fields=('meduz_phase','grid_moved','meduz_backup_position','mental_extension','tasks_counter')
                game.update(zip(fields,v[:5]));game['tasks_done'][0]=v[5]
                game.update(zip(('meduz_entry_pending','information_context','encounter_suppressed','recruit_ready',
                    'location_changed','previous_position','combat_phase','combat_actor','monster_type','meduz_travelling'),v[6:]))
            elif tag=='W':game['meduz_word']=bytes(v[1:]).split(b'\0')[0].decode('cp437')
            elif tag=='E' and v[0] in (3,4):print(f"{game['actors'][v[1]]['name']} : {v[3]} degats")
            elif tag=='Z':
                phase=game['meduz_phase']
                if phase>=6:continue
                if phase==5:
                    combat=game['combat_phase']
                    prompt=(names[game['monster_type']-1]+f" ; {game['actors'][game['combat_actor']]['name']} : T L J A D P C F ? "
                        if combat==0 else 'O/Entree confirme : ' if combat==1 else 'Mot : ')
                elif phase in (1,4):prompt=('Mot de voyage : ' if phase==1 else 'Invoquer qui ? ')+game['meduz_word']+' '
                else:prompt={0:'Partir pour Meduz ? O/Entree : ',2:'Mediter ? O/Entree : ',3:'Invoquer ? O/Entree : '}[phase]
                print(prompt,end='',flush=True);p.stdin.write(f'{read_key()}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Meduz interrompu ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
    if not mental and game['mental_extension']:print('Extension des facultes mentales !')
    if not before and game['tasks_done'][0]:print(f"Union mentale accomplie ; {game['tasks_counter']} taches sur 4.")
    return game
