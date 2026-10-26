"""Tests des executables et de leur transport, sans oracle de regles Python."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE/'frontend'))
from combat import demo_game


class CreationFrontendTests(unittest.TestCase):
    def create(self, directory, extra=(), text='ALIX\rH333FIVEHAO'):
        output = Path(directory)/'characters.json'
        result = subprocess.run([sys.executable,str(HERE/'frontend/creation.py'),
                                 '--seed','1986','--output',str(output),*extra],
                                input=text,text=True,capture_output=True,timeout=10)
        self.assertEqual(result.returncode,0,result.stderr)
        return output,json.loads(output.read_text(encoding='utf-8'))

    def test_real_starting_hero_and_companions_export(self):
        """Pas de retour accidentel au profil surarme du mode demo."""
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            path, roster = self.create(directory)
        hero = roster['actors'][0]
        self.assertEqual(hero['name'],'ALIX')
        self.assertEqual(hero['equipment'],[0]*10)
        self.assertEqual(hero['skills'],[0]*10)
        self.assertEqual(hero['hp'],hero['hp_limit'])
        self.assertEqual(hero['magic'],hero['magic_limit'])
        self.assertEqual(roster['creation_result']['phase'],5)
        self.assertEqual(roster['days'],7300)
        self.assertEqual(roster['world_id'],11)
        self.assertEqual(roster['position'],1000)
        self.assertEqual(roster['actors'][9]['equipment'][:2],[21,18])
        self.assertEqual([a['active'] for a in roster['actors']],[1]+[0]*9)
        self.assertNotIn('maps_column7',roster) # Monde inconnu : ne pas le fabriquer.

    def test_recreation_retains_inventory_and_raises_money_only_to_floor(self):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            _, roster = self.create(directory)
            roster['actors'][0]['equipment'][0] = 13
            roster['money'] = 700
            source = Path(directory)/'previous.json'
            source.write_text(json.dumps(roster),encoding='utf-8')
            raw = source.read_bytes()
            _, result = self.create(directory,extra=('--roster',str(source)),
                                    text='ALIX\rH000FFFFFFNSECOND\rF999NNNNNNO')
            self.assertEqual(source.read_bytes(),raw)
        self.assertEqual(result['actors'][0]['name'],'SECOND')
        self.assertEqual(result['actors'][0]['equipment'][0],13)
        self.assertEqual(result['money'],700)
        self.assertEqual(result['actors'][0]['skills'],[0]*10)

    def test_character_handoff_keeps_map_and_context(self):
        """Lier le roster au combat sans effacer les donnees du monde hote."""
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            roster_path,roster = self.create(directory)
            game = demo_game(7,123)
            game['maps_column7'] = [i%251 for i in range(1100)]
            game['helpers'] = [12,9,6,3]
            game['secret_word'] = 'SECRET'
            source,destination = Path(directory)/'world.json',Path(directory)/'battle.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            raw = source.read_bytes()
            process = subprocess.run([sys.executable,str(HERE/'frontend/combat.py'),
                                      '--state',str(source),'--characters',str(roster_path),
                                      '--output',str(destination)],input='FO',text=True,
                                     capture_output=True,timeout=10)
            self.assertEqual(process.returncode,0,process.stderr)
            result = json.loads(destination.read_text(encoding='utf-8'))
            self.assertEqual(source.read_bytes(),raw)
        self.assertEqual(result['actors'][0]['name'],'ALIX')
        self.assertEqual(result['actors'][0]['stats'],roster['actors'][0]['stats'])
        self.assertEqual(result['actors'][0]['equipment'],[0]*10)
        self.assertEqual(result['maps_column7'],game['maps_column7'])
        self.assertEqual(result['helpers'],game['helpers'])
        self.assertEqual(result['secret_word'],game['secret_word'])
        self.assertEqual(result['world_id'],11)
        self.assertEqual(result['days'],7300)
        self.assertEqual(result['combat_result']['fled'],1)

    def test_incomplete_creation_does_not_export_final_roster(self):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            output = Path(directory)/'characters.json'
            process = subprocess.run([sys.executable,str(HERE/'frontend/creation.py'),
                                      '--seed','1986','--output',str(output)],
                                     input='ALIX\rH',text=True,capture_output=True,timeout=10)
            self.assertNotEqual(process.returncode,0)
            self.assertFalse(output.exists())


if __name__=='__main__':
    unittest.main()
