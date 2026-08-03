"""Regenere data/game_data.json depuis les fichiers de donnees bruts
(data/compiled program/VERBES, OBJETS, NOMON, NOMPERS, VALMON, MESSAGE,
MESSAGE2, MESSAGE3) -- voir docs/TERA_notes_moteur.md section 12.

Format confirme pour tous ces fichiers : enregistrements de taille FIXE,
chacun = 1 octet de longueur + N octets de contenu (encodage CP437) +
padding zero jusqu'a la taille d'enregistrement. VALMON est different :
6 entiers 16 bits signes par enregistrement (pas une chaine), deja
documente section 14.

Ce script n'existait pas avant (le premier jet de game_data.json avait
ete fait a la main dans une session precedente) -- ecrit maintenant pour
que l'extraction soit reproductible et pour ajouter MESSAGE/MESSAGE2/
MESSAGE3, qui n'etaient pas encore dans game_data.json.

Lancer avec n'importe quel interpreteur (aucune dependance externe) :
    python3 tools/extract_text_data.py
"""
from __future__ import annotations
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
COMPILED_DIR = ROOT / "data" / "compiled program"
OUT_PATH = ROOT / "data" / "game_data.json"

# (fichier, nombre d'entrees connu) -- la taille d'enregistrement est
# derivee de la taille du fichier / nombre d'entrees (confirme byte-exact
# pour chacun, voir docs/TERA_notes_moteur.md).
STRING_FILES = {
    "verbes": ("VERBES", 56),
    "objets": ("OBJETS", 25),
    "nomon": ("NOMON", 22),
    "nompers": ("NOMPERS", 27),
    "message": ("MESSAGE", 102),
    "message2": ("MESSAGE2", 33),
    "message3": ("MESSAGE3", 33),
}

# Correspondance action de combat -> indice VERBES/MESSAGE* (section 12.1).
COMBAT_VERB_INDICES = list(range(26, 34))  # 26..33 inclus


def read_string_records(filename: str, count: int) -> list[str]:
    data = (COMPILED_DIR / filename).read_bytes()
    record_width = len(data) // count
    assert record_width * count == len(data), (
        f"{filename}: {len(data)} octets ne se divise pas exactement par {count} entrees"
    )
    records = []
    for i in range(count):
        chunk = data[i * record_width:(i + 1) * record_width]
        length = chunk[0]
        records.append(chunk[1:1 + length].decode("cp437"))
    return records


def read_valmon() -> list[dict]:
    data = (COMPILED_DIR / "VALMON").read_bytes()
    record_width = 12  # 6 entiers 16 bits signes, confirme section 14
    count = len(data) // record_width
    monsters = []
    names = read_string_records("NOMON", 22)
    for i in range(count):
        chunk = data[i * record_width:(i + 1) * record_width]
        hp_base, resist_a, resist_b, weakness, initiative, hp_variance = struct.unpack("<6h", chunk)
        monsters.append({
            "id": i + 1,
            "name": names[i],
            "hp_base": hp_base,
            "resist_a": resist_a,
            "resist_b": resist_b,
            "weakness_item_id": weakness if weakness != 0 else None,
            "initiative": initiative,
            "hp_variance": hp_variance,
        })
    return monsters


def main() -> None:
    out: dict = {}
    for key, (filename, count) in STRING_FILES.items():
        out[key] = read_string_records(filename, count)

    out["monsters"] = read_valmon()
    out["combat_verbs"] = [out["verbes"][i - 1] for i in COMBAT_VERB_INDICES]

    OUT_PATH.write_text(json.dumps(out, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Ecrit {OUT_PATH} : " + ", ".join(f"{k}={len(v)}" for k, v in out.items()))


if __name__ == "__main__":
    main()
