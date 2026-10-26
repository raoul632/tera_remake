"""Parcours terminal et reprise des objets, avec monde/heros generes par le C."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from test_navigation_frontend import create_generated_game,HERE


class InventoryChestFrontendTests(unittest.TestCase):
    def game(self):
        g=create_generated_game();g.update(world_id=2,position=245,previous_position=244,
            city_overlay=1,city_backup_position=1500,chest_open=0,fast_moves=1)
        g['maps'][245][4:]=[18,0,1]
        g['actors'][0].update(stats=[0]+[10]*9,skills=[0]+[3]*9,equipment=[0]*10,hp=30,condition='OK')
        return g

    def launch(self,source,output,text):
        return subprocess.run([sys.executable,str(HERE/'frontend/exploration.py'),
            '--state',str(source),'--output',str(output)],input=text,text=True,capture_output=True,timeout=30)

    def test_open_take_export_and_reload(self):
        game=self.game()
        with tempfile.TemporaryDirectory(dir=HERE/'build') as d:
            source=Path(d)/'game.json';out=Path(d)/'next.json';again=Path(d)/'again.json'
            source.write_text(json.dumps(game),encoding='utf-8');raw=source.read_bytes()
            p=self.launch(source,out,'OOAPOAQ');self.assertEqual(p.returncode,0,p.stderr)
            final=json.loads(out.read_text(encoding='utf-8'))
            self.assertEqual(source.read_bytes(),raw)
            self.assertEqual(final['maps'][245][4],0)
            self.assertEqual(final['actors'][0]['equipment'][0],18)
            p=self.launch(out,again,'Q');self.assertEqual(p.returncode,0,p.stderr)
            restored=json.loads(again.read_text(encoding='utf-8'))
            for field in ('maps','actors','seed','keys','has_map','chest_open'):
                self.assertEqual(restored[field],final[field],field)

    def test_movement_closes_local_open_flag_without_removing_content(self):
        game=self.game();game['maps'][245][0]=1;game['maps'][246][4:]=[0,0,1]
        with tempfile.TemporaryDirectory(dir=HERE/'build') as d:
            source=Path(d)/'game.json';out=Path(d)/'next.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            p=self.launch(source,out,'OOA8Q');self.assertEqual(p.returncode,0,p.stderr)
            final=json.loads(out.read_text(encoding='utf-8'))
        self.assertEqual(final['position'],246);self.assertEqual(final['chest_open'],0)
        self.assertEqual(final['maps'][245][4],18)

    def test_full_recipient_keeps_chest_then_drop_and_take(self):
        game=self.game();game['actors'][5].update(active=1,condition='OK',equipment=[3]*10)
        with tempfile.TemporaryDirectory(dir=HERE/'build') as d:
            source=Path(d)/'game.json';out=Path(d)/'next.json';again=Path(d)/'again.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            p=self.launch(source,out,'OOAPOBQ');self.assertEqual(p.returncode,0,p.stderr)
            final=json.loads(out.read_text(encoding='utf-8'))
            self.assertEqual(final['maps'][245][4],18);self.assertEqual(final['actors'][5]['equipment'],[3]*10)
            p=self.launch(out,again,'AOB0POBQ');self.assertEqual(p.returncode,0,p.stderr)
            restored=json.loads(again.read_text(encoding='utf-8'))
        self.assertEqual(restored['maps'][245][4],0)
        self.assertEqual(restored['actors'][5]['equipment'],[18]+[3]*9)

    def test_two_potions_have_distinct_effects_after_reload(self):
        game=self.game();game['actors'][0].update(hp=1,magic=0,condition='EM',equipment=[23,24]+[0]*8)
        with tempfile.TemporaryDirectory(dir=HERE/'build') as d:
            source=Path(d)/'game.json';out=Path(d)/'next.json';again=Path(d)/'again.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            p=self.launch(source,out,'BA0Q');self.assertEqual(p.returncode,0,p.stderr)
            final=json.loads(out.read_text(encoding='utf-8'));a=final['actors'][0]
            self.assertEqual(a['equipment'][0],0);self.assertEqual(a['condition'],'EM')
            self.assertGreater(a['hp'],1);self.assertGreater(a['magic'],0)
            p=self.launch(out,again,'BA1Q');self.assertEqual(p.returncode,0,p.stderr)
            restored=json.loads(again.read_text(encoding='utf-8'))['actors'][0]
        self.assertEqual(restored['condition'],'OK');self.assertEqual(restored['equipment'][1],0)
        self.assertEqual(restored['hp'],a['hp'])

    def test_transfer_failure_then_success_preserves_source(self):
        game=self.game();game['actors'][0]['equipment'][0]=18
        game['actors'][5].update(active=1,condition='OK',equipment=[3]*10)
        with tempfile.TemporaryDirectory(dir=HERE/'build') as d:
            source=Path(d)/'game.json';out=Path(d)/'next.json';again=Path(d)/'again.json'
            source.write_text(json.dumps(game),encoding='utf-8')
            p=self.launch(source,out,'NDOA0BQ');self.assertEqual(p.returncode,0,p.stderr)
            final=json.loads(out.read_text(encoding='utf-8'))
            self.assertEqual(final['actors'][0]['equipment'][0],18)
            p=self.launch(out,again,'AOB0NDOA0BQ');self.assertEqual(p.returncode,0,p.stderr)
            restored=json.loads(again.read_text(encoding='utf-8'))
        self.assertEqual(restored['actors'][0]['equipment'][0],0)
        self.assertEqual(restored['actors'][5]['equipment'][0],18)

    def test_digit_reader_escape_is_not_an_export_or_cancellation(self):
        game=self.game()
        with tempfile.TemporaryDirectory(dir=HERE/'build') as d:
            source=Path(d)/'game.json';out=Path(d)/'next.json'
            source.write_text(json.dumps(game),encoding='utf-8');raw=source.read_bytes()
            p=self.launch(source,out,'AOA\x1b')
            self.assertNotEqual(p.returncode,0);self.assertFalse(out.exists())
            self.assertEqual(source.read_bytes(),raw)


if __name__=='__main__':unittest.main()
