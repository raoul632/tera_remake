"""Oracle optionnel des primitives de scene residentes, execute dans le COM.

Genere des vecteurs C independants de SDL ; aucun bitmap original embarque.
Les sorties texte/BIOS sont ignorees, sans ignorer les calculs de dessin.
"""
import hashlib
import json
from audit_media_binary import MediaRoutine
from compare_combat_binary import SOURCE, EXPECTED_HASH, HERE
from unicorn.x86_const import UC_X86_REG_SP, UC_X86_REG_IP


class SceneRoutine(MediaRoutine):
    def on_instruction(self, cpu, address, size, user_data):
        graphics = {0x2DB4: ("point", 3), 0x2DBD: ("arc", 5),
                    0x2DCF: ("fill", 4), 0x2DC6: ("rect", 4),
                    0x5E04: ("spriteoff", 3), 0x46BC: ("clearoff", 5)}
        if address in graphics:
            kind, count = graphics[address]
            sp = cpu.reg_read(UC_X86_REG_SP)
            self.trace.append([kind, *[self.signed_word(sp+2*i) for i in range(count, 0, -1)]])
            self.return_from_hook(count*2)
        elif address in (0x6013, 0x60BB):
            self.trace.append(["layer" if address == 0x6013 else "merge"])
            self.return_from_hook()
        elif address == 0x7237:
            cpu.reg_write(UC_X86_REG_IP, 0x7243)  # texte, apres tout le dessin
        else:
            super().on_instruction(cpu, address, size, user_data)


def main():
    image = SOURCE.read_bytes()
    assert hashlib.sha256(image).hexdigest() == EXPECTED_HASH
    traces = {}
    lines = ["/* Primitives executees dans TERA.COM ; outils/audit_scene_binary.py. */",
             "typedef struct {unsigned kind;int a,b,c,d,color;} OriginalScenePrimitive;"]
    for name, entry, args in (("spaceport", 0x7136, []), ("catalyst", 0x6DC5, [38,164,140,4,2,0]),
                              ("mountains", 0x6C4F, []), ("portrait", 0x7A96, [12,3]),
                              ("monolith", 0x7309, [])):
        machine = SceneRoutine(image)
        machine.call(entry, args)
        traces[name] = machine.trace
        if name == "monolith":
            print(name, len(machine.trace), "primitives (FillShape a verifier)")
            continue
        lines.append(f"static const OriginalScenePrimitive original_scene_{name}[]={{")
        for kind, *values in machine.trace:
            tag = {"line": 1, "clear": 2, "sprite": 3, "point": 4, "layer": 5,
                   "merge": 6, "spriteoff": 7, "clearoff": 8, "fill": 9, "arc": 10}[kind]
            values = [tag, *values] + [0]*(5-len(values))
            lines.append("    {" + ",".join(map(str, values)) + "},")
        lines.append("};")
        print(name, len(machine.trace), "primitives", hex(machine.seed))
    (HERE / "tests/original_scene_vectors.h").write_text("\n".join(lines)+"\n", encoding="utf-8")
    (HERE / "build/audit/scene_binary_trace.json").write_text(json.dumps(
        dict(sha256=EXPECTED_HASH, traces=traces), indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
