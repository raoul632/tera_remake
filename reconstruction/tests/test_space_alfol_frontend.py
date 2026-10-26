"""Parcours terminal : depart, retour, media, portes, radiation et interruption."""
import copy
import subprocess
import sys
import unittest
import test_commerce_seth_frontend
from test_navigation_frontend import create_generated_game,HERE
from space import pack_space
from alfol import pack_alfol
from creation import apply_snapshot
from commerce import present_place


def probe(game,bridge,packet,tape):
    """Obtenir la longueur d'une operation du vrai moteur, sans regle Python."""
    p=subprocess.run([str(HERE/'build/cmake'/f'tera_{bridge}_bridge.exe')],
        input=packet+''.join(f'{ord(k)}\n' for k in tape),text=True,capture_output=True,timeout=15)
    if p.returncode:raise AssertionError((p.returncode,p.stderr,p.stdout[-800:]))
    count=-1
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':apply_snapshot(game,v);count+=1
        elif tag=='X':
            game.update(space_pirates=v[1],space_backup_position=v[3],on_alfol=v[11],space_phase=v[0])
    return tape[:count]


class SpaceAlfolFrontendTests(unittest.TestCase):
    launch=test_commerce_seth_frontend.CommerceSethFrontendTests.launch

    def port(self):
        g=create_generated_game();g.update(position=g['towns'][1],town_found=1,town_id=1,sound=0,energy=1000)
        # Profil de banc explicite : assurer un pilote pour tester le trajet.
        g['actors'][0].update(stats=[0]+[10]*9,skills=[0]+[3]*9)
        return g

    def surface(self,position=0):
        g=self.port();g.update(on_alfol=1,position=position,space_pirates=4,space_backup_position=g['position'])
        return g

    def test_outward_arrival_export_and_return(self):
        g=self.port();expected=copy.deepcopy(g)
        out=probe(expected,'space',pack_space(expected),'ON'+'0'*100)
        arrival=probe(expected,'alfol',pack_alfol(expected,0,True),'FO'*50)
        p,final=self.launch(g,'E'+out+arrival+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['on_alfol'],1);self.assertEqual(final['position'],0)
        for field in ('seed','energy','money','days','actors'):self.assertEqual(final[field],expected[field],field)
        expected=copy.deepcopy(final)
        back=probe(expected,'space',pack_space(expected,2),'N'+'0'*100)
        # TERAMAIN recharge OVL0148 apres le retour, meme sans deplacement.
        present_place(expected,force=True)
        # Le refus de rembarquer au point zero sort quand meme de l'overlay.
        p,again=self.launch(final,'RN'+back+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['on_alfol'],0);self.assertEqual(again['position'],g['position'])
        for field in ('seed','energy','money','days','actors'):self.assertEqual(again[field],expected[field],field)

    def test_cancel_departure_and_no_pilot(self):
        g=self.port();p,final=self.launch(g,'ENQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['on_alfol'],0);self.assertEqual(final['position'],g['position'])
        self.assertNotEqual(final['seed'],g['seed'])
        g['actors'][0].update(condition='HS')
        p,final=self.launch(g,'EOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['space_departed'],0);self.assertEqual(final['on_alfol'],0)

    def test_destroyed_ship_stays_halted_on_reload(self):
        g=self.surface();g['energy']=5
        p,final=self.launch(g,'RON0');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['program_halted'],1);self.assertIn('Vaisseau detruit',p.stdout)
        p,again=self.launch(final,'');self.assertEqual(p.returncode,0,p.stderr)
        for field in ('seed','actors','energy','days','on_alfol'):self.assertEqual(again[field],final[field],field)

    def test_interrupted_flight_has_no_export(self):
        g=self.port();p,final=self.launch(g,'EON');self.assertNotEqual(p.returncode,0);self.assertIsNone(final)

    def test_surface_wrap_and_reboard_away_from_landing(self):
        g=self.surface();p,final=self.launch(g,'2RNQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['position'],99);self.assertEqual(final['on_alfol'],1)
        self.assertEqual(final['energy'],g['energy'])

    def room(self,content):
        g=self.surface(944);g.update(world_id=9,previous_position=943,money=3000)
        g['maps'][944]=[1,0,1,0,0,0,1];g['maps'][945]=[0,0,1,0,content,0,1]
        return g

    def test_media_payment_objective_once_but_repeat_cost(self):
        g=self.room(1);p,final=self.launch(g,'8OQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['tasks_done'][1],1);self.assertEqual(final['tasks_counter'],g['tasks_counter']+1)
        self.assertEqual(final['money'],2000);self.assertIn('Objectif des medias',p.stdout)
        p,again=self.launch(final,'22OQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['money'],1000);self.assertEqual(again['tasks_counter'],final['tasks_counter'])

    def test_media_enter_does_not_pay_and_eof_does_not_export(self):
        g=self.room(1);p,final=self.launch(g,'8\rQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['money'],3000);self.assertEqual(final['tasks_done'][1],0)
        p,final=self.launch(g,'8');self.assertNotEqual(p.returncode,0);self.assertIsNone(final)

    def test_radiation_and_door_persist_after_reload(self):
        g=self.room(2);g['maps'][944][0]=3;g['keys'][9]=9
        p,final=self.launch(g,'8UQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['maps'][944][0],2);self.assertEqual(final['position'],945)
        self.assertLess(final['actors'][0]['hp'],g['actors'][0]['hp'])
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        for field in ('seed','actors','maps','world_id','position','on_alfol'):self.assertEqual(again[field],final[field],field)

    def test_building_entry_and_exit_returns_to_alfol(self):
        g=self.surface();g['position']=g['alfol_position'];g['maps'][900]=[0,0,1,0,0,0,1]
        p,final=self.launch(g,'EOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['world_id'],9);self.assertEqual(final['position'],900)
        p,again=self.launch(final,'2Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['world_id'],11);self.assertEqual(again['position'],g['alfol_position'])
        self.assertEqual(again['on_alfol'],1)

    def test_mortal_entry_keeps_first_command_then_creates_successor(self):
        g=self.surface();g['position']=g['alfol_position'];g['maps'][900]=[0,0,1,0,0,0,1]
        for a in g['actors']:a.update(active=0,hp=0,condition='HS')
        g['actors'][0]['active']=1
        p,final=self.launch(g,'EOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['world_id'],9);self.assertEqual(final['party_available'],0)
        self.assertEqual(final['alfol_entry_pending'],1)
        p,again=self.launch(final,'?\rNEXT\rH333FIVEHAOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['world_id'],11);self.assertEqual(again['on_alfol'],0)
        self.assertEqual(again['actors'][0]['name'],'NEXT');self.assertEqual(again['party_available'],1)

    def test_fixed_monster_victory_clears_map_and_keeps_alfol(self):
        g=self.room(0);g['maps'][945][5]=1
        g['actors'][0].update(hp=10000,magic=200,equipment=[3,6,9,12,15,18,21,0,0,0])
        expected=copy.deepcopy(g);tape=probe(expected,'alfol',pack_alfol(expected,ord('8')),'TO'*100)
        p,final=self.launch(g,'8'+tape+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['maps'][945][5],0);self.assertEqual(final['on_alfol'],1)
        self.assertEqual(final['seed'],expected['seed']);self.assertEqual(final['actors'],expected['actors'])

    def test_original_radiation_alias_changes_hint_and_survives_reload(self):
        g=self.room(2);g['alfol_status_actor']=16;g['actors'][6]['magic']=-1
        g['hints'][0]='0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'
        p,final=self.launch(g,'8Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['actors'][6]['magic'],0)
        self.assertEqual(final['hints'][0][17:20],'\x02HS')
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['hints'],final['hints']);self.assertEqual(again['alfol_hint_bytes'],final['alfol_hint_bytes'])

    def test_companion_drop_preserves_original_global_for_radiation(self):
        g=self.room(2);g['actors'][5]['active']=1
        p,final=self.launch(g,'ACBQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['alfol_status_actor'],5)
        p,again=self.launch(final,'ITEST\rQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['alfol_status_actor'],9)


if __name__=='__main__':unittest.main()
