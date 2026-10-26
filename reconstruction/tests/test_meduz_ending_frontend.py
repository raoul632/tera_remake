"""Parcours complets pyramide/Meduz et fin, via les executables C reels."""
import copy
import subprocess
import unittest
import test_commerce_seth_frontend
from test_navigation_frontend import create_generated_game,HERE
from creation import apply_snapshot
from meduz import pack_meduz


def operation(game,key=0,tape='',enter=False):
    """Utiliser les lectures natives pour preparer un parcours deterministe."""
    p=subprocess.run([str(HERE/'build/cmake/tera_meduz_bridge.exe')],input=pack_meduz(game,key,enter=enter)+
        ''.join(f'{ord(k)}\n' for k in tape),text=True,capture_output=True,timeout=20)
    if p.returncode:raise AssertionError((p.returncode,p.stderr,p.stdout[-600:]))
    count=-1
    for line in p.stdout.splitlines():
        tag,*v=line.split();v=list(map(int,v))
        if tag=='S':apply_snapshot(game,v);count+=1
        elif tag=='V':
            game.update(mental_extension=v[3],tasks_counter=v[4]);game['tasks_done'][0]=v[5]
    return tape[:count]


class MeduzEndingFrontendTests(unittest.TestCase):
    launch=test_commerce_seth_frontend.CommerceSethFrontendTests.launch

    def pyramid(self):
        g=create_generated_game();g.update(position=g['towns'][3],town_id=3,town_found=1,sound=0)
        return g

    def room(self,content=2):
        g=self.pyramid();g.update(world_id=10,position=1000,previous_position=999,meduz_backup_position=g['position'])
        g['maps'][1000]=[0,0,1,0,content,0,1];g['maps'][999]=[0,0,0,0,0,0,1]
        return g

    def test_pyramid_confirmation_wrong_word_and_interruption(self):
        g=self.pyramid()
        for tape in ('MNQ','MOWRONG\rQ'):
            p,final=self.launch(g,tape);self.assertEqual(p.returncode,0,p.stderr)
            for field in ('seed','actors','money','days','position','tasks_done'):self.assertEqual(final[field],g[field],field)
        p,final=self.launch(g,'MO');self.assertNotEqual(p.returncode,0);self.assertIsNone(final)

    def test_pyramid_travel_arrival_export_return_and_reload(self):
        g=self.pyramid();g['seed']=0 # Banc sans rencontre a l'arrivee, pour tester l'aller-retour.
        g['maps'][1000]=[0,0,1,0,0,0,1];g['maps'][999]=[0,0,0,0,0,0,1]
        expected=copy.deepcopy(g);tape=operation(expected,ord('M'),'O'+g['secret_words'][1]+'\r'+'FO'*100,True)
        p,final=self.launch(g,'M'+tape+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['world_id'],10)
        for field in ('seed','actors','days','position'):self.assertEqual(final[field],expected[field],field)
        final['maps'][1000][2]=1;expected=copy.deepcopy(final)
        back=operation(expected,ord('2'),'FO'*100)
        p,again=self.launch(final,'2'+back+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['world_id'],11);self.assertEqual(again['position'],g['position'])
        for field in ('seed','actors','days'):self.assertEqual(again[field],expected[field],field)
        p,reloaded=self.launch(again,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(reloaded['seed'],again['seed'])

    def test_meditation_once_active_only_and_byte_wrap(self):
        g=self.room();g['actors'][0]['stats'][2]=255;g['actors'][0]['stats'][5]=255
        before=copy.deepcopy(g)
        p,final=self.launch(g,'MOM\rQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['mental_extension'],1)
        self.assertEqual(final['actors'][0]['stats'][2],0);self.assertEqual(final['actors'][0]['stats'][5],0)
        self.assertEqual(final['actors'][1:],before['actors'][1:])
        for field in ('days','seed','money'):self.assertEqual(final[field],before[field],field)

    def test_meditation_refusal_and_word_without_extension(self):
        g=self.room();p,final=self.launch(g,'MNQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['mental_extension'],0);self.assertEqual(final['actors'],g['actors'])
        g=self.room(3);p,final=self.launch(g,'IO'+g['secret_words'][2]+'\rQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['tasks_done'][0],0);self.assertEqual(final['tasks_counter'],g['tasks_counter'])

    def test_invocation_objective_persists_and_counts_once(self):
        g=self.room(3);g['mental_extension']=1
        text='IO'+g['secret_words'][2]+'\r'
        p,final=self.launch(g,text+text+'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['tasks_done'][0],1);self.assertEqual(final['tasks_counter'],g['tasks_counter']+1)
        self.assertIn('Union mentale accomplie',p.stdout)
        p,again=self.launch(final,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['tasks_done'],final['tasks_done']);self.assertEqual(again['seed'],final['seed'])

    def test_last_objective_waits_for_meduz_exit_before_ending(self):
        g=self.room(3);g.update(mental_extension=1,tasks_done=[0,1,1,1],tasks_counter=3)
        p,final=self.launch(g,'IO'+g['secret_words'][2]+'\rQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['tasks_done'],[1]*4);self.assertEqual(final['world_id'],10)
        self.assertFalse(final.get('ending_complete',0));self.assertNotIn('SCORE ->',p.stdout)
        expected=copy.deepcopy(final);back=operation(expected,ord('2'),'FO'*100)
        p,finished=self.launch(final,'2'+back+'X');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(finished['ending_complete'],1);self.assertEqual(finished['ending_victory'],1)
        self.assertEqual(finished['world_id'],11);self.assertIn('SCORE ->',p.stdout)

    def test_ending_score_ack_and_reload_does_not_replay_music(self):
        g=self.pyramid();g.update(tasks_done=[1]*4,experience=12345,days=7300,sound=1)
        p,final=self.launch(g,'X');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['ending_complete'],1);self.assertEqual(final['ending_error'],0)
        self.assertEqual(len(final['score_real48']),6);self.assertIn('Felicitations',p.stdout)
        p,again=self.launch(final,'');self.assertEqual(p.returncode,0,p.stderr)
        for field in ('seed','score','score_text','days','actors'):self.assertEqual(again[field],final[field],field)

    def test_ending_interrupts_without_export_and_zero_division(self):
        g=self.pyramid();g['tasks_done']=[1]*4
        p,final=self.launch(g,'');self.assertNotEqual(p.returncode,0);self.assertIsNone(final)
        g['days']=4000;p,final=self.launch(g,'');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['ending_error'],2);self.assertEqual(final['ending_complete'],1)
        self.assertEqual(final['seed'],g['seed']);self.assertEqual(final['score_real48'],[0]*6)

    def test_all_tasks_dead_has_condolences_and_no_successor_or_score(self):
        g=self.pyramid();g.update(tasks_done=[1]*4,party_available=0)
        p,final=self.launch(g,'X');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['ending_complete'],1);self.assertEqual(final['ending_victory'],0)
        self.assertEqual(final['actors'],g['actors']);self.assertEqual(final['score_text'],'none')
        self.assertNotIn('SCORE ->',p.stdout)

    def test_mortal_arrival_waits_for_command_then_returns_and_recreates(self):
        g=self.pyramid();g['seed']=0;g['maps'][1000]=[0,0,1,0,0,0,1]
        for a in g['actors']:a.update(active=0,hp=0,condition='HS')
        g['actors'][0]['active']=1
        p,final=self.launch(g,'MO'+g['secret_words'][1]+'\rQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['world_id'],10);self.assertEqual(final['meduz_entry_pending'],1)
        self.assertEqual(final['party_available'],0)
        p,again=self.launch(final,'?\rNEXT\rH333FIVEHAOQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['world_id'],11);self.assertEqual(again['actors'][0]['name'],'NEXT')
        self.assertEqual(again['party_available'],1)

    def test_all_tasks_dead_inside_meduz_exits_before_final_condolences(self):
        g=self.room(0);g.update(tasks_done=[1]*4,party_available=0,meduz_entry_pending=1)
        p,final=self.launch(g,'Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['world_id'],10);self.assertFalse(final.get('ending_complete',0))
        p,again=self.launch(final,'?X');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(again['world_id'],11);self.assertEqual(again['days'],g['days']+7)
        self.assertEqual(again['ending_complete'],1);self.assertEqual(again['ending_victory'],0)
        self.assertEqual(again['actors'],g['actors'])

    def test_generic_invocation_reads_word_and_interruption_has_no_export(self):
        g=self.room(0);p,final=self.launch(g,'IWRONG\rQ');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['seed'],g['seed']);self.assertEqual(final['tasks_done'],g['tasks_done'])
        p,final=self.launch(g,'IWR');self.assertNotEqual(p.returncode,0);self.assertIsNone(final)

    def test_blocked_meduz_step_still_refreshes_day(self):
        g=self.room(0);g.update(force_day=1);g['maps'][1000][0]=0
        p,final=self.launch(g,'8Q');self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(final['position'],1000);self.assertGreater(final['days'],g['days'])


if __name__=='__main__':unittest.main()
