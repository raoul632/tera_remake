"""Pilotage des operations C de la Cite, sans regles de navigation Python."""
import subprocess
import sys
from combat import HERE, RESOURCE, read_key, records
from creation import apply_snapshot
from exploration import pack_exploration


def pack_navigation(game,key,enter=False,initial_move=False,resource=RESOURCE,leave=False):
    """Ajouter au transport commun les champs de contexte de l'overlay."""
    extra=[int(initial_move),game.get('city_backup_position',0),
           game.get('city_overlay',int(game['world_id']!=0)),game.get('tasks_counter',0),
           game.get('tasks_done',[0]*4)[2],game['arioch_position'],game.get('chest_open',0),
           *game.get('has_map',[0]*11),game.get('tasks_done',[0]*4)[3],
           *game.get('secret_words',['']*4)[3].encode('cp437').ljust(10,b'\0')]
    return pack_exploration(game,1 if leave else 2 if enter else 0,key,resource)+' '.join(map(str,extra))+'\n'


def apply_navigation(game,values):
    """Reintegrer flags, contexte et progression ; les cartes arrivent par B."""
    fields=('navigation_phase','grid_moved','changed_level','city_backup_position',
            'city_overlay','encounter_suppressed','recruit_ready','location_changed',
            'previous_position','hint_count','hint1','hint2','healed_mask',
            'monster_type','combat_actor','combat_attempt','combat_phase','fled','victory')
    game.update(zip(fields,values[:19]))
    game['tasks_done'][2]=values[19];game['tasks_counter']=values[20]
    game['force_day']=values[21];game['day_advanced']=values[22];game['level_advanced']=values[23]


def execute_navigation(game,key,enter=False,initial_move=False,resource=RESOURCE,leave=False):
    """Attendre les portes, confirmations et combats du C jusqu'au retour."""
    exe=HERE/'build/cmake'/('tera_navigation_bridge.exe' if sys.platform=='win32' else 'tera_navigation_bridge')
    names=records(resource/'NOMON',40,22);messages=records(resource/'MESSAGE',40,102)
    objects=records(resource/'OBJETS',16,25)
    content_before=game['maps'][game['position']][4] if 0<=game['position']<1100 else 0
    money_before=game['money']
    temple_objective_before=game['tasks_done'][3]
    process=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,text=True,bufsize=1)
    try:
        process.stdin.write(pack_navigation(game,key,enter,initial_move,resource,leave));process.stdin.flush()
        for line in process.stdout:
            tag,*raw=line.split();v=list(map(int,raw))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='J':game['information_context']=v[0]
            elif tag=='F':game['tasks_done'][3]=v[0]
            elif tag=='R':game['temple_entry_pending']=v[0]
            elif tag=='N':apply_navigation(game,v)
            elif tag=='B':
                game['maps'][v[0]]=v[1:];game['maps_column7'][v[0]]=v[7]
            elif tag=='W':game['invocation_word']=bytes(v[1:]).split(b'\0')[0].decode('cp437')
            elif tag=='G':
                game.update(zip(('chest_open','chest_phase','chest_trap','item_outcome','chest_actor','pending_item'),v[:6]))
                game['has_map']=v[6:]
            elif tag=='E' and v[0] in (3,4):print(f"{game['actors'][v[1]]['name']} : {v[3]} degats")
            elif tag=='Z':
                phase=game['navigation_phase']
                if phase==3:
                    if not temple_objective_before and game['tasks_done'][3]:
                        print(f"{game['secret_words'][3]} vaincu ; {game['tasks_counter']} taches sur 4 accomplies.")
                        temple_objective_before=1
                    if key in (ord('O'),ord('P')):
                        traps={0:'Gaz nocif : le groupe est empoisonne.',1:'Explosion !',2:'Cyanure !',3:'Une lame se declenche !',4:'Le piege est dejoue.'}
                        if game.get('chest_trap',-1) in traps:print(traps[game['chest_trap']])
                        outcome=game.get('item_outcome',0)
                        if outcome==2:print('Inventaire plein : le contenu reste dans le coffre.')
                        elif outcome==3:print('Personnage HS.')
                        elif outcome==4:print('Le coffre est vide.')
                        elif outcome==9:print('Annule.')
                        elif key==ord('P') and outcome==1:print('Carte prise.' if content_before==99 else 'Cle prise.' if content_before>89 else 'Objet pris.')
                        elif game['money']!=money_before:print(f"Argent trouve ; total {game['money']}.")
                        elif game.get('chest_open',0):
                            content=game['maps'][game['position']][4]
                            print('Coffre ouvert : '+(objects[content-1] if 1<=content<=25 else 'carte' if content==99 else 'cle' if content>89 else 'vide')+' ; P pour prendre.')
                    for index in (game['hint1'],game['hint2'])[:game['hint_count']]:
                        if index==100:print('O etranger gardes-toi de penetrer dans ces lieux sacres et terribles..')
                        elif index==101:print('AUTEL DES TENEBRES')
                        else:print(messages[69] if index==0 else game['hints'][index-1])
                    if game['healed_mask']:print('Le rituel restaure HP et MP des personnages ayant reussi.')
                    continue
                if phase==4:continue
                if phase==6:
                    prompt='O/Entree confirme : ' if game['chest_phase']==0 else 'Qui ? A-D (ESC annule seulement ouvrir) : '
                elif phase==5:prompt=('\rInvoquer qui ? '+game.get('invocation_word','')).ljust(40)
                elif phase==0:prompt='Porte : O pour ouvrir, U pour utiliser la cle : '
                elif phase==1:prompt='O/Entree confirme, autre touche annule : '
                else:
                    if game['combat_phase']==0:prompt=names[game['monster_type']-1]+' : T L J A D P C F ? '
                    elif game['combat_phase']==1:prompt='O/Entree confirme : '
                    else:prompt='Mot : '
                print(prompt,end='',flush=True)
                process.stdin.write(str(read_key())+'\n');process.stdin.flush()
        process.stdin.close();code=process.wait(timeout=5)
        if code:raise ValueError(process.stderr.read().strip() or f'Navigation C interrompue ({code})')
    finally:
        if process.poll() is None:process.kill();process.wait()
    return game
