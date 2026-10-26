"""Kronopolis : les prix, achats/ventes et soins restent dans le moteur C."""
import subprocess
import sys
from combat import HERE,RESOURCE,read_key,records
from creation import apply_snapshot
from exploration import pack_exploration
from inventory import show_inventory


def present_place(game,resource=RESOURCE,force=False):
    """Executer la facade apres le pas/jour ou un retour Cite/Seth/Alfol."""
    exe=HERE/'build/cmake'/('tera_places_bridge.exe' if sys.platform=='win32' else 'tera_places_bridge')
    p=subprocess.run([str(exe)],input=pack_exploration(game,0,int(force),resource),text=True,capture_output=True,timeout=10)
    if p.returncode:raise ValueError(p.stderr.strip() or 'Facade interrompue')
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':apply_snapshot(game,v)
        elif tag=='V':game['moved']=v[0]
        elif tag=='J':game['information_context']=v[0]
    return game


def execute_commerce(game,resource=RESOURCE):
    """Lire l'entree confirmee puis la session complete ; aucun export partiel."""
    exe=HERE/'build/cmake'/('tera_commerce_bridge.exe' if sys.platform=='win32' else 'tera_commerce_bridge')
    messages=records(resource/'MESSAGE',40,102);objects=records(resource/'OBJETS',16,25)
    packet=pack_exploration(game,2,ord('E'),resource)+' '.join(map(str,game['has_map']))+'\n'
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    try:
        p.stdin.write(packet);p.stdin.flush()
        for line in p.stdout:
            tag,*raw=line.split();v=list(map(int,raw))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='K':
                phase,command,actor,item,price,weeks,outcome=v[:7];game['has_map']=v[7:18]
                game['shop_result']=dict(phase=phase,item=item,price=price,weeks=weeks,outcome=outcome)
                if outcome:print({1:f'Achat effectue : {price} ducats.',2:f'Vente : {price} ducats.',
                    3:f'Repos : {weeks} semaine(s).',4:'Pas assez de pepetes.',5:'Inventaire plein.',6:'Personnage HS.'}[outcome])
                if phase>=5:continue
                if phase==0:prompt='Kronopolis : A acheter, V vendre, H hopital, ESC sortir : '
                elif phase==1:prompt='O/Entree confirme, autre touche annule : '
                elif phase==2:prompt='Qui effectue la transaction ? A-D : '
                elif phase==3:
                    print('\n'.join(messages[71:90]));prompt=f'Argent {game["money"]} ; achat A-S, ESC retour : '
                else:
                    show_inventory(game,actor,objects);prompt='Slot 0-9 a vendre, ESC retour : '
                print(prompt,end='',flush=True);p.stdin.write(f'{read_key()}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Commerce interrompu ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
    return game
