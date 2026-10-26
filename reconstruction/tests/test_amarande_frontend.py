"""Parcours des lieux secondaires et des commandes residentes d'Amarande."""
import contextlib
import copy
import io
import subprocess
import unittest
import test_commerce_seth_frontend
from test_navigation_frontend import create_generated_game,HERE
from amarande import pack_amarande,show_map,TOWNS
from commerce import present_place
from creation import apply_snapshot
from test_navigation_frontend import run_bridge


def probe(game,key,tape):
    """Lire la longueur et l'etat d'une operation C pour borner la saisie UI.

    Le test ne recalcule ni combat ni RANDOM en Python. La comparaison avec
    le COM est assuree separement par compare_amarande_binary.py.
    """
    p=subprocess.run([str(HERE/'build/cmake/tera_amarande_bridge.exe')],
        input=pack_amarande(game,ord(key))+''.join(f'{ord(c)}\n' for c in tape),
        text=True,capture_output=True,timeout=15)
    if p.returncode:raise AssertionError((p.returncode,p.stderr,p.stdout[-600:]))
    consumed=-1
    for line in p.stdout.splitlines():
        tag,*values=line.split();v=list(map(int,values))
        if tag=='S':apply_snapshot(game,v);consumed+=1
        elif tag=='Y':
            game.update(sound=v[1],fast_moves=v[2],information_context=v[3],
                encounter_suppressed=v[4],alfol_status_actor=v[16])
    return tape[:consumed]


class AmarandeFrontendTests(unittest.TestCase):
    launch=test_commerce_seth_frontend.CommerceSethFrontendTests.launch

    def place(self,town=5):
        g=create_generated_game();g.update(position=g['towns'][town],town_found=1,
            town_id=town,sound=0,alfol_status_actor=5)
        return g

    def assert_state(self,actual,expected):
        for field in ('actors','seed','days','money','position','tasks_done'):
            self.assertEqual(actual[field],expected[field],field)

    def test_facades_and_original_npc_presence_predicate(self):
        for town,npc in ((5,3),(6,8),(7,5),(8,8),(9,3)):
            for active,condition,visible in ((0,'OK',1),(1,'OK',0),(0,'HS',0)):
                with self.subTest(town=town,active=active,condition=condition):
                    g=self.place(town);g.update(moved=1,information_context=0)
                    g['locations'][npc]=g['position'];g['actors'][npc].update(active=active,condition=condition)
                    before=copy.deepcopy(g);present_place(g)
                    self.assertEqual(g['information_context'],visible)
                    for field in ('actors','days','maps','position'):self.assertEqual(g[field],before[field])
                    # La branche OR originale montre encore le PNJ ailleurs.
                    g.update(moved=1,information_context=0);g['locations'][npc]=0
                    present_place(g);self.assertEqual(g['information_context'],1)

    def test_secondary_places_keep_generic_enter_and_original_reply(self):
        for town in range(5,10):
            with self.subTest(town=TOWNS[town]):
                g=self.place(town);expected=copy.deepcopy(g);probe(expected,'E','')
                p,final=self.launch(g,'EQ');self.assertEqual(p.returncode,0,p.stderr)
                self.assertEqual(final['world_id'],11);self.assert_state(final,expected)
                self.assertNotIn('Commande non portee',p.stdout)

    def test_arrival_facade_enables_original_secondary_hints(self):
        g=self.place(5);g.update(arrival_pending=1,information_context=0)
        g['extra_locations']=[0]*10;g['extra_locations'][3]=g['position']
        g['locations'][3]=0
        p,final=self.launch(g,'NIOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['information_context'],1)
        self.assertIn(g['hints'][4],p.stdout);self.assertIn(g['hints'][5],p.stdout)
        self.assertEqual(final['position'],g['position']);self.assertEqual(final['world_id'],11)

    def test_city_return_replays_facade_without_an_extra_day(self):
        g=self.place(2);g['maps'][1][2]=1;g['maps'][1][4:6]=[0,0]
        expected=copy.deepcopy(g)
        self.assertEqual(run_bridge(expected,ord('E'),'O',enter=True).returncode,0)
        self.assertEqual(run_bridge(expected,ord('2'),'').returncode,0)
        self.assertEqual(run_bridge(expected,0,'',leave=True).returncode,0)
        before=expected['seed'];present_place(expected,force=True)
        self.assertNotEqual(expected['seed'],before)
        p,final=self.launch(g,'EO2Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assert_state(final,expected)

    def test_panel_signed_coordinates_cancel_and_reload(self):
        g=self.place();g.update(position=1450,town_id=10);g['towns'][0]=1419
        p,final=self.launch(g,'LOXQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('1 O 1 S de Kronopolis',p.stdout);self.assertEqual(final['alfol_status_actor'],-1)
        self.assert_state(final,g)
        p,again=self.launch(final,'LNQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assert_state(again,final)
        p,missing=self.launch(g,'LO');self.assertNotEqual(p.returncode,0);self.assertIsNone(missing)

    def test_halt_fast_moves_yes_then_no_and_sound_toggle(self):
        g=self.place();p,final=self.launch(g,'HAO#OQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['fast_moves'],1);self.assertEqual(final['sound'],1)
        p,again=self.launch(final,'HAN#NQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['fast_moves'],0);self.assertEqual(again['sound'],1)
        self.assertEqual(again['days'],g['days'])

    def test_map_is_local_and_uses_initial_flag_alias(self):
        g=self.place();output=io.StringIO()
        with contextlib.redirect_stdout(output):show_map(g)
        lines=output.getvalue().splitlines()[1:]
        self.assertEqual(len(lines),30);self.assertTrue(all(len(row)==10 for row in lines))
        self.assertEqual(lines[15][4],'@')
        p,final=self.launch(g,'HCQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('Carte locale',p.stdout);self.assertEqual(final['alfol_status_actor'],9)
        g['initial_flag']=0
        p,final=self.launch(g,'HCQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('Pas de carte disponible',p.stdout);self.assertEqual(final['alfol_status_actor'],5)

    def test_forced_rest_and_companion_inspection_follow_c(self):
        for command,tape in (('H','DO'+'FO'*100),('H','MO'+'FO'*100),('H','SO'+'X'*10)):
            with self.subTest(tape=tape[:2]):
                g=self.place();g['actors'][0].update(stats=[0]+[10]*9,skills=[0]+[3]*9)
                g['actors'][5]['active']=1;expected=copy.deepcopy(g)
                consumed=probe(expected,command,tape)
                p,final=self.launch(g,command+consumed+'Q');self.assertEqual(p.returncode,0,p.stderr)
                self.assert_state(final,expected)
                self.assertEqual(final['alfol_status_actor'],expected['alfol_status_actor'])

    def test_confirmed_halt_exit_exports_and_interruption_does_not(self):
        g=self.place();p,final=self.launch(g,'H\x1bO');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['quit_requested'],1)
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assert_state(again,final)
        p,missing=self.launch(g,'H\x1b');self.assertNotEqual(p.returncode,0);self.assertIsNone(missing)

    def test_resident_combat_victory_marks_npc_hs_and_persists(self):
        g=self.place();g['locations'][4]=g['position'];g['extra_locations']=[0]*10
        g['actors'][0].update(hp=10000,magic=200,stats=[0]+[10]*9,skills=[0]+[3]*9,
            equipment=[3,6,9,12,15,18,21,0,0,0])
        expected=copy.deepcopy(g);tape=probe(expected,'C','O'+'TO'*200)
        p,final=self.launch(g,'C'+tape+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['actors'][4]['condition'],'HS');self.assert_state(final,expected)
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assert_state(again,final)

    def test_invocation_word_and_u_keep_original_reads(self):
        g=self.place();p,final=self.launch(g,'IARIOCH\rQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['invocation_word'],'ARIOCH');self.assertEqual(final['alfol_status_actor'],9)
        # Dans un lieu, ARIOCH n'est pas invoque et ne consomme aucun tirage.
        self.assert_state(final,g)
        expected=copy.deepcopy(g);tape=probe(expected,'U','OXY')
        self.assertEqual(tape,'OXY')
        p,final=self.launch(g,'U'+tape+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assert_state(final,expected)


if __name__=='__main__':unittest.main()
