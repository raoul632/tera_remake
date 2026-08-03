"""Port exact des 23 scenes fixes d'Amarande (`Sav.f_Position MOD 23`),
`OVL_00FB.PAS` (procedure `INNER_C377`, la grosse instruction CASE en fin
de fichier) -- section 19.1/21 des notes moteur pour le contexte.

Contrairement a une premiere version modernisee (decor place par un
generateur pseudo-aleatoire seede par la position), CETTE table reproduit
l'algorithme original : chaque position d'Amarande tombe dans l'une de 23
"scenes" fixes, chacune une combinaison precise d'appels de placement de
sprites a des coordonnees codees en dur dans le Pascal. Rien n'est invente
ici -- chaque coordonnee/identifiant de sprite ci-dessous est recopie
depuis `data/source code/OVL_00FB.PAS`.

**Non porte / non confirme** :
- `DrawMountains` (silhouette de montagne "proche" quand on est adjacent a
  une chaine) est une primitive residente (hors overlay) dont le code n'a
  jamais ete retrouve (section 15 des notes moteur) -- `scene.py` la
  remplace par une silhouette vectorielle approximative, deja documentee
  comme telle avant ce port.
- Le `Circle`/`FillShape` conditionnel dans `INNER_BD0C` (`IF D_A679='6'`,
  donc seulement apres un deplacement vers l'est) n'est pas reproduit --
  fonction visuelle mineure et incertaine (peut-etre un soleil/une lune),
  hors de portee pour ce port.
- L'ancre exacte de `PutSprite_off(x,y,sprite)` (coin haut-gauche ou un
  autre point du sprite) n'a pas ete confirmee par desassemblage -- ce
  port suppose "coin haut-gauche", l'hypothese la plus simple et la plus
  courante pour ce genre de routine.

Espace de coordonnees original : CGA 320x200, aire visible ~320x192 (la
derniere ligne de commande occupe le bas de l'ecran). Les coordonnees X
ci-dessous sont recopiees TELLES QUE LE PASCAL LES ECRIT, c'est a dire en
OCTETS (pas en pixels) -- confirme en comparant tous les appels
`PutSprite(_off)` de tous les overlays decompiles : X ne depasse jamais
~80 nulle part (320px / 4 pixels-par-octet CGA = 80 octets/ligne), alors
que Y va normalement jusqu'a ~190. `scene.py::SPRITE_X_UNIT` fait la
conversion en pixels (x4) avant mise a l'echelle vers la zone de rendu
Pygame -- ne PAS multiplier par 4 ici, la table reste une transcription
brute du Pascal.
"""
from __future__ import annotations

from .sprites import (
    SPR_TREE_BIG_A, SPR_TREE_SMALL_A, SPR_TREE_SMALL_B, SPR_TREE_MED_A,
    SPR_TREE_MED_B, SPR_TREE_MED_C, SPR_TREE_SIGNATURE,
    SPR_ROCK_PEAK_A, SPR_ROCK_PEAK_B, SPR_ROCK_MOUND_A, SPR_ROCK_RIDGE_A,
    SPR_ROCK_MOUND_B, SPR_ROCK_HILL_A, SPR_ROCK_HILL_B, SPR_ROCK_SUMMIT,
    SPR_GROUND_PEBBLE, SPR_GROUND_TUFT_A, SPR_GROUND_TUFT_B, SPR_REED,
    SPR_CREATURE_A, SPR_CREATURE_B, SPR_CREATURE_C,
)

# Un "draw call" est soit :
#   ("put", x, y, sprite_id)                      -- PutSprite_off(x,y,spr)
#   ("fill", x1, y1, x2, y2)                       -- FillRect_off(...,0) (toujours noir dans le Pascal)
#   ("scatter", count, rng_key)                    -- INNER_C0EC(count)


def _tree_small_a(x: int, y: int) -> list[tuple]:
    """INNER_BDCC(x,y) : 3x SPR_20."""
    return [
        ("put", x, y, SPR_TREE_SMALL_A),
        ("put", x + 3, y - 4, SPR_TREE_SMALL_A),
        ("put", x + 8, y + 2, SPR_TREE_SMALL_A),
    ]


def _be1a() -> list[tuple]:
    """INNER_BE1A (fixe, sans parametres)."""
    return [
        ("put", 9, 176, SPR_TREE_SMALL_B),
        ("put", 15, 182, SPR_TREE_SMALL_B),
        ("put", 1, 158, SPR_TREE_MED_B),
    ]


def _tree_small_b3(x: int, y: int) -> list[tuple]:
    """INNER_BE5A(x,y) : 3x SPR_21."""
    return [
        ("put", x, y, SPR_TREE_SMALL_B),
        ("put", x + 4, y - 2, SPR_TREE_SMALL_B),
        ("put", x + 7, y + 6, SPR_TREE_SMALL_B),
    ]


def _tree_small_b5(x: int, y: int) -> list[tuple]:
    """INNER_BEA8(x,y) : 5x SPR_21."""
    return [
        ("put", x, y, SPR_TREE_SMALL_B),
        ("put", x + 5, y - 4, SPR_TREE_SMALL_B),
        ("put", x + 13, y - 2, SPR_TREE_SMALL_B),
        ("put", x + 9, y + 6, SPR_TREE_SMALL_B),
        ("put", x + 16, y + 8, SPR_TREE_SMALL_B),
    ]


def _bf20(o: int) -> list[tuple]:
    """INNER_BF20(BBP04) : bosquet dense avec decoupes noires."""
    return [
        ("put", 0x40, 0x52 - o, SPR_TREE_SMALL_B),
        ("put", 0x45, 80 - o, SPR_TREE_SMALL_B),
        ("put", 0x48, 80 - o, SPR_TREE_SMALL_B),
        ("fill", 0x40, 0x5e - o, 80, 110 - o),
        ("put", 0x42, 0x52 - o, SPR_TREE_SMALL_B),
        ("fill", 0x42, 0x60 - o, 80, 110 - o),
        ("put", 0x43, 0x5c - o, SPR_TREE_SMALL_B),
        ("put", 0x45, 90 - o, SPR_TREE_SMALL_B),
        ("put", 0x48, 0x58 - o, SPR_TREE_SMALL_B),
        ("fill", 0x43, 0x66 - o, 80, 0x78 - o),
        ("put", 0x45, 0x66 - o, SPR_TREE_MED_A),
    ]


def _c01c() -> list[tuple]:
    """INNER_C01C : bosquet avec le grand arbre "signature" (SPR_47)."""
    return [
        ("put", 0x30, 0x18, SPR_TREE_MED_C),
        ("put", 0x3a, 0x13, SPR_TREE_MED_C),
        ("put", 0x40, 0x11, SPR_TREE_MED_C),
        ("fill", 0x2e, 0x32, 80, 80),
        ("put", 0x34, 0x22, SPR_TREE_MED_C),
        ("fill", 0x36, 0x38, 80, 90),
        ("put", 0x36, 0x2a, SPR_TREE_MED_C),
        ("put", 0x3b, 0x25, SPR_TREE_MED_C),
        ("put", 0x40, 0x23, SPR_TREE_MED_C),
        ("fill", 0x34, 0x40, 80, 100),
        ("put", 0x3a, 0x40, SPR_TREE_SIGNATURE),
    ]


def _scatter(count: int) -> list[tuple]:
    """INNER_C0EC(count) : decor de sol (cailloux/touffes), x aleatoire
    dans 0-75, y aleatoire pair dans 148-186 -- voir `_draw_ground_scatter`
    dans scene.py pour le tirage."""
    return [("scatter", count, None)]


def _c1de(x: int, y: int) -> list[tuple]:
    """INNER_C1DE(x,y) : creature/carcasse au sol (machoire visible)."""
    return [
        ("put", x, y, SPR_CREATURE_A),
        ("put", x - 5, y + 6, SPR_CREATURE_C),
        ("put", x + 9, y + 4, SPR_CREATURE_B),
    ]


def _c22c(o: int) -> list[tuple]:
    """INNER_C22C(BBP04)."""
    return [
        ("put", o + 15, 0x70, SPR_ROCK_PEAK_A),
        ("put", o, 0x76, SPR_ROCK_MOUND_B),
        ("put", o + 12, 0x7a, SPR_ROCK_PEAK_B),
        ("put", o + 17, 0x72, SPR_ROCK_RIDGE_A),
        ("put", o + 10, 0x82, SPR_ROCK_MOUND_A),
    ]


def _c298(o: int) -> list[tuple]:
    """INNER_C298(BBP04)."""
    return [
        ("put", o + 19, 110, SPR_ROCK_PEAK_A),
        ("put", o, 0x6a, SPR_ROCK_HILL_B),
        ("put", o + 18, 0x7c, SPR_ROCK_MOUND_B),
        ("put", o + 12, 0x84, SPR_ROCK_MOUND_A),
    ]


def _c2f2(o: int) -> list[tuple]:
    """INNER_C2F2(BBP04)."""
    return [
        ("put", o, 0x6c, SPR_ROCK_PEAK_B),
        ("put", o + 4, 0x70, SPR_ROCK_HILL_A),
        ("put", o + 5, 0x80, SPR_ROCK_MOUND_A),
        ("put", o + 14, 0x7c, SPR_ROCK_HILL_B),
    ]


def _put(x: int, y: int, spr: int) -> tuple:
    return ("put", x, y, spr)


# Table des 23 scenes -- transcrite telle quelle depuis le CASE
# (Sav.f_Position MOD 23) OF ... END d'OVL_00FB.PAS (INNER_C377).
def _build_scenes() -> dict[int, list[tuple]]:
    return {
        1: [*_tree_small_b5(0x14, 0x70), *_scatter(5), *_c01c(),
            _put(2, 100, SPR_TREE_MED_C), *_tree_small_a(1, 0xb2)],
        0: [*_tree_small_b5(0x23, 0x72), *_tree_small_b3(0x12, 0x76), *_scatter(5),
            *_bf20(0), *_tree_small_b3(0x36, 0x76), _put(1, 0x9e, SPR_TREE_MED_B),
            *_be1a(), _put(2, 100, SPR_TREE_MED_C), *_tree_small_a(0x43, 0xae)],
        22: [*_tree_small_b3(2, 0x70), *_scatter(5), *_c01c(),
             _put(0x16, 100, SPR_TREE_MED_C), *_tree_small_a(1, 0xb2),
             *_tree_small_a(0x2d, 0x92), *_be1a()],
        9: [*_tree_small_b3(0x29, 0x72), *_tree_small_b5(0x14, 0x76), *_tree_small_b3(1, 0x70),
            *_scatter(5), *_bf20(0), _put(1, 0x9e, SPR_TREE_MED_B), *_be1a(),
            *_tree_small_a(0x43, 0xae)],
        16: [*_c2f2(0x27), *_tree_small_b5(0xe, 0x70), *_scatter(5), *_c01c(),
             _put(0, 100, SPR_TREE_MED_C), *_tree_small_a(1, 0xb2)],
        21: [*_c22c(0xa), *_tree_small_b5(0x26, 0x72), *_scatter(5), *_bf20(0),
             *_tree_small_b3(0x3a, 0x76), _put(1, 0x9e, SPR_TREE_MED_B), *_be1a(),
             _put(0, 100, SPR_TREE_MED_C), *_tree_small_a(0x43, 0xae)],
        19: [*_c2f2(0x24), *_tree_small_b3(2, 0x70), *_scatter(5), *_c01c(),
             _put(0x16, 100, SPR_TREE_MED_C), *_tree_small_a(1, 0xb2),
             *_tree_small_a(0x2d, 0x92), *_be1a()],
        11: [*_c22c(0x12), *_tree_small_b3(0x34, 0x72), *_tree_small_b3(1, 0x70),
             *_scatter(5), *_bf20(0), _put(1, 0x9e, SPR_TREE_MED_B), *_be1a(),
             *_tree_small_a(0x43, 0xae)],
        2: [*_tree_small_b5(1, 0x78), *_tree_small_b3(30, 0x74), *_tree_small_b5(0x2c, 0x72),
            *_scatter(5), *_tree_small_a(1, 0xae), _put(0x49, 0x8e, SPR_TREE_SMALL_A),
            _put(0x40, 0x66, SPR_TREE_MED_C), _put(0x47, 0x9c, SPR_TREE_MED_B)],
        3: [*_tree_small_b5(1, 0x78), *_tree_small_b5(0x1c, 0x72), *_tree_small_b5(0x36, 0x74),
            *_scatter(5), *_tree_small_a(1, 0xae), _put(0x47, 0x9c, SPR_TREE_MED_B)],
        12: [*_tree_small_b3(1, 0x78), *_tree_small_b5(0x16, 0x72), *_tree_small_b5(0x36, 0x76),
             *_scatter(5), *_tree_small_a(0x44, 0xae), *_be1a(), _put(1, 0x9c, SPR_TREE_MED_B)],
        17: [*_tree_small_b5(1, 0x78), *_tree_small_b5(0x1b, 0x72), _put(0x3f, 0x66, SPR_TREE_MED_C),
             _put(0x32, 0x6c, SPR_TREE_MED_C), *_tree_small_a(0x38, 0x94), *_scatter(5),
             _put(0x47, 0x9c, SPR_TREE_MED_B)],
        4: [*_c22c(0), *_c298(40), *_scatter(5), *_c1de(0x2a, 0x90), _put(0x49, 30, SPR_ROCK_SUMMIT)],
        5: [*_c298(0), *_c22c(0x24), *_scatter(3), _put(2, 0xb0, SPR_CREATURE_C),
            _put(0x46, 0xa6, SPR_ROCK_MOUND_A)],
        13: [*_c2f2(0), *_c298(0x2c), *_scatter(4), _put(0x4c, 110, SPR_REED),
             _put(0x46, 0x8c, SPR_TREE_BIG_A), *_c1de(0xa, 160)],
        14: [*_c22c(0xa), *_c2f2(0x2e), *_scatter(5), _put(0x16, 0x9a, SPR_CREATURE_B),
             _put(0x42, 150, SPR_CREATURE_C), _put(0x49, 30, SPR_ROCK_SUMMIT)],
        18: [*_c298(30), *_c2f2(-1), *_c22c(0x30), *_scatter(3), _put(0x40, 0xa4, SPR_CREATURE_B),
             _put(0x48, 0xac, SPR_CREATURE_C), *_be1a()],
        6: [*_c22c(4), *_c2f2(40), *_scatter(4), _put(0x44, 0x32, SPR_ROCK_SUMMIT),
            _put(0x4a, 0x24, SPR_ROCK_SUMMIT), *_bf20(0x46), _put(0x1a, 160, SPR_CREATURE_A),
            *_be1a(), _put(0x36, 0xb0, SPR_ROCK_HILL_A)],
        15: [*_c298(0x22), *_tree_small_b5(1, 0x78), *_tree_small_b3(30, 0x74), *_scatter(5),
             *_tree_small_a(1, 0xae), _put(0x49, 0x8e, SPR_TREE_SMALL_A),
             _put(0x40, 0x66, SPR_TREE_MED_C), _put(0x47, 0x9c, SPR_TREE_MED_B)],
        7: [*_c22c(0x16), *_c2f2(0x24), *_tree_small_b5(1, 0x78), *_scatter(5),
            *_tree_small_a(1, 0xae), _put(0x47, 0x9c, SPR_TREE_MED_B)],
        20: [*_c298(6), *_tree_small_b5(0x20, 0x72), *_tree_small_b5(0x36, 0x76), *_scatter(5),
             _put(0x44, 0x32, SPR_ROCK_SUMMIT), _put(0x4a, 0x24, SPR_ROCK_SUMMIT), *_bf20(0x46),
             *_c1de(0x1a, 160), *_be1a(), _put(0x36, 0xb0, SPR_ROCK_HILL_A)],
        8: [*_c2f2(0x1a), *_tree_small_b5(6, 0x78), _put(0x39, 0x68, SPR_TREE_MED_C),
            *_scatter(5), *_tree_small_a(0x3f, 0x98), _put(0x49, 30, SPR_ROCK_SUMMIT)],
        10: [*_c22c(0x2c), *_c298(0x18), *_c2f2(0xa), *_scatter(5), _put(7, 0x82, SPR_TREE_BIG_A),
             _put(2, 100, SPR_REED), _put(0x46, 0x8a, SPR_TREE_BIG_A), _put(0x38, 0x92, SPR_CREATURE_A),
             *_c1de(0x32, 0x98), _put(0x1a, 0xa4, SPR_CREATURE_A), _put(6, 0xa8, SPR_CREATURE_B)],
    }


SCENES = _build_scenes()
