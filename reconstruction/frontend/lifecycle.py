"""Defaite -> successeur dans la meme partie, sans nouvelle generation."""
import subprocess
import sys
from combat import HERE,RESOURCE,read_key
from creation import apply_snapshot,show_stats
from exploration import pack_exploration


def pack_lifecycle(game,resource=RESOURCE):
    return pack_exploration(game,0,0,resource)+' '.join(map(str,[*game['tasks_done'],game.get('fled',0),game.get('victory',0)]))+'\n'


def execute_lifecycle(game,resource=RESOURCE):
    """Acquitter les condoleances puis piloter OVL0022 sans regenerer le monde.

    Renvoie l'etat avant arrivee automatique, ou apres fin des objectifs.
    Les cartes/progres restent dans game ; une creation incomplete echoue.
    """
    exe=HERE/'build/cmake'/('tera_lifecycle_bridge.exe' if sys.platform=='win32' else 'tera_lifecycle_bridge')
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    try:
        p.stdin.write(pack_lifecycle(game,resource));p.stdin.flush()
        for line in p.stdout:
            tag,*raw=line.split();v=list(map(int,raw))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='L':
                life,phase,split,left,name_length,attempts,alfol,fled,ready,victory=v
                game.update(lifecycle_phase=life,on_alfol=alfol,fled=fled,recruit_ready=ready,victory=victory)
                if life>=2:continue
                if life==0:
                    print('Condoleances... Le butin du heros est conserve pour son successeur.')
                    prompt='Appuie sur une touche : '
                elif phase==0:prompt='Nom du successeur : '
                elif phase==1:prompt='H(omme) ou F(emme) : '
                elif phase==2:
                    show_stats(game['actors'][0]);prompt=('Endurance','Volonte','Vitalite')[split-1]+' : '
                elif phase==3:
                    show_stats(game['actors'][0]);prompt=f'Bonus restants {left} : F I N E V T A C H : '
                else:
                    show_stats(game['actors'][0]);prompt='Gardes-tu ce personnage ? N recommence, autre touche accepte : '
                print(prompt,end='',flush=True);p.stdin.write(f'{read_key()}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Recreation interrompue ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
    return game
