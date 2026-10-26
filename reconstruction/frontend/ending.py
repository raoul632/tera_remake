"""OVL00CB : felicitations/condoleances finales, score calcule dans le C."""
import subprocess
import sys
from combat import HERE,RESOURCE,read_key
from creation import apply_snapshot
from exploration import pack_exploration


def pack_ending(game,resource=RESOURCE):
    return pack_exploration(game,0,0,resource)+' '.join(map(str,game['tasks_done']))+'\n'


def execute_ending(game,resource=RESOURCE):
    """Consommer la musique et la pause une seule fois, puis marquer la fin.

    Le snapshot JSON moderne conserve cette fin. La sauvegarde scenario
    DOS reste distincte : elle n'est pas simulee par ce conteneur.
    """
    exe=HERE/'build/cmake'/('tera_ending_bridge.exe' if sys.platform=='win32' else 'tera_ending_bridge')
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    announced=False
    try:
        p.stdin.write(pack_ending(game,resource));p.stdin.flush()
        for line in p.stdout:
            tag,*raw=line.split()
            if tag=='S':apply_snapshot(game,list(map(int,raw)))
            elif tag=='K':
                v=list(map(int,raw));game.update(ending_complete=v[0],ending_victory=v[1],ending_error=v[2],score_real48=v[3:])
            elif tag=='R':
                game['score']=float(raw[0]);game['score_text']=raw[1]
                if not announced:
                    if game['ending_error']:print(f"Fin interrompue par l'erreur Pascal {game['ending_error']} (score).")
                    elif game['ending_victory']:
                        print(f"Felicitations ! ARIOCH et {game['secret_words'][3]} sont retournes au chaos. L'HARMONIE regne de nouveau.")
                        print('SCORE -> '+game['score_text'])
                    else:print('Condoleances... Les quatre taches sont accomplies ; cette aventure se termine.')
                    announced=True
                if not game['ending_complete']:
                    print('Appuie sur une touche : ',end='',flush=True);p.stdin.write(f'{read_key()}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Fin interrompue ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
    return game
