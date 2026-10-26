"""Transport et presentation des objets ; les decisions appartiennent au C."""
import subprocess
import sys
from combat import HERE,RESOURCE,read_key,records
from creation import pack_roster,apply_snapshot


def pack_inventory(game,command,acquire=False,item=0):
    """Le mode acquisition est un banc CAA13 ; aucun objet n'est cree en jeu."""
    return f'{int(acquire)} {command}\n'+pack_roster(game,False)+(f'{item}\n' if acquire else '')


def show_inventory(game,actor,objects):
    """Afficher les dix slots originaux, sans tri, equipement actif invente ni jet."""
    person=game['actors'][actor]
    print(person['name']+' : '+person['condition'])
    for slot,item in enumerate(person['equipment']):
        print(f'{slot}: '+(objects[item-1] if 1<=item<=25 else 'vide' if item==0 else f'ID {item}'))


def execute_inventory(game,command,resource=RESOURCE):
    """A, B, N/D : suivre les lectures originales jusqu'au retour, puis reintegrer."""
    exe=HERE/'build/cmake'/('tera_inventory_bridge.exe' if sys.platform=='win32' else 'tera_inventory_bridge')
    objects=records(resource/'OBJETS',16,25)
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    try:
        p.stdin.write(pack_inventory(game,command));p.stdin.flush()
        for line in p.stdout:
            tag,*raw=line.split();v=list(map(int,raw))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='J':game['alfol_status_actor']=v[0]
            elif tag=='I':
                phase,subject,slot,target,pending,outcome=v
                game['inventory_result']=dict(phase=phase,subject=subject,slot=slot,target=target,pending=pending,outcome=outcome)
                if phase>=6:
                    labels={1:'Objet pris.',2:'Inventaire plein.',3:'Personnage HS.',4:'Emplacement vide.',
                        5:'Cet objet ne se boit pas.',6:'Potion consommee.',7:'Abandon effectue.',8:'Objet donne.',9:'Annule.',10:'Personnage absent.'}
                    if outcome in labels:print(labels[outcome])
                    continue
                if phase==0:prompt='Negocier : D donner (autres dialogues non portes ici) : '
                elif phase==1:prompt='Donner ? O/Entree : '
                elif phase==5:prompt='Abandonner : C compagnon, O objet : '
                elif phase==3:
                    show_inventory(game,subject,objects);prompt='Slot 0-9 (autre touche redemande) : '
                else:
                    active=[i for i in range(1,10) if game['actors'][i]['active']][:3]
                    print('A: '+game['actors'][0]['name']+' ; '+' ; '.join(f'{chr(66+j)}: '+game['actors'][i]['name'] for j,i in enumerate(active)))
                    prompt='A qui ? ' if phase==4 else 'Qui ? '
                print(prompt,end='',flush=True);p.stdin.write(f'{read_key()}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Inventaire interrompu ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
    return game
