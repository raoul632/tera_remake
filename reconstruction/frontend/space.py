"""Presentation du voyage OVL00AE ; les regles, jets et degats restent en C."""
import os
import select
import subprocess
import sys
import time
from combat import HERE,RESOURCE,read_key
from creation import apply_snapshot
from exploration import pack_exploration


def pack_space(game,command=1,resource=RESOURCE):
    """command 1 depart, 2 retour, 0 overlay seul (banc de comparaison)."""
    return pack_exploration(game,0,command,resource)+f"{game.get('space_pirates',4)} {game.get('space_backup_position',game['position'])}\n"


def timed_shot(delay_ms,target):
    """Adapter les purges BIOS et le poll final a un terminal.

    En pipe, une touche represente le dernier poll ; 0 represente son absence.
    En terminal, les delais de l'overlay sont conserves : le petit point est
    suivi d'une purge, puis les quatre tailles durent 230/200/170/90 ms.
    """
    if not sys.stdin.isatty():
        print(target)
        key=read_key();return 0 if key==ord('0') else key
    if os.name=='nt':
        import msvcrt
        def flush():
            while msvcrt.kbhit():msvcrt.getwch()
        def poll():return read_key() if msvcrt.kbhit() else 0
        return _animate_shot(delay_ms,target,flush,poll)
    import termios
    import tty
    fd=sys.stdin.fileno();saved=termios.tcgetattr(fd)
    try:
        tty.setcbreak(fd)
        return _animate_shot(delay_ms,target,lambda:termios.tcflush(fd,termios.TCIFLUSH),
            lambda:read_key() if select.select([sys.stdin],[],[],0)[0] else 0)
    finally:termios.tcsetattr(fd,termios.TCSADRAIN,saved)


def _animate_shot(delay_ms,target,flush,poll):
    """Afficher les etapes sans prendre de decision sur le tir."""
    time.sleep(delay_ms/1000);flush()
    print(target)
    print(' . ',end='',flush=True);time.sleep(.300);flush()
    for image,duration in (('o',.230),('O',.200),('(O)',.170),('((O))',.090)):
        print(image,end=' ',flush=True);time.sleep(duration)
        if image=='o':flush()
    key=poll();print();return key


def execute_space(game,returning=False,resource=RESOURCE):
    """Piloter un vol complet ; HALT est durable et n'est pas une defaite sol."""
    exe=HERE/'build/cmake'/('tera_space_bridge.exe' if sys.platform=='win32' else 'tera_space_bridge')
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    try:
        p.stdin.write(pack_space(game,2 if returning else 1,resource));p.stdin.flush()
        for line in p.stdout:
            tag,*v=line.split();v=list(map(int,v))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='X':
                fields=('space_phase','space_pirates','space_counter','space_backup_position','space_pilot',
                    'space_departed','space_alien_x','space_alien_y','space_hit','space_delay_ms','space_trials','on_alfol')
                game.update(zip(fields,v));phase=v[0]
                if phase==5:
                    game['program_halted']=1;print('Vaisseau detruit. Fin de la partie.');continue
                if phase>=4:
                    if not returning and not game['space_departed']:print('Depart annule ou aucun pilote disponible.')
                    continue
                if phase==3:
                    x,y=v[6:8]
                    target='Pirate : '+('gauche','centre','droite')[x+1]+', '+('haut','milieu','bas')[y+1]+'. Tir : pave 1-9.'
                    print(f"Energie {game['energy']} ; pirates {game['space_pirates']}.")
                    key=timed_shot(v[9],target)
                else:
                    if phase==1:print(f"Energie {game['energy']} ; argent {game['money']}.")
                    prompt=('Embarquer ? O/Entree : ','Ravitailler ? O : ','Quantite de carburant (0-9, 50 par unite) : ')[phase]
                    print(prompt,end='',flush=True);key=read_key()
                p.stdin.write(f'{key}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Voyage interrompu ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
    return game
