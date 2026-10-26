"""Optional native SOUND/DELAY oracle; no game/runtime dependency."""
import hashlib
from audit_media_binary import MediaRoutine, SOURCE, EXPECTED_HASH, HERE


def main():
    image = SOURCE.read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED_HASH
    cases = [
        ('objective', [(0x6496, [9, 8]), (0x6518, [1, 9, 8]), (0x6518, [2, 9, 8])]),
        ('recruitment', [(0x6518, [1, 9, 8])]),
        ('meduz_entry', [(0x68F4, [])]),
        ('refusal', [(0x693B, [])]),
        ('trap_sweep', [(0x6121, [50, 1500, 10, 20])]),
        ('explosion_sound', [(0x61D1, [100, 2, 0, 16])]),
        ('meduz_guardians', [(0x6221, [3000, 30, 100])]),
    ]
    lines = ['/* Native COM SOUND/DELAY, seed 1986; audit_progression_sound.py. */',
             'typedef struct {uint32_t hash,seed;unsigned notes,ms;} OriginalProgressionSound;',
             'static const OriginalProgressionSound original_progression_sounds[]={']
    for name, calls in cases:
        machine = MediaRoutine(image)
        for entry, args in calls:
            machine.call(entry, args)
        hz = 0
        score = []
        pending = False
        off = False
        for kind, value in machine.trace:
            if kind in ('hz', 'off'):
                if pending:
                    score.append((hz, 0))
                pending = kind == 'hz'
                hz = value if pending else 0
                off = not pending
            elif kind == 'ms':
                if not (off and value == 0):
                    score.append((hz, value))
                pending = False
            else:
                raise AssertionError(kind)
        if pending:
            score.append((hz, 0))
        digest = 2166136261
        for pair in score:
            for item in pair:
                digest = ((digest ^ item)*16777619) & 0xFFFFFFFF
        notes = len(score)
        elapsed = sum(ms for hz, ms in score)
        lines.append(f'    {{0x{digest:08x},0x{machine.seed:08x},{notes},{elapsed}}}, /* {name} */')
        print(name, f'{digest:08x}', notes, elapsed, flush=True)
    lines.append('};')
    (HERE/'tests/original_progression_sound_vectors.h').write_text('\n'.join(lines)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
