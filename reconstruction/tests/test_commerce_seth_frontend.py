"""Parcours complets du terminal : Kronopolis et temple de Seth."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from test_navigation_frontend import create_generated_game,run_bridge,HERE


class CommerceSethFrontendTests(unittest.TestCase):
    def launch(self,g,text):
        with tempfile.TemporaryDirectory(dir=HERE/'build') as d:
            source=Path(d)/'input.json';output=Path(d)/'output.json'
            source.write_text(json.dumps(g),encoding='utf-8');original=source.read_bytes()
            p=subprocess.run([sys.executable,str(HERE/'frontend/exploration.py'),'--state',str(source),
                '--output',str(output)],input=text,text=True,capture_output=True,timeout=30)
            self.assertEqual(source.read_bytes(),original)
            final=json.loads(output.read_text(encoding='utf-8')) if output.exists() else None
        return p,final

    def place(self,town):
        g=create_generated_game();g.update(position=g['towns'][town],town_id=town,town_found=1,money=2000)
        return g

    def test_buy_sell_exit_reload(self):
        g=self.place(0);p,final=self.launch(g,'EOAOAG\x1bVOA0\x1b\x1bQ')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('Achat effectue',p.stdout);self.assertIn('Vente :',p.stdout)
        self.assertEqual(final['actors'][0]['equipment'][0],0);self.assertLess(final['money'],g['money'])
        for field in ('maps','days','position','tasks_done'):self.assertEqual(final[field],g[field],field)
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        for field in ('actors','seed','money','maps'):self.assertEqual(again[field],final[field],field)

    def test_special_goods_update_energy_maps_key_without_slots(self):
        g=self.place(0);g['actors'][0]['equipment']=[3]*10
        p,final=self.launch(g,'EOAOADEFEL\x1b\x1bQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['actors'][0]['equipment'],[3]*10)
        self.assertEqual(final['energy'],g['energy']+50)
        self.assertEqual(final['has_map'][9:],[1,1]);self.assertEqual(final['keys'][9],9)

    def test_hospital_cures_active_only_but_restores_everyone(self):
        g=self.place(0);g['actors'][0].update(hp=1,condition='EM')
        g['actors'][3].update(active=0,hp=0,magic=0,condition='HS')
        p,final=self.launch(g,'EOHO\x1bQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['actors'][0]['condition'],'OK')
        self.assertEqual(final['actors'][3]['condition'],'HS');self.assertGreater(final['actors'][3]['hp'],0)
        self.assertGreater(final['days'],g['days']);self.assertEqual(final['money'],g['money'])

    def test_entry_cancellation_and_interrupted_purchase(self):
        g=self.place(0);p,final=self.launch(g,'ENQ');self.assertEqual(p.returncode,0,p.stderr)
        for field in ('seed','actors','money','position'):self.assertEqual(final[field],g[field],field)
        p,final=self.launch(g,'EOAOA');self.assertNotEqual(p.returncode,0);self.assertIsNone(final)

    def test_seth_entry_read_exit_and_reload(self):
        g=self.place(4);g['maps'][800]=[0,0,1,0,0,0,1]
        p,final=self.launch(g,'EOLO2Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('lieux sacres',p.stdout);self.assertEqual(final['world_id'],11)
        self.assertEqual(final['position'],g['position']);self.assertEqual(final['city_backup_position'],g['position'])
        self.assertEqual(final['maps'],g['maps']);self.assertNotEqual(final['seed'],g['seed'])
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['seed'],final['seed'])

    def room(self):
        g=self.place(4);g.update(world_id=8,position=845,previous_position=844,city_overlay=2,
            city_backup_position=g['position'],chest_open=0,sound=0)
        g['maps'][845]=[0,0,0,0,18,0,1];g['actors'][0]['equipment']=[0]*10
        return g

    def test_seth_open_take_and_resume(self):
        g=self.room();g['actors'][0].update(stats=[0]+[10]*9,skills=[0]+[3]*9)
        p,final=self.launch(g,'OOAPOAQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['maps'][845][4],0);self.assertEqual(final['actors'][0]['equipment'][0],18)
        self.assertEqual(final['city_overlay'],2)
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['actors'],final['actors'])

    def test_mortal_entry_keeps_original_mandatory_command_on_reload(self):
        g=self.place(4);g['maps'][800]=[0,0,1,0,0,0,1]
        for a in g['actors']:a.update(active=0,hp=0,condition='HS')
        g['actors'][0]['active']=1
        p,final=self.launch(g,'EOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['world_id'],8);self.assertEqual(final['party_available'],0)
        self.assertEqual(final['temple_entry_pending'],1)
        p,again=self.launch(final,'?\rNEXT\rH333FIVEHAOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['world_id'],11);self.assertEqual(again['actors'][0]['name'],'NEXT')
        self.assertEqual(again['party_available'],1)

    def test_seth_invocation_completes_fourth_flag_once(self):
        g=self.room();g['maps'][845][4]=110;g['secret_words'][3]='TEST';g['secret_word']='TEST'
        g['actors'][0].update(hp=10000,magic=200,stats=[0]+[10]*9,skills=[0]+[3]*9)
        action='OTEST\r'+'COTEST\r'*20
        probe=run_bridge(copy.deepcopy(g),ord('I'),action)
        self.assertEqual(probe.returncode,0,probe.stderr)
        consumed=sum(line.startswith('S ') for line in probe.stdout.splitlines())-1
        p,final=self.launch(g,'I'+action[:consumed]+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['tasks_done'][3],1);self.assertEqual(final['tasks_counter'],g['tasks_counter']+1)
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['tasks_done'],final['tasks_done']);self.assertEqual(again['tasks_counter'],final['tasks_counter'])


if __name__=='__main__':unittest.main()
