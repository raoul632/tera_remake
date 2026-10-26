"""Audit optionnel BIN : traces de primitives, frequences et delais originaux.

Execute les routines de TERA.COM ; seuls DRAW/CLEAR/SPRITE et les ports
sonores sont remplaces par une collecte. Aucun calcul graphique ou sonore
du remake ne sert d'oracle. Python/Unicorn ne sont pas requis par le jeu.
"""
import hashlib
import json
from compare_combat_binary import OriginalRoutine, SOURCE, EXPECTED_HASH, HERE, STOP, STACK, word
from unicorn.x86_const import UC_X86_REG_AX, UC_X86_REG_SP, UC_X86_REG_IP


class MediaRoutine(OriginalRoutine):
    def __init__(self, image):
        super().__init__(image)
        self.trace = []
        self.seed = 1986

    def call(self, entry, args, has_result=False):
        values = [STOP, *reversed(args)]
        self.cpu.mem_write(STACK, b"".join(word(v) for v in values))
        self.cpu.reg_write(UC_X86_REG_SP, STACK)
        self.cpu.emu_start(entry, STOP, timeout=10000000, count=2000000)
        assert self.cpu.reg_read(UC_X86_REG_IP) == STOP, hex(self.cpu.reg_read(UC_X86_REG_IP))
        assert self.cpu.reg_read(UC_X86_REG_SP) == STACK + 2*len(values)

    def on_instruction(self, cpu, address, size, user_data):
        primitives = {0x2DB7: ("line", 5), 0x4544: ("clear", 5),
                      0x5051: ("sprite", 3)}
        if address == 0x10DA:
            self.seed = (129*self.seed + 0x361962E9) & 0xFFFFFFFF
            cpu.reg_write(UC_X86_REG_AX, (self.seed >> 17) % cpu.reg_read(UC_X86_REG_AX))
            self.return_from_hook()
        elif address in primitives:
            name, count = primitives[address]
            sp = cpu.reg_read(UC_X86_REG_SP)
            self.trace.append([name, *[self.signed_word(sp + 2*i)
                                      for i in range(count, 0, -1)]])
            self.return_from_hook(count*2)
        elif address in (0x59C, 0x213, 0x5C3):
            self.trace.append([{0x59C: "hz", 0x213: "ms", 0x5C3: "off"}[address],
                               cpu.reg_read(UC_X86_REG_AX)])
            self.return_from_hook()
        else:
            super().on_instruction(cpu, address, size, user_data)


def main():
    image = SOURCE.read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED_HASH
    result = {"sha256": EXPECTED_HASH, "figures": {}, "sound": {}}
    for key, entry, args in (("large", 0x77F0, []), ("hood0", 0x7B74, [0, 1]),
                              ("hood84", 0x7B74, [84, 1])):
        machine = MediaRoutine(image)
        machine.call(entry, args, has_result=False)
        result["figures"][key] = machine.trace
    for key, entry, args in (("ray", 0x6221, [6000, 6, 6]),
                              ("pirates", 0x6221, [200, 2, 200]),
                              ("temple", 0x6221, [30, 1400, 6]),
                              ("launch", 0x666B, []), ("landing", 0x6721, []),
                              ("country", 0x6352, [1, 1]), ("country43", 0x6352, [4, 3]),
                              ("melody", 0x6496, [9, 8]), ("victory", 0x6518, [2, 9, 8]),
                              ("monster", 0x62B7, [60, 100, 60, 12, 14, 8000, 10000]),
                              ("town8", 0x62B7, [130, 100, 80, 6, 14, 1, 5000])):
        machine = MediaRoutine(image)
        machine.call(entry, args, has_result=False)
        result["sound"][key] = machine.trace
    output = HERE / "build/audit/media_binary_trace.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(output)
    for name, trace in result["figures"].items():
        print(name, len(trace), "primitives")
    for name, trace in result["sound"].items():
        hz = 0
        digest = 2166136261
        notes = ms = 0
        for kind, value in trace:
            if kind == "hz": hz = value
            elif kind == "off": hz = 0
            else:
                notes += 1
                ms += value
                for item in (hz, value):
                    digest = ((digest ^ item)*16777619) & 0xFFFFFFFF
        print(name, notes, ms, f"{digest:08x}")
    # Petit vecteur de primitives relevees dans le BIN, sans image originale.
    lines = ["/* Genere par tools/audit_media_binary.py depuis le COM SHA256 confirme. */",
             "typedef struct {unsigned kind;int a,b,c,d,color;} OriginalPrimitive;"]
    for name, trace in result["figures"].items():
        lines.append(f"static const OriginalPrimitive original_{name}[]={{")
        for kind, *values in trace:
            tag = {"line": 1, "clear": 2, "sprite": 3}[kind]
            values = [tag, *values]
            values += [0]*(6-len(values))
            lines.append("    {" + ",".join(map(str, values)) + "},")
        lines.append("};")
    (HERE / "tests/original_figure_vectors.h").write_text("\n".join(lines)+"\n", encoding="utf-8")


if __name__ == "__main__":
    main()
