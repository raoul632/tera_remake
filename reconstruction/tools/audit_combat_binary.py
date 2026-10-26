"""Capturer les preuves BIN du combat pour pouvoir refaire l'analyse.

Les originaux sont lus dans le depot parent ; les extraits et le manifeste
restent dans reconstruction/build/audit, exclu de Git. Le desassemblage
est une preuve statique : cet outil n'execute pas le jeu original.
"""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]  # sources historiques, lecture seule
OUT = Path(__file__).resolve().parents[1] / "build" / "audit"


def describe_source(source: Path) -> dict:
    """Identifier le fichier analyse par sa taille et son empreinte SHA256.

    Intention : permettre au lecteur de verifier qu'il travaille sur la
    meme copie. L'empreinte identifie la source, elle ne valide pas une regle.
    Le fichier est uniquement lu ; aucun binaire n'est ajoute au manifeste.
    """
    return {
        "size": source.stat().st_size,
        "sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
    }


def capture_disassembly(objdump: str, source: Path, origin: int,
                        start: int, end: int, destination: Path) -> None:
    """Ecrire le desassemblage 8086 de l'intervalle [start, end[.

    origin est l'adresse correspondant au premier octet du fichier, pas
    son point d'entree. Elle aligne les adresses sur celles du Pascal.
    Une erreur d'objdump interrompt la capture pour eviter une preuve vide.
    """
    command = [objdump, "-D", "-b", "binary", "-m", "i8086",
               "-Maddr16,data16,intel", f"--adjust-vma={origin:#x}",
               f"--start-address={start:#x}", f"--stop-address={end:#x}", str(source)]
    result = subprocess.run(command, check=True, capture_output=True)
    destination.write_bytes(result.stdout)


def main() -> None:
    """Extraire l'overlay utile, capturer les regions et publier le manifeste.

    Seul OUT est ecrit. Les regions sont ciblees sur les fonctions auditees ;
    le script ne tente pas de certifier ou desassembler tout le jeu.
    """
    objdump = shutil.which("objdump")
    if not objdump:
        raise SystemExit("objdump requis pour produire les preuves BIN")
    OUT.mkdir(parents=True, exist_ok=True)
    com = ROOT / "data" / "compiled program" / "TERA.COM"
    overlays = ROOT / "data" / "compiled program" / "TERA.000"
    overlay = OUT / "OVL_0010.bin"
    # La plage est celle du module OVL_0010 dans TERA.000 ; conserver
    # l'original entier comme source, et n'ecrire que l'extrait local.
    overlay.write_bytes(overlays.read_bytes()[0x1000:0x2200])
    regions = [
        ("combat_formules", com, 0x100, 0x8215, 0x86E1),
        ("combat_menu", com, 0x100, 0x8790, 0x8B5E),
        ("combat_randomize", com, 0x100, 0x0F14, 0x0F22),
        ("combat_random", com, 0x100, 0x10DA, 0x111B),
        ("combat_word", com, 0x100, 0x42AC, 0x4418),
        ("combat_phrase", com, 0x100, 0x4473, 0x4503),
        ("combat_confirmation", com, 0x100, 0x4800, 0x4850),
        ("combat_escape", com, 0x100, 0x86E1, 0x8790),
        ("combat_resolution", com, 0x100, 0x8B5E, 0x9419),
        ("combat_turns", com, 0x100, 0x9419, 0x9B4E),
        ("combat_actor_condition", com, 0x100, 0x7CC8, 0x7DBD),
        ("combat_actor_limits", com, 0x100, 0x4C6C, 0x4CF9),
        # Ensembles Pascal : confirme les IDs qui peuvent empoisonner.
        ("combat_poison_set", com, 0x100, 0x14FC, 0x1617),
        # BA0F designe le descripteur resident ; le contenu commence a BA21.
        # Cette origine aligne INNER_BA2F a BA2F et les appels RANDOM a 10DA.
        ("combat_helpers", overlay, 0xBA21, 0xC246, 0xC509),
    ]
    manifest = {"sources": {}, "regions": []}
    for source in (com, overlays, ROOT / "data/source code/OVL_0010.PAS",
                   ROOT / "data/compiled program/VERBES",
                   ROOT / "data/compiled program/OBJETS",
                   ROOT / "data/compiled program/VALMON"):
        manifest["sources"][source.relative_to(ROOT).as_posix()] = describe_source(source)
    for name, source, origin, start, end in regions:
        capture_disassembly(objdump, source, origin, start, end, OUT / f"{name}.asm")
        manifest["regions"].append({"name": name, "origin": origin,
                                    "start": start, "end_exclusive": end})
    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
