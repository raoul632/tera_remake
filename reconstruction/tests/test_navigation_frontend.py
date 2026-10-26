"""Verifications de transport et de reprise de l'exploration, sans emulateur."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

HERE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(HERE/'frontend'))
from world import generate_world
from creation import pack_roster,apply_snapshot
from navigation import pack_navigation,apply_navigation


def create_generated_game(seed=1986):
    """Le vrai monde et le vrai heros C ; aucun profil Python de remplacement."""
    game=generate_world(seed,1);game['party_available']=1
    result=subprocess.run([str(HERE/'build/cmake/tera_creation_bridge.exe')],
        input=pack_roster(game,False)+''.join(f'{ord(k)}\n' for k in 'ALIX\rH333FIVEHAO'),
        text=True,capture_output=True,check=True)
    rows=[list(map(int,line.split()[1:])) for line in result.stdout.splitlines() if line.startswith('S ')]
    apply_snapshot(game,rows[-1]);game['arrival_pending']=0
    return game


def run_bridge(game,key,text,enter=False,leave=False):
    """Decodage identique au frontend, sans simuler une reponse du moteur."""
    p=subprocess.run([str(HERE/'build/cmake/tera_navigation_bridge.exe')],
        input=pack_navigation(game,key,enter=enter,leave=leave)+''.join(f'{ord(k)}\n' for k in text),
        text=True,capture_output=True,timeout=10)
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':apply_snapshot(game,v)
        elif tag=='N':apply_navigation(game,v)
        elif tag=='B':game['maps'][v[0]]=v[1:];game['maps_column7'][v[0]]=v[7]
    return p


class NavigationFrontendTests(unittest.TestCase):
    def launch(self,source,output,keys):
        return subprocess.run([sys.executable,str(HERE/'frontend/exploration.py'),
            '--state',str(source),'--output',str(output)],input=keys,text=True,capture_output=True,timeout=30)

    def test_entry_cancel_and_reload_keeps_surface(self):
        game=create_generated_game();game.update(position=game['towns'][2],town_found=1,town_id=2,
            city_backup_position=1234)
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'game.json';output=Path(directory)/'next.json'
            source.write_text(json.dumps(game),encoding='utf-8');raw=source.read_bytes()
            result=self.launch(source,output,'ENQ')
            self.assertEqual(result.returncode,0,result.stderr)
            final=json.loads(output.read_text(encoding='utf-8'))
            self.assertEqual(source.read_bytes(),raw)
        for field in ('position','world_id','seed','maps','actors','city_backup_position'):
            self.assertEqual(final[field],game[field],field)

    def test_doors_persist_after_export_and_reload(self):
        game=create_generated_game();game.update(world_id=2,position=245,previous_position=244,
            city_overlay=1,city_backup_position=1500)
        # Couloir d'integration explicite : seul ce test impose ses murs,
        # afin de verifier l'export des mutations sans combattre une fixture.
        game['maps'][245][:4]=[3,3,1,1];game['maps'][245][4]=0;game['maps'][245][5]=0
        game['maps'][246][4:]=[0,0,1];game['keys'][2]=2
        before=copy.deepcopy(game)
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'game.json';output=Path(directory)/'next.json';again=Path(directory)/'again.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            result=self.launch(source,output,'8UQ')
            self.assertEqual(result.returncode,0,result.stderr)
            final=json.loads(output.read_text(encoding='utf-8'))
            self.assertEqual(final['position'],246)
            self.assertEqual(final['maps'][245][:4],[2,2,1,1])
            self.assertEqual(final['keys'][2],2)
            result=self.launch(output,again,'Q')
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(json.loads(again.read_text(encoding='utf-8'))['maps'],final['maps'])
        for field in ('locations','hints','secret_words','helpers'):
            self.assertEqual(final[field],before[field],field)

    def test_surface_entry_and_exit_restore_saved_location(self):
        game=create_generated_game();game.update(position=game['towns'][2],town_found=1,town_id=2)
        # Le test impose une premiere salle sans rencontre ni coffre ; la
        # generation/creation reelles restent celles du programme C.
        game['maps'][1][2]=1;game['maps'][1][4]=0;game['maps'][1][5]=0
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'game.json';output=Path(directory)/'next.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            result=self.launch(source,output,'EO2Q')
            self.assertEqual(result.returncode,0,result.stderr)
            final=json.loads(output.read_text(encoding='utf-8'))
        self.assertEqual(final['world_id'],11)
        self.assertEqual(final['position'],game['position'])
        self.assertEqual(final['city_backup_position'],game['position'])
        self.assertEqual(final['maps'],game['maps'])
        self.assertNotEqual(final['seed'],game['seed'])

    def test_interrupted_gate_does_not_export(self):
        game=create_generated_game();game.update(world_id=2,position=245,previous_position=244,city_overlay=1)
        game['maps'][245][0]=2
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'game.json';output=Path(directory)/'next.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            result=self.launch(source,output,'8')
            self.assertNotEqual(result.returncode,0)
            self.assertFalse(output.exists())

    def test_fixed_victory_and_objective_persist_after_reload(self):
        game=create_generated_game();game.update(world_id=2,position=245,previous_position=244,
            city_overlay=1,city_backup_position=1500,arioch_position=246,tasks_counter=0)
        game['tasks_done']=[0]*4;game['maps'][245][0]=1
        game['maps'][245][4]=0;game['maps'][246][4:]=[0,1,1]
        # Profil renforce uniquement pour atteindre vite la branche victoire
        # de ce test de persistance. Il n'est jamais exporte comme jeu neuf.
        actor=game['actors'][0];actor.update(hp=10000,magic=200,condition='OK',
            stats=[0]+[10]*9,skills=[0]+[3]*9,equipment=[18,21]+[0]*8)
        # Le test vise la victoire/export : ne pas envoyer des O surnumeraires
        # maintenant que cette touche ouvre reellement les coffres du lieu.
        probe=run_bridge(copy.deepcopy(game),ord('8'),'TO'*30)
        self.assertEqual(probe.returncode,0,probe.stderr)
        consumed=sum(line.startswith('S ') for line in probe.stdout.splitlines())-1
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'game.json';output=Path(directory)/'next.json';again=Path(directory)/'again.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            result=self.launch(source,output,'8'+('TO'*30)[:consumed]+'Q')
            self.assertEqual(result.returncode,0,result.stderr)
            final=json.loads(output.read_text(encoding='utf-8'))
            self.assertEqual(final['maps'][246][5],0)
            self.assertEqual(final['tasks_done'][2],1)
            self.assertEqual(final['tasks_counter'],1)
            self.assertGreater(final['experience'],game['experience'])
            self.assertEqual(final['information_context'],1) # C9B55 -> C6073, meme sans pixels.
            result=self.launch(output,again,'Q')
            self.assertEqual(result.returncode,0,result.stderr)
            reloaded=json.loads(again.read_text(encoding='utf-8'))
            for field in ('maps','seed','tasks_done','tasks_counter','money','experience','actors','information_context'):
                self.assertEqual(reloaded[field],final[field],field)


if __name__=='__main__':unittest.main()
