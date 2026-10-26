"""Dialogue N/S : etat et choix du C, indices des ressources du monde genere."""
import subprocess
import sys
from combat import HERE,RESOURCE,records,read_key
from creation import apply_snapshot
from exploration import pack_exploration
from inventory import show_inventory


def execute_dialogue(game,command,resource=RESOURCE):
    """Piloter N/S jusqu'au retour ; reintegrer le roster, AFCE et A750.

    Le C decide les indices/reponses/tirages, Python affiche les ressources.
    Une interruption leve une erreur avant l'export de l'exploration.
    """
    exe=HERE/'build/cmake'/('tera_dialogue_bridge.exe' if sys.platform=='win32' else 'tera_dialogue_bridge')
    objects=records(resource/'OBJETS',16,25);messages=records(resource/'MESSAGE',40,102)
    responses=[messages,records(resource/'MESSAGE2',40,33),records(resource/'MESSAGE3',40,33)]
    p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
    try:
        p.stdin.write(pack_exploration(game,0,command,resource));p.stdin.flush()
        for line in p.stdout:
            tag,*raw=line.split();v=list(map(int,raw))
            if tag=='S':apply_snapshot(game,v)
            elif tag=='J':game['alfol_status_actor']=v[0]
            elif tag=='T':
                phase,_,choice,ready,available,extra,message,count=v[:8]
                game['recruit_ready']=ready;game['information_context']=v[19]
                game['dialogue_result']=dict(phase=phase,choice=choice,available=available,extra=extra,
                    message=message,hints=v[8:8+count],recruitment=v[12],item_outcome=v[18])
                if v[12]:game['recruitment_result']=dict(outcome=v[12],ready=ready,available_mask=available,
                    rolls=v[20],negotiator=v[21],initial_party_count=v[22],joined_mask=v[23])
                if phase==3:
                    if v[24]>=0:print(responses[v[25]][v[24]-1])
                    if count or message:
                        if extra<0:print('...')
                        for index in v[8:8+count]:print(game['hints'][index-1])
                        if message==2:print(messages[45])
                    labels={2:'Un compagnon peut etre sollicite ici.',3:'Personne ne repond.',4:'Groupe complet.',
                        5:'Recrutement par negociation.',6:'Recrutement contre 250.',7:'Argent insuffisant.',8:'Aucune sollicitation en attente.'}
                    if v[12] in labels:print(labels[v[12]])
                    if v[18]:print({2:'Inventaire plein.',3:'Personnage HS.',4:'Emplacement vide.',8:'Objet donne.'}.get(v[18],'Dialogue termine.'))
                    continue
                if phase==0:prompt='Negocier : I informations, A acheter, V vendre, D donner, S solliciter : '
                elif phase==1:prompt='O/Entree confirme, autre touche annule : '
                elif v[13]==3:
                    show_inventory(game,v[14],objects);prompt='Slot 0-9 : '
                else:
                    active=[i for i in range(1,10) if game['actors'][i]['active']][:3]
                    print('A: '+game['actors'][0]['name']+' ; '+' ; '.join(f'{chr(66+j)}: '+game['actors'][i]['name'] for j,i in enumerate(active)))
                    prompt='A qui ? ' if v[13]==4 else 'Qui ? '
                print(prompt,end='',flush=True);p.stdin.write(f'{read_key()}\n');p.stdin.flush()
        p.stdin.close();code=p.wait(timeout=5)
        if code:raise ValueError(p.stderr.read().strip() or f'Dialogue interrompu ({code})')
    finally:
        if p.poll() is None:p.kill();p.wait()
    return game
