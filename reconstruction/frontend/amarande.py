"""Lieux, panneaux et interactions residentes d'Amarande, regles dans le C."""
import subprocess
import sys
from combat import HERE,RESOURCE,read_key,records
from creation import apply_snapshot,show_stats
from exploration import pack_exploration

TOWNS=('Kronopolis','Spatioport','Cite des cranes','Pyramide','Temple de Seth',
       'Sanctuaire de Mahalore','Domaine de Balgence','Ermitage de la Roche aux Goules',
       'Hameau de la Porte des sables','Abbaye de Mortefontaine')


def pack_amarande(game,key,resource=RESOURCE):
    return pack_exploration(game,0,key,resource)+f"{game.get('alfol_status_actor',9)} {game.get('npc_variant',0)} {game.get('initial_flag',1)}\n"


def show_map(game):
    """Presentation du plan accessible ; les coordonnees viennent du monde.

    WorldId=11 utilise 9417 (initial_flag), pas un achat de carte invente.
    C5495/5B28 montre une fenetre locale de dix colonnes et trente lignes,
    position-135+30*colonne+ligne, avec bouclage ; pas la carte entiere.
    """
    print('Carte locale : @ groupe, V lieu, - et | montagnes.')
    for row in range(30):
        chars=[]
        for col in range(10):
            world=1000+(game['position']-1000-135+col*30+row)%900
            char='V' if world in game['towns'] else '.'
            horizontal=world in game['horizontal_mountains'];vertical=world in game['vertical_mountains']
            if horizontal or vertical:char='+' if horizontal and vertical else '-' if horizontal else '|'
            if col==4 and row==15:char='@'
            chars.append(char)
        print(''.join(chars))


def execute_amarande(game,key,resource=RESOURCE):
    """Suivre les lectures CAFE8 et TERAMAIN jusqu'a leur retour effectif."""
    exe=HERE/'build/cmake'/('tera_amarande_bridge.exe' if sys.platform=='win32' else 'tera_amarande_bridge')
    names=records(resource/'NOMON',40,22)
    responses=[records(resource/name,40,size) for name,size in (('MESSAGE',102),('MESSAGE2',33),('MESSAGE3',33))]
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    last_outcome=None;last_response=0
    try:
        p.stdin.write(pack_amarande(game,key,resource));p.stdin.flush()
        for line in p.stdout:
            tag,*raw=line.split();v=list(map(int,raw))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='Y':
                fields=('amarande_phase','sound','fast_moves','information_context','encounter_suppressed',
                    'recruit_ready','location_changed','previous_position','amarande_outcome','shown_actor',
                    'combat_phase','combat_actor','monster_type','camp_choice','compass_east','compass_north','alfol_status_actor')
                game.update(zip(fields,v))
            elif tag=='Q':game['quit_requested']=v[0]
            elif tag=='F':game['fled']=v[0]
            elif tag=='W':game['invocation_word']=bytes(v[1:]).split(b'\0')[0].decode('cp437')
            elif tag=='R' and v[2]!=last_response:
                print(responses[v[1]][v[0]-1]);last_response=v[2]
            elif tag=='E' and v[0] in (3,4):print(f"{game['actors'][v[1]]['name']} : {v[3]} degats")
            elif tag=='Z':
                phase=game['amarande_phase'];outcome=game['amarande_outcome']
                if outcome!=last_outcome:
                    if outcome==3:show_map(game)
                    elif outcome==4:print('Pas de carte disponible.')
                    elif outcome==5:print('Halte : A accelerer, C carte, D dormir, H aide, I introspection, M manger, S sonder.')
                    elif outcome==6:show_stats(game['actors'][0])
                    elif outcome==7:
                        east,north=game['compass_east'],game['compass_north']
                        print(f"Tu te trouves a {abs(east)} {'E' if east>0 else 'O'} {abs(north)} {'N' if north>0 else 'S'} de Kronopolis.")
                    elif outcome==9:print('HS !')
                    last_outcome=outcome
                if phase>=6:continue
                if phase==3:prompt='Repondre : ' if key==ord('U') else 'Halte : A/C/D/H/I/M/S, ESC quitter : '
                elif phase==1:prompt='Invoquer qui ? '+game.get('invocation_word','')+' '
                elif phase==4:
                    if game.get('camp_choice')==ord('S'):show_stats(game['actors'][game['shown_actor']])
                    prompt='Appuie sur une touche : '
                elif phase in (2,5):
                    combat=game['combat_phase']
                    prompt=names[game['monster_type']-1]+' : T L J A D P C F ? ' if combat==0 else 'O/Entree confirme : ' if combat==1 else 'Mot : '
                else:prompt='O/Entree confirme, autre touche annule : '
                print(prompt,end='',flush=True);p.stdin.write(f'{read_key()}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Evenement Amarande interrompu ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
        for stream in (p.stdin,p.stdout,p.stderr):
            if stream and not stream.closed:stream.close()
    return game
