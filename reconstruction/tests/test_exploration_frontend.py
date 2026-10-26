"""Creation generee -> arrivee -> marche/jour -> export, sans fenetre DOS."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

HERE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(HERE/'frontend'))
from exploration import pack_exploration
from combat import demo_game


def fixture(seed):
    # Contexte synthetique annonce dans ce test uniquement ; aucun emulateur
    # n'est requis pour les contrats de transport des frontends.
    game=demo_game(7,seed)
    game.update(party_available=1,days=7300,level=0,dummy_level=0,energy=50,
                keys=[10]*10,name_variants=[['']*10,['']*10],arrival_pending=0,
                alfol_position=75,sound=1,moved=0,town_found=0,town_id=0,
                force_day=0,encounter_suppressed=0,world_id=11,position=1000,
                horizontal_mountains=[-1]*9,vertical_mountains=[-1]*25,
                towns=[-1]*20,locations=[0]+[-1]*9,extra_locations=[0]+[-1]*9,
                maps=[[0,0,0,0,255,0,v] for v in game['maps_column7']])
    return game


class ExplorationFrontendTests(unittest.TestCase):
    def run_cli(self,program,args,keys):
        return subprocess.run([sys.executable,str(HERE/'frontend'/program),*args],
                              input=keys,text=True,capture_output=True,timeout=30)

    def test_generated_start_walk_and_export_preserve_world(self):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'new.json';destination=Path(directory)/'walk.json'
            creation=self.run_cli('world.py',['--seed','1986','--output',str(source)],'ALIX\rH333FIVEHAO')
            self.assertEqual(creation.returncode,0,creation.stderr)
            initial=json.loads(source.read_text(encoding='utf-8'));raw=source.read_bytes()
            result=self.run_cli('exploration.py',['--state',str(source),'--output',str(destination)],'8664Q')
            self.assertEqual(result.returncode,0,result.stderr)
            final=json.loads(destination.read_text(encoding='utf-8'))
            self.assertEqual(source.read_bytes(),raw)
        self.assertEqual(final['arrival_pending'],0)
        self.assertGreater(final['days'],initial['days'])
        self.assertNotEqual(final['seed'],initial['seed'])
        self.assertEqual(final['day_phase'],2)
        for field in ('maps','maps_column7','towns','locations','hints','secret_words','helpers'):
            self.assertEqual(final[field],initial[field],field)

    def test_eof_does_not_export_unfinished_exploration(self):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'new.json';destination=Path(directory)/'walk.json'
            self.assertEqual(self.run_cli('world.py',['--seed','1986','--output',str(source)],
                                         'ALIX\rH333FIVEHAO').returncode,0)
            result=self.run_cli('exploration.py',['--state',str(source),'--output',str(destination)],'')
            self.assertNotEqual(result.returncode,0)
            self.assertFalse(destination.exists())

    def test_seduction_does_not_invent_a_solicitation(self):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'new.json'
            self.assertEqual(self.run_cli('world.py',['--seed','1986','--output',str(source)],
                                         'ALIX\rH333FIVEHAO').returncode,0)
            baseline=Path(directory)/'baseline.json';result=Path(directory)/'seduced.json'
            self.assertEqual(self.run_cli('exploration.py',['--state',str(source),'--output',str(baseline)],'Q').returncode,0)
            process=self.run_cli('exploration.py',['--state',str(source),'--output',str(result)],'SOQ')
            self.assertEqual(process.returncode,0,process.stderr)
            old=json.loads(baseline.read_text(encoding='utf-8'));new=json.loads(result.read_text(encoding='utf-8'))
            self.assertEqual(new['recruitment_result']['outcome'],8)
            self.assertEqual(new['recruitment_result']['rolls'],1)
            for field in ('actors','money','experience','days','position'):
                self.assertEqual(old[field],new[field])
            self.assertNotEqual(old['seed'],new['seed'])

    def test_real_nested_combat_returns_to_daily_clock(self):
        # Trouver une rencontre avec le C, pas avec une formule Python.
        bridge=HERE/'build/cmake/tera_exploration_bridge.exe'
        for seed in range(40):
            game=fixture(seed);game.update(experience=255,level=10,position=1500)
            probe=subprocess.run([str(bridge)],input=pack_exploration(game,1,ord('8')),
                                 text=True,capture_output=True,timeout=10)
            if probe.returncode==4:break
        else:self.fail('Aucune rencontre dans le corpus')
        final=subprocess.run([str(bridge)],input=pack_exploration(game,1,ord('8'))+'70\n79\n',
                             text=True,capture_output=True,timeout=10)
        self.assertEqual(final.returncode,0,final.stderr)
        rows=[list(map(int,line.split()[1:])) for line in final.stdout.splitlines() if line.startswith('D ')]
        self.assertEqual(rows[0][0],1)
        self.assertEqual(rows[-1][0],2)
        self.assertEqual(rows[-1][3],1)
        self.assertEqual(rows[-1][17],1)


if __name__=='__main__':unittest.main()
