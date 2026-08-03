"""Extraction des sprites/images CGA depuis data/compiled program/*.IMA.

CRAPIM.IMA (23206 octets, confirme = 0x5AA6 exactement -- taille du
buffer BPA2A2 dans OVL_00DB.PAS) : atlas de 82 sprites, format et
algorithme d'offset portes ligne a ligne depuis OVL_00DB.PAS (section 10
des notes moteur) -- chaque sprite est prefixe largeur(octets)/hauteur
(pixels), donnees stockees en deux plans pair/impair (entrelacement CGA
$B800/$BA00), avec mise a l'echelle (hscale/vscale) directement reflete
dans la taille des donnees stockees pour les sprites au-dela de SPR_32.

Les 6 autres fichiers (ASTRON/BANNIERE/FACADE/KRONO/MAISONJ/MAISONS.IMA)
font tous exactement 16384 = 0x4000 octets = une page memoire CGA
complete -- aucune routine de chargement n'a ete retrouvee pour ceux-la
(pas d'overlay identifie qui les charge), mais la taille exacte
correspond au format de dump plein ecran CGA standard (320x200, 2 plans
de 8000 octets utiles + padding a 8192), donc traites comme tels ICI PAR
HYPOTHESE, non confirme par lecture de code.

Palette : CGA 4 couleurs. Le jeu appelle PALETTE(0..3) a plusieurs
endroits (OVL_0145.PAS), ce qui suggere plus que les 2 palettes CGA
standard -- l'exacte correspondance n'a pas ete confirmee. Ce script
utilise par defaut la palette CGA la plus courante pour les jeux DOS de
cette epoque (noir/cyan/magenta/blanc, "palette 1 intensite haute").
A verifier visuellement et ajuster si besoin (voir CGA_PALETTE).

Lancer avec l'interpreteur qui a Pillow (`python`, pas `python3` -- voir
CLAUDE.md) :
    python tools/extract_sprites.py
"""
from __future__ import annotations
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
COMPILED_DIR = ROOT / "data" / "compiled program"
OUT_DIR = ROOT / "data" / "sprites_extracted"

CGA_PALETTE = [
    (0, 0, 0),
    (85, 255, 255),
    (255, 85, 255),
    (255, 255, 255),
]

SPR_32 = 0x32
SPR_3B = 0x3B
SPR_4A = 0x4A
SPR_4C = 0x4C
SPR_51 = 0x51
SPR_52 = 0x52

BACKGROUND_FILES = ["ASTRON.IMA", "BANNIERE.IMA", "FACADE.IMA", "KRONO.IMA", "MAISONJ.IMA", "MAISONS.IMA"]


def _scale_for(sprite_id: int) -> tuple[int, int]:
    """Port exact de la cascade de IF de OVL_00DB.PAS (ordre important :
    chaque IF peut ecraser le resultat du precedent pour le meme sprite)."""
    hscale, vscale = 1, 1
    if sprite_id > SPR_32:
        hscale = 2
        if sprite_id == SPR_3B:
            vscale = 2
            hscale = 1
        if sprite_id > SPR_3B:
            hscale = 2
            vscale = 3 if sprite_id > SPR_4A else 2
        if sprite_id > SPR_4C:
            hscale = 3
            vscale = 1
        if sprite_id == SPR_51:
            vscale = 3
        if sprite_id == SPR_52:
            vscale = 6
            hscale = 2
    return hscale, vscale


def _unpack_row(row_bytes: bytes, width_px: int) -> list[int]:
    """4 pixels/octet, 2 bits/pixel, poids fort en premier (CGA standard)."""
    pixels = []
    for byte in row_bytes:
        pixels.append((byte >> 6) & 0b11)
        pixels.append((byte >> 4) & 0b11)
        pixels.append((byte >> 2) & 0b11)
        pixels.append(byte & 0b11)
    return pixels[:width_px]


def _interlaced_image(row_bytes_len: int, plane0: bytes, plane1: bytes, transparent_key: int | None = None) -> Image.Image:
    """`transparent_key` : indice de palette (0-3) a rendre transparent
    (alpha=0) au lieu d'opaque. Utilise pour les sprites `CRAPIM.IMA`
    (voir `extract_crapim` -- l'indice 0/noir sert de cle de transparence,
    convention standard pour ce genre de blit CGA 2 bits/pixel sans canal
    alpha materiel : sans cle de transparence, superposer deux sprites
    -- ex. un bouquet de 3 arbres avec de petits decalages -- ferait que
    chaque sprite dessine par-dessus efface un rectangle noir opaque sur
    celui du dessous). `None` (par defaut) = toujours opaque, utilise
    pour les 6 fonds plein ecran (pas de composition, pas besoin de
    transparence)."""
    rows = len(plane0) // row_bytes_len if row_bytes_len else 0
    width_px = row_bytes_len * 4
    mode = "RGBA" if transparent_key is not None else "RGB"
    img = Image.new(mode, (max(1, width_px), max(1, rows * 2)))
    px = img.load()

    def _color(v: int):
        if transparent_key is not None:
            return (0, 0, 0, 0) if v == transparent_key else CGA_PALETTE[v] + (255,)
        return CGA_PALETTE[v]

    for row in range(rows):
        even = _unpack_row(plane0[row * row_bytes_len:(row + 1) * row_bytes_len], width_px)
        odd = _unpack_row(plane1[row * row_bytes_len:(row + 1) * row_bytes_len], width_px)
        for x, v in enumerate(even):
            px[x, 2 * row] = _color(v)
        for x, v in enumerate(odd):
            px[x, 2 * row + 1] = _color(v)
    return img


def extract_crapim(out_dir: Path) -> list[dict]:
    data = (COMPILED_DIR / "CRAPIM.IMA").read_bytes()
    out_dir.mkdir(parents=True, exist_ok=True)
    counter = 0
    manifest = []

    for sprite_id in range(1, SPR_52 + 1):
        hscale, vscale = _scale_for(sprite_id)
        width = data[counter]; counter += 1
        height = data[counter]; counter += 1

        half_h = height // 2
        if half_h * 2 != height and sprite_id < SPR_3B:
            plane_len = hscale * vscale * width * ((height - 1) // 2)
        else:
            plane_len = (width * height * hscale * vscale) // 2

        # Les deux plans sont entrelaces OCTET PAR OCTET dans le fichier
        # (Pascal : SpriteData[0][i]:=lit 1 octet ; SpriteData[1][i]:=lit
        # l'octet SUIVANT ; boucle) -- pas deux blocs contigus.
        plane0 = bytearray(plane_len)
        plane1 = bytearray(plane_len)
        for i in range(plane_len):
            plane0[i] = data[counter]; counter += 1
            plane1[i] = data[counter]; counter += 1

        row_bytes_len = width * hscale
        img = _interlaced_image(row_bytes_len, plane0, plane1, transparent_key=0)

        filename = f"sprite_{sprite_id:02d}.png"
        img.save(out_dir / filename)
        manifest.append({
            "id": sprite_id, "file": filename,
            "width_bytes_raw": width, "height_px_raw": height,
            "hscale": hscale, "vscale": vscale,
            "final_size": list(img.size),
        })

    print(f"CRAPIM.IMA : {counter}/{len(data)} octets consommes ({SPR_52} sprites)")
    if counter != len(data):
        print(f"  ATTENTION : {len(data) - counter} octets non consommes -- verifier l'algorithme.")
    return manifest


def extract_backgrounds(out_dir: Path) -> list[dict]:
    out_dir.mkdir(parents=True, exist_ok=True)
    manifest = []
    width_bytes, height_lines = 80, 100  # 320px/4, 200 lignes / 2 plans
    for name in BACKGROUND_FILES:
        path = COMPILED_DIR / name
        if not path.exists():
            continue
        data = path.read_bytes()
        even = data[:width_bytes * height_lines]
        odd = data[0x2000:0x2000 + width_bytes * height_lines]
        img = _interlaced_image(width_bytes, even, odd)
        out_name = f"{path.stem.lower()}.png"
        img.save(out_dir / out_name)
        manifest.append({"file": name, "out": out_name, "size": list(img.size)})
    return manifest


def make_contact_sheet(sprite_dir: Path, manifest: list[dict], out_path: Path) -> None:
    cols = 10
    cell = 64
    rows = (len(manifest) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell, rows * cell), (30, 30, 30))
    for entry in manifest:
        img = Image.open(sprite_dir / entry["file"]).convert("RGBA")
        img.thumbnail((cell - 4, cell - 4))
        idx = entry["id"] - 1
        x, y = (idx % cols) * cell + 2, (idx // cols) * cell + 2
        sheet.paste(img, (x, y), img)  # img comme masque -- respecte la transparence
    sheet.save(out_path)


def main() -> None:
    sprite_manifest = extract_crapim(OUT_DIR / "crapim")
    bg_manifest = extract_backgrounds(OUT_DIR / "backgrounds")
    make_contact_sheet(OUT_DIR / "crapim", sprite_manifest, OUT_DIR / "crapim_contact_sheet.png")

    manifest_path = OUT_DIR / "manifest.json"
    manifest_path.write_text(
        json.dumps({"crapim_sprites": sprite_manifest, "backgrounds": bg_manifest}, indent=2),
        encoding="utf-8",
    )
    print(f"{len(sprite_manifest)} sprites + {len(bg_manifest)} fonds extraits -> {OUT_DIR}")


if __name__ == "__main__":
    main()
