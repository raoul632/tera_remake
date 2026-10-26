"""Verifier le passage des donnees entre les executables C, sans regles Python."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE/'frontend'))
from world import generate_world
from recruitment import execute_request


class WorldFrontendTests(unittest.TestCase):
    def launch(self, program, arguments, keys):
        return subprocess.run([sys.executable, str(HERE/'frontend'/program), *arguments],
                              input=keys, text=True, capture_output=True, timeout=30)

    def test_world_creation_recruitment_combat_preserve_context(self):
        """La vraie generation, puis le heros, alimentent les deux interactions."""
        initial = generate_world(1986, 1)
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            created = Path(directory)/'created.json'
            result = self.launch('world.py', ['--seed','1986','--output',str(created)],
                                 'ALIX\rH333FIVEHAO')
            self.assertEqual(result.returncode, 0, result.stderr)
            game = json.loads(created.read_text(encoding='utf-8'))
            self.assertEqual(game['creation_result']['phase'], 5)
            self.assertNotEqual(game['seed'], initial['seed'])
            for field in ('maps','maps_column7','locations','towns','hints','secret_words','helpers'):
                self.assertEqual(game[field], initial[field], field)
            # Position imposee uniquement pour le banc : deux compagnons
            # sont co-localises par le generateur original pour cette graine.
            self.assertEqual(game['locations'][7], game['locations'][9])
            before = copy.deepcopy(game)
            game['position'] = game['locations'][7]
            solicitation = execute_request(game, 0, 13)
            self.assertEqual(solicitation['ready'], 1)
            self.assertEqual(game['seed'], before['seed'])
            recruitment = execute_request(game, 1, ord('O'))
            self.assertEqual(recruitment['rolls'], 10)
            self.assertTrue(game['actors'][7]['active'])
            self.assertTrue(game['actors'][9]['active'])
            game['world_id'] = 9
            recruited = Path(directory)/'recruited.json'
            recruited.write_text(json.dumps(game), encoding='utf-8')
            raw = recruited.read_bytes()
            battle = Path(directory)/'battle.json'
            result = self.launch('combat.py', ['--state',str(recruited),'--monster','7',
                                               '--output',str(battle)], 'FO')
            self.assertEqual(result.returncode, 0, result.stderr)
            final = json.loads(battle.read_text(encoding='utf-8'))
            self.assertEqual(final['combat_result']['fled'], 1)
            self.assertEqual(final['maps_column7'][final['position']],
                             initial['maps_column7'][final['position']])
            self.assertEqual(recruited.read_bytes(), raw)
            for field in ('maps','hints','helpers','secret_words','locations'):
                self.assertEqual(final[field], initial[field], field)

    def test_cancelled_recruitment_preserves_generated_state(self):
        game = generate_world(27, 9)
        game['recruit_ready'] = 1
        previous = copy.deepcopy(game)
        result = execute_request(game, 1, 27)
        self.assertEqual(result['outcome'], 1)
        self.assertEqual(result['ready'], 1)
        for field in previous:
            if field != 'creation_result':
                self.assertEqual(game[field], previous[field], field)

    def test_incomplete_world_creation_does_not_export(self):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            destination = Path(directory)/'game.json'
            result = self.launch('world.py', ['--seed','1','--output',str(destination)], 'ALIX\rH')
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(destination.exists())

    def test_missing_monster_explains_combat_bench_requirement(self):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source = Path(directory)/'game.json'
            source.write_text(json.dumps(generate_world(1,1)), encoding='utf-8')
            result = self.launch('combat.py', ['--state',str(source)], '')
            self.assertNotEqual(result.returncode,0)
            self.assertIn('--monster',result.stderr)
            self.assertNotIn('Traceback',result.stderr)


if __name__=='__main__':
    unittest.main()
