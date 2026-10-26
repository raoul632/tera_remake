"""Verifier le transport et les vraies cartes importees, sans oracle Python."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'frontend'))
from combat import demo_game, load_savedata


class CombatFrontendTests(unittest.TestCase):
    def test_savedata_offsets_and_untouched_resources(self):
        """Sentinelles distinctes pour reperer une erreur de colonne/offset."""
        payload = bytearray(0x27B2)
        def put(address, raw):
            payload[address-0x73B0:address-0x73B0+len(raw)] = raw
        def string(address, value, size):
            raw = value.encode('ascii')
            put(address, bytes([len(raw)]) + raw.ljust(size-1, b'\0'))
        for id in range(10):
            put(0x73B0+10*id, bytes([id]*10))
            put(0x7414+10*id, bytes([id+10]*10))
            put(0x7478+10*id, bytes([id+20]*10))
            for address, value in ((0x74DC, id+30), (0x74F0, id+40),
                                   (0x7504, id+50), (0x7518, id+60)):
                put(address+2*id, struct.pack('<h', value))
            put(0x9402+id, bytes([int(id == 5)]))
            string(0x94C8+10*id, f'Actor{id}', 10)
            string(0x952C+3*id, 'EM' if id == 5 else 'HS', 3)
        for position in range(1100):
            put(0x75EE+7*position, bytes([1, 2, 3, 4, 5, 6, position % 255]))
        for address, value in ((0x75D2, 419), (0x75D4, 32767),
                               (0x75D6, -123), (0x75E8, 7)):
            put(address, struct.pack('<h', value))
        put(0x75EA, bytes([12, 3, 6, 9]))
        string(0x9B3A, 'SECRET', 10)
        with tempfile.TemporaryDirectory(dir=HERE / 'build') as directory:
            path = Path(directory) / 'synthetic_savedata.bin'
            path.write_bytes(payload)
            game = load_savedata(path, 22, 0xFFFFFFFF, 4, 1, -999)
            self.assertEqual(path.read_bytes(), payload)
        self.assertEqual(game['actors'][5]['stats'], [15]*10)
        self.assertEqual(game['actors'][5]['skills'], [25]*10)
        self.assertEqual(game['actors'][5]['equipment'], [5]*10)
        self.assertEqual(game['actors'][5]['condition'], 'EM')
        self.assertEqual(game['actors'][5]['hp'], 35)
        self.assertEqual(game['actors'][5]['name'], 'Actor5')
        self.assertEqual(game['maps_column7'][419], 419 % 255)
        self.assertEqual(game['helpers'], [12, 3, 6, 9])
        self.assertEqual(game['secret_word'], 'SECRET')
        self.assertEqual(game['world_id'], 4)
        self.assertEqual(game['previous_position'], -999)
        self.assertEqual(game['seed'], 0xFFFFFFFF)

    def test_terminal_export_escape_uses_supplied_map(self):
        """Une seule case autorisee, export apres la riposte, entree inchangee."""
        game = demo_game(7, 1986)
        game['world_id'] = 4
        game['maps_column7'] = [0]*1100
        game['maps_column7'][494] = 6
        with tempfile.TemporaryDirectory(dir=HERE / 'build') as directory:
            source, destination = Path(directory)/'state.json', Path(directory)/'result.json'
            raw = json.dumps(game)
            source.write_text(raw, encoding='utf-8')
            process = subprocess.run([sys.executable, str(HERE/'frontend/combat.py'),
                                      '--state', str(source), '--output', str(destination)],
                                     input='FO', text=True, capture_output=True, timeout=10)
            self.assertEqual(process.returncode, 0, process.stderr)
            result = json.loads(destination.read_text(encoding='utf-8'))
            self.assertEqual(source.read_text(encoding='utf-8'), raw)
        self.assertEqual(result['position'], 494)
        self.assertEqual(result['previous_position'], 493)
        self.assertEqual(result['maps_column7'], game['maps_column7'])
        self.assertEqual(result['combat_result']['phase'], 3)
        self.assertEqual(result['combat_result']['fled'], 1)
        self.assertEqual(result['location_changed'], 1)
        self.assertEqual(result['money'], 0)
        self.assertEqual(result['experience'], 0)

    def test_eof_does_not_export_a_finished_combat(self):
        """Une entree tronquee ne doit jamais passer pour une issue du jeu."""
        with tempfile.TemporaryDirectory(dir=HERE / 'build') as directory:
            destination = Path(directory)/'result.json'
            process = subprocess.run([sys.executable, str(HERE/'frontend/combat.py'),
                                      '--demo', '--seed', '1986', '--output', str(destination)],
                                     input='T', text=True, capture_output=True, timeout=10)
            self.assertNotEqual(process.returncode, 0)
            self.assertFalse(destination.exists())


if __name__ == '__main__':
    unittest.main()
