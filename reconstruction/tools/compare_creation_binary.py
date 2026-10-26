"""Comparer la creation C aux instructions COM + overlays 0010/0022.

Oracle x86-16 execute : aucun jet ni caracteristique n'est calcule en
Python. OVL0010 compare les frontieres personnages, explicitement sans
son calendrier geographique intermediaire. OVL0022 est execute entier,
menus compris, avec sorties video/son abstraites et touches imposees.
"""
import hashlib
import json
import random
import struct
import subprocess
import sys
import time
from compare_combat_binary import OriginalRoutine, HERE, ROOT, SOURCE, EXPECTED_HASH, STACK, STOP, word
from unicorn.x86_const import UC_X86_REG_DS, UC_X86_REG_AX, UC_X86_REG_SP, UC_X86_REG_IP
sys.path.insert(0, str(HERE/'frontend'))
from creation import empty_roster, pack_roster

DATA = 0x10000
OVERLAYS = ROOT/'data/compiled program/TERA.000'
OVERLAY_HASH = '539d95781f9296d40ecd05698eb98255835a45cb03e60b49cf581e3ada7038f4'


class CreationRoutine(OriginalRoutine):
    def __init__(self, image, overlays, game, keys):
        super().__init__(image)
        self.cpu.mem_map(DATA, 65536)
        self.cpu.reg_write(UC_X86_REG_DS, DATA >> 4)
        self.overlays = overlays
        self.game = game
        self.keys = list(keys)
        self.states = []
        self.halted = False
        self.draw_bounds = []
        self.cpu.mem_write(DATA+0x1FC, struct.pack('<I', game['seed']))
        for key, address in [('money',0x75D4),('experience',0x75D6),('days',0x75E2),
                              ('level',0x75D8),('dummy_level',0x75DA),('energy',0x75E6),
                              ('position',0x75D2),('world_id',0x9B76)]:
            self.data_word(address, game[key])
        for key, address in [('arrival_pending',0xA74F),('party_available',0xA745)]:
            self.cpu.mem_write(DATA+address, bytes([game[key]]))
        for id, value in enumerate(game['keys']):
            self.data_word(0x752C+2*id,value)
        for id, actor in enumerate(game['actors']):
            self.cpu.mem_write(DATA+0x9402+id, bytes([actor['active']]))
            self.data_string(0x94C8+10*id, actor['name'], 10)
            self.data_string(0x952C+3*id, actor['condition'], 3)
            for name, address in [('stats',0x7414),('skills',0x7478),('equipment',0x73B0)]:
                self.cpu.mem_write(DATA+address+10*id,bytes(actor[name]))
            for name, address in [('hp',0x74DC),('magic',0x74F0),('hp_limit',0x7504),('magic_limit',0x7518)]:
                self.data_word(address+2*id,actor[name])
        for group, names in enumerate(game['name_variants']):
            for id, name in enumerate(names):
                self.data_string(0xA67A+group*100+id*10,name,10)

    def data_word(self,address,value):
        self.cpu.mem_write(DATA+address,word(value))

    def data_string(self,address,value,size):
        raw = value.encode('cp437')
        self.cpu.mem_write(DATA+address,bytes([len(raw)])+raw.ljust(size-1,b'\0'))

    def state(self, phase):
        """Lire noms canoniques, tableaux, compteurs et graine du vrai binaire."""
        v = [phase,struct.unpack('<I',self.cpu.mem_read(DATA+0x1FC,4))[0]]
        v += [self.signed_word(DATA+a) for a in (0x75D4,0x75D6,0x75E2,0x75D8,0x75DA,0x75E6,0x75D2,0x9B76)]
        v += [self.cpu.mem_read(DATA+a,1)[0] for a in (0xA74F,0xA745)]
        v += [self.signed_word(DATA+0x752C+2*i) for i in range(10)]
        for id in range(10):
            v += [self.cpu.mem_read(DATA+0x9402+id,1)[0]]
            v += [self.signed_word(DATA+a+2*id) for a in (0x74DC,0x74F0,0x7504,0x7518)]
            raw = bytes(self.cpu.mem_read(DATA+0x952C+3*id,3))
            v += list(raw[1:1+raw[0]].ljust(2,b'\0'))
            raw = bytes(self.cpu.mem_read(DATA+0x94C8+10*id,10))
            v += list(raw[1:1+raw[0]].ljust(10,b'\0'))
            for address in (0x7414,0x7478,0x73B0):
                v += list(self.cpu.mem_read(DATA+address+10*id,10))
        return v

    def on_instruction(self,cpu,address,size,user_data):
        if address == 0x10DA:
            self.draw_bounds.append(cpu.reg_read(UC_X86_REG_AX)) # Aucune injection.
        elif address == 0x4277:
            sp = cpu.reg_read(UC_X86_REG_SP)
            caller = self.signed_word(sp) & 65535
            phase = {0x42D4:0,0xBCCE:1,0x4424:2,0x4456:2,0xC2C7:3,0xC436:4}.get(caller)
            if phase is None:
                raise RuntimeError(f'Clavier inconnu {caller:04X}')
            self.states.append(self.state(phase))
            if not self.keys:
                self.halted = True
                cpu.emu_stop()
            else:
                key = self.keys.pop(0)
                if 97 <= key <= 122: key -= 32
                cpu.reg_write(UC_X86_REG_AX,key)
                self.return_from_hook(1)
        elif address == 0xC549:
            self.states.append(self.state(5))
            self.halted = True
            cpu.emu_stop()
        elif address in (0x2558,0x1022,0x4503,0x0213,0x0226,0x02D9,0x03BB,0x03CF,0x2975):
            self.return_from_hook()
        elif address in (0x0371,0x037F):
            cpu.reg_write(UC_X86_REG_AX,1)
            self.return_from_hook()
        elif address == 0x038D:
            self.return_from_hook(6)
        elif address == 0x2926:
            sp = cpu.reg_read(UC_X86_REG_SP)
            self.return_from_hook(cpu.mem_read(sp+2,1)[0]+1)
        elif address in {0x4779:40,0x289D:2,0x28B9:2,0x0DAA:2,0x4873:2,0x49FD:2,0x4520:2,0x6179:8}:
            self.return_from_hook({0x4779:40,0x289D:2,0x28B9:2,0x0DAA:2,0x4873:2,0x49FD:2,0x4520:2,0x6179:8}[address])
        elif address == 0xBA2F: # Cadre texte de l'overlay 0022, pas la logique du heros.
            self.return_from_hook(48)
        elif address == 0x295D:
            sp = cpu.reg_read(UC_X86_REG_SP)
            inline = self.signed_word(sp) & 65535
            length = cpu.mem_read(inline,1)[0]
            self.write_word(sp,inline+length+1)
            self.return_from_hook()

    def run(self, initialize):
        """Respecter les frontieres overlay ; ne pas simuler son chargeur DOS."""
        if initialize:
            self.cpu.mem_write(0xBA21,self.overlays[0x1000:0x2200])
            self.cpu.reg_write(UC_X86_REG_SP,STACK)
            self.cpu.emu_start(0xC597,0xC845,count=100000)
            if self.cpu.reg_read(UC_X86_REG_IP) != 0xC845:
                raise RuntimeError('Initialisation roster incomplete')
            self.cpu.emu_start(0xCB91,0xCBC1,count=1000) # Equipements, sans RNG geographique.
            if self.cpu.reg_read(UC_X86_REG_IP) != 0xCBC1:
                raise RuntimeError('Equipements incomplets')
        self.cpu.mem_write(0xBA21,self.overlays[0x2200:0x2E00])
        self.cpu.mem_write(STACK,word(STOP))
        self.cpu.reg_write(UC_X86_REG_SP,STACK)
        self.cpu.emu_start(0xBA28,STOP,timeout=3000000,count=500000)
        if not self.halted:
            raise RuntimeError(f'Creation sans frontiere IP={self.cpu.reg_read(UC_X86_REG_IP):04X}')
        return self.states


def execute_c(game,keys,initialize):
    result = subprocess.run([str(HERE/'build/cmake/tera_creation_bridge.exe')],
                            input=pack_roster(game,initialize)+''.join(f'{k}\n' for k in keys),
                            text=True,capture_output=True,timeout=5)
    if result.returncode not in (0,4):
        raise RuntimeError(result.stderr or f'Pont {result.returncode}')
    return [list(map(int,line.split()[1:])) for line in result.stdout.splitlines() if line.startswith('S ')]


def generate_cases():
    """Seeds, noms, chaque touche/borne, refus, renaissance et etats extremes."""
    cases = []
    seeds = (0,1,1986,0xFFFFFFFF,0x80000000,0x01000000,0xFFFFFF)
    prefixes = ['', 'A', 'AL\bIX\r', 'ABCDEFGHIJ', '\r', 'A7-LIX\r',
                'ALIX\rH', 'ALIX\rf', 'ALIX\r?', 'ALIX\rH0', 'ALIX\rF9',
                'ALIX\rH?000', 'ALIX\rH999?', 'ALIX\rH333FFFFFFN']
    for seed in seeds:
        for prefix in prefixes:
            game = empty_roster(seed)
            cases.append((f'partial/seed{seed}/{prefix!r}',game,True,prefix))
        for gender in ('H','F','f','?', '\r'):
            for digit in ('0','1','9'):
                for bonus in ('FFFFFF','IIIIII','NNNNNN','EVTA CH'):
                    game = empty_roster(seed)
                    text = 'ALIX\r'+gender+digit*3+bonus+'O'
                    cases.append((f'full/seed{seed}/{gender!r}/{digit}/{bonus}',game,True,text))
    for digit in range(10):
        game = empty_roster(1986+digit)
        cases.append((f'digit{digit}',game,True,'A\rH'+str(digit)*3+'FIVEHA\r'))
    for letter in 'FINEVTACH':
        game = empty_roster(1986)
        cases.append((f'bonus{letter}',game,True,'A\rF000'+letter*6+' '))
    for keep in ('O','?', '\r', '\x1b', 'n'):
        game = empty_roster(1986)
        text = 'FIRST\rH333FIVEHA'+keep
        if keep == 'n':
            text += 'SECOND\rF999NNNNNNO'
        cases.append((f'keep{keep!r}',game,True,text))
    rng = random.Random(1986)
    for index in range(120):
        game = empty_roster(rng.getrandbits(32))
        game.update(money=rng.choice([-32768,-1,0,499,500,501,32767]),
                    experience=12345,days=-4,level=12,dummy_level=7,energy=77,
                    position=321,world_id=3,arrival_pending=0,party_available=0)
        game['keys'] = [rng.randrange(-5,255) for _ in range(10)]
        for id, actor in enumerate(game['actors']):
            actor.update(active=rng.randrange(2),hp=rng.choice([-32768,-1,0,1,32767]),
                         magic=-1,hp_limit=333,magic_limit=444,condition=rng.choice(['OK','HS','EM']),
                         name='OLD'+str(id))
            actor['stats'] = [rng.randrange(256) for _ in range(10)]
            actor['skills'] = [rng.randrange(6) for _ in range(10)]
            actor['equipment'] = [rng.randrange(26) for _ in range(10)]
        initialize = index % 2 == 0
        text = 'ALIX\r'+rng.choice(['H','F','?'])+''.join(str(rng.randrange(10)) for _ in range(3))
        text += ''.join(rng.choice('FINEVTACH') for _ in range(6))
        if index%3==0:
            text+='NSECOND\rF999FIVEHA'
        text+='O'
        cases.append((f'rebirth{index}/initialize{initialize}',game,initialize,text))
    return cases


def main():
    started = time.monotonic()
    image,overlays = SOURCE.read_bytes(),OVERLAYS.read_bytes()
    if hashlib.sha256(image).hexdigest()!=EXPECTED_HASH or hashlib.sha256(overlays).hexdigest()!=OVERLAY_HASH:
        raise SystemExit('Empreintes originales differentes')
    subprocess.run(['cmake','--build',str(HERE/'build/cmake')],check=True)
    failures = []
    boundaries = completed = draws = 0
    cases = generate_cases()
    for index,(name,game,initialize,text) in enumerate(cases,1):
        keys = list(map(ord,text))
        machine = CreationRoutine(image,overlays,game,keys)
        binary,c = machine.run(initialize),execute_c(game,keys,initialize)
        boundaries += len(c)
        completed += int(c[-1][0]==5)
        draws += len(machine.draw_bounds)
        if binary!=c:
            mismatch = next((i for i,(a,b) in enumerate(zip(binary,c)) if a!=b),min(len(binary),len(c)))
            diffs = [(j,x,y) for j,(x,y) in enumerate(zip(binary[mismatch],c[mismatch])) if x!=y] if mismatch<min(len(binary),len(c)) else []
            failures.append(dict(name=name,boundary=mismatch,lengths=[len(binary),len(c)],diffs=diffs[:20]))
        if index%100==0:
            print(f'Creations comparees {index}/{len(cases)} ; ecarts {len(failures)}',flush=True)
    report = dict(com_sha256=EXPECTED_HASH,overlays_sha256=OVERLAY_HASH,
                  cases=len(cases),completed=completed,input_boundaries=boundaries,
                  original_rng_calls=draws,mismatches=len(failures),first_mismatches=failures[:5],
                  seconds=round(time.monotonic()-started,2),
                  scope='OVL0010 character regions only; full OVL0022 input/state rules; original RNG executed; presentation bypassed')
    (HERE/'build/audit/creation_comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    if failures:
        raise SystemExit(1)


if __name__=='__main__':
    main()
