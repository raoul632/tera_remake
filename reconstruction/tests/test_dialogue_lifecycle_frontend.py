"""Parcours terminal des dialogues et de la succession avec un monde C."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from test_navigation_frontend import create_generated_game,HERE


class DialogueLifecycleFrontendTests(unittest.TestCase):
    def launch(self,g,text):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as directory:
            source=Path(directory)/'source.json';output=Path(directory)/'result.json'
            source.write_text(json.dumps(g),encoding='utf-8');raw=source.read_bytes()
            p=subprocess.run([sys.executable,str(HERE/'frontend/exploration.py'),
                '--state',str(source),'--output',str(output)],input=text,text=True,capture_output=True,timeout=30)
            self.assertEqual(source.read_bytes(),raw)
            final=json.loads(output.read_text(encoding='utf-8')) if output.exists() else None
        return p,final

    def test_information_then_seduction_uses_shared_context(self):
        g=create_generated_game();g.update(sound=0,information_context=1)
        g['locations'][3]=g['extra_locations'][3]=g['position'];g['locations'][8]=g['position']
        g['actors'][3].update(active=0,condition='OK');g['actors'][8].update(active=0,condition='OK')
        p,final=self.launch(g,'NIOSOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['dialogue_result']['recruitment'],5)
        self.assertEqual(final['actors'][3]['active'],1);self.assertEqual(final['experience'],g['experience']+5)
        self.assertIn(g['hints'][4],p.stdout);self.assertIn(g['hints'][5],p.stdout)
        for name in ('money','days','maps','secret_words'):self.assertEqual(final[name],g[name],name)

    def test_purchase_and_sale_are_original_responses(self):
        g=create_generated_game();p,final=self.launch(g,'NAONVOQ')
        self.assertEqual(p.returncode,0,p.stderr)
        for name in ('money','actors','days','maps'):self.assertEqual(final[name],g[name],name)
        self.assertNotEqual(final['seed'],g['seed'])

    def test_cancelled_dialogues_preserve_seed_and_context(self):
        g=create_generated_game();g['recruit_ready']=1
        p,final=self.launch(g,'NINSNQ');self.assertEqual(p.returncode,0,p.stderr)
        for name in ('seed','actors','money','recruit_ready'):self.assertEqual(final[name],g[name],name)

    def dead_game(self):
        g=create_generated_game();g.update(party_available=0,sound=1,money=12,days=8000,
            experience=255,level=3,tasks_done=[1,0,1,0],tasks_counter=2)
        for a in g['actors']:a.update(hp=0,magic=0,condition='HS')
        g['actors'][0]['equipment']=[18,23]+[0]*8
        return g

    def test_death_successor_arrival_export_and_reload(self):
        g=self.dead_game();p,final=self.launch(g,'\rNEXT\rH333FIVEHAOQ')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['actors'][0]['name'],'NEXT');self.assertEqual(final['party_available'],1)
        self.assertEqual(final['money'],500);self.assertEqual(final['experience'],0)
        self.assertEqual(final['arrival_pending'],0);self.assertEqual(final['position'],1000)
        self.assertGreaterEqual(final['days'],7300);self.assertNotEqual(final['seed'],g['seed'])
        for name in ('maps','towns','locations','extra_locations','hints','secret_words','helpers',
                     'tasks_done','tasks_counter','has_map'):
            self.assertEqual(final[name],g[name],name)
        self.assertEqual([a['equipment'] for a in final['actors']],[a['equipment'] for a in g['actors']])
        self.assertTrue(all(not a['active'] for a in final['actors'][1:]))
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        for name in ('maps','actors','seed','tasks_done'):self.assertEqual(again[name],final[name],name)

    def test_dead_city_restores_surface_before_successor(self):
        g=self.dead_game();g.update(world_id=2,position=245,city_overlay=1,city_backup_position=1500)
        p,final=self.launch(g,'\rNEXT\rF333FIVEHAOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['world_id'],11);self.assertEqual(final['position'],1000)
        self.assertEqual(final['maps'],g['maps']);self.assertEqual(final['city_backup_position'],1500)

    def test_interrupted_successor_never_exports(self):
        for text in ('','\rNEXT','\rNEXT\rH'):
            p,final=self.launch(self.dead_game(),text)
            self.assertNotEqual(p.returncode,0);self.assertIsNone(final)

    def test_dead_with_all_objectives_exits_after_condolences(self):
        g=self.dead_game();g['tasks_done']=[1]*4
        p,final=self.launch(g,'\r');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['ending_complete'],1);self.assertEqual(final['ending_victory'],0)
        self.assertEqual(final['party_available'],0)
        self.assertEqual(final['actors'],g['actors']);self.assertNotEqual(final['seed'],g['seed'])


if __name__=='__main__':unittest.main()
