"""Chargement des vrais sprites extraits (`tools/extract_sprites.py`,
`data/sprites_extracted/crapim/*.png`) pour le rendu Pygame.

**Fiabilite de l'identification** : la table ci-dessous est confirmee par
recoupement direct avec `OVL_00FB.PAS` (scene procedurale d'Amarande,
section 19.1/21 des notes moteur) -- chaque identifiant `SPR_XX` (notation
hexadecimale du Pascal reconstruit) est utilise nommement dans les 23
scenes fixes (`Sav.f_Position MOD 23`), et l'inspection visuelle des PNG
extraits confirme la semantique (arbre/rocher/etc.) dans chaque cas. Seul
`SPR_2D` (porte, section 5.2) etait deja confirme par une reference
textuelle independante.

**Correction par rapport a une session precedente** : `SPR_44`/`SPR_45`
avaient d'abord ete identifies comme des rochers par inspection visuelle
seule ("une forme qui ressemble a un rocher/colline"). En reexaminant le
code source d'`OVL_00FB.PAS`, ces deux identifiants sont en realite des
**arbres** (troncs magenta, feuillage cyan/blanc) -- confirme par
zoom pixel a pixel. Les vrais rochers/silhouettes de montagne du jeu sont
`SPR_2A`/`SPR_2B`/`SPR_2C`/`SPR_39`/`SPR_3A`/`SPR_4F`/`SPR_50`/`SPR_52`.
"""
from __future__ import annotations
from functools import lru_cache
from pathlib import Path

import pygame

SPRITE_DIR = Path(__file__).resolve().parent.parent.parent.parent / "data" / "sprites_extracted" / "crapim"

# Conversion depuis la notation SPR_XX (hexadecimal) du Pascal reconstruit
# vers l'identifiant decimal utilise par tools/extract_sprites.py.
SPR_DOOR = 0x2D             # confirme (section 5.2 des notes moteur)

# -- Arbres (confirmes par usage dans OVL_00FB.PAS + inspection visuelle) --
SPR_TREE_BIG_A = 0x01        # grand arbre orne (branches magenta, feuillage cyan/blanc)
SPR_TREE_SMALL_A = 0x20      # petit arbre (INNER_BDCC)
SPR_TREE_SMALL_B = 0x21      # petit arbre, variante (INNER_BE5A/BEA8)
SPR_TREE_MED_A = 0x22        # arbre moyen (INNER_BF20)
SPR_TREE_MED_B = 0x44        # arbre moyen -- corrige (etait etiquete "rocher" par erreur)
SPR_TREE_MED_C = 0x45        # arbre a large frondaison -- corrige (idem)
SPR_TREE_SIGNATURE = 0x47    # grand arbre "signature" (branches noueuses) -- INNER_C01C

# -- Rochers / silhouettes de montagne --
SPR_ROCK_PEAK_A = 0x2A
SPR_ROCK_PEAK_B = 0x2B
SPR_ROCK_MOUND_A = 0x2C
SPR_ROCK_RIDGE_A = 0x39
SPR_ROCK_MOUND_B = 0x3A
SPR_ROCK_HILL_A = 0x4F
SPR_ROCK_HILL_B = 0x50
SPR_ROCK_SUMMIT = 0x52       # pic enneige, place haut dans le ciel

# -- Autres decors utilises par la table des 23 scenes --
SPR_GROUND_PEBBLE = 0x14     # petit rocher/caillou (decor de sol, INNER_C0EC)
SPR_GROUND_TUFT_A = 0x35     # touffe d'herbe/caillou minuscule (decor de sol)
SPR_GROUND_TUFT_B = 0x36     # idem
SPR_REED = 0x3B              # tige/plante haute et fine (magenta)
SPR_CREATURE_A = 0x27        # creature/carcasse au sol (machoire visible sur SPR_29)
SPR_CREATURE_B = 0x28
SPR_CREATURE_C = 0x29

SKULL_DECORATIONS = [0x05, 0x06, 0x07, 0x08, 0x16, 0x17, 0x18]  # cranes/os, theme "Cite des Cranes"

# -- Villes/batiments -- IDENTIFICATION AU MIEUX, PAS CONFIRMEE --
# Aucune reference textuelle a un sprite de ville n'a ete trouvee dans
# OVL_00FB.PAS : les 20 villes d'Amarande passent soit par une gestion
# directe non decodee (index 0-9), soit par le dispatcher `C_7309`
# (index >9, non decode -- section 19.1/21.3 des notes moteur). Ce
# sprite est choisi par inspection visuelle seule (forme geometrique a
# angles droits avec decoupe porte/fenetre, la seule du lot qui ressemble
# a un batiment plutot qu'a un element naturel) -- a corriger si une
# lecture plus attentive du code contredit ce choix.
SPR_VILLAGE = 0x42

# Conserves pour compatibilite (scene.py Alfol, moins prioritaire -- pas de
# table de scenes confirmee pour OVL_0085) : pool d'arbres/rochers corrige.
SPR_TREE_A = SPR_TREE_SMALL_A
SPR_TREE_B = SPR_TREE_SMALL_B
SPR_ROCK_A = SPR_ROCK_PEAK_A
SPR_ROCK_B = SPR_ROCK_MOUND_A

# -- Indicateur de coffre en couloir -- CONFIRME (OVL_002E.PAS, monde 0
# "Cite des Cranes 1", procedure INNER_C5FC) : des qu'une case a un coffre
# non ouvert (`Sav.f_Maps[pos][5]` ni $ff ni 101), un sprite est affiche --
# SPR_05 si un monstre d'id 3/6/9 y est associe, SPR_3C sinon (avec ou sans
# monstre). Seul ce couple est confirme par reference textuelle directe ;
# le reste de la table ci-dessous (monstres en combat) ne l'est PAS.
SPR_CHEST_MARKER_A = 0x05
SPR_CHEST_MARKER_B = 0x3C

# -- Monstres en combat -- AUCUNE reference confirmee. --
# `SpawnMonster` (routine qui affiche reellement le monstre en combat)
# n'a jamais ete desassemblee/decompilee (section 15/17 des notes
# moteur) : impossible de savoir quel sprite l'original utilise pour
# quel monstre. La table ci-dessous est un choix PUREMENT DECORATIF,
# par ressemblance visuelle avec le nom du monstre (NOMON) -- a ne
# jamais presenter comme une reconstruction fidele. Objectif : donner
# au joueur QUELQUE CHOSE a voir plutot que rien, pas reproduire
# l'original.
SPR_MONSTER_COBRA = 18       # capuchon de cobra tres reconnaissable
SPR_MONSTER_BAT = 3          # silhouette ailee
SPR_MONSTER_DEMON = 4        # figure cornue magenta
SPR_MONSTER_WARRIOR = 14     # humanoide arme/armure
SPR_MONSTER_GHOST = 36       # silhouette encapuchonnee fantomatique
SPR_MONSTER_BRUTE = 64       # creature massive/trapue
SPR_MONSTER_GENERIC = 17     # repli -- insectes/araignees/cristaux... sans equivalent visuel clair

# Mots-cles (sous-chaine, insensible a la casse) -> sprite, dans l'ordre
# de priorite (premier mot-cle trouve dans le nom qui gagne). Les noms
# de `NOMON` contiennent de vrais caracteres accentues UTF-8 ("demon",
# "necromanciens", "creature" s'ecrivent avec e/é) -- une recherche de
# sous-chaine plate sur un mot-cle SANS accent ne matcherait donc jamais
# "prince-démon" ; les mots-cles ci-dessous sont choisis pour rester du
# cote non-accentue du mot (ex. "prince" plutot que "demon", "cromancien"
# plutot que "necromancien").
_MONSTER_KEYWORDS: list[tuple[str, int]] = [
    ("cobra", SPR_MONSTER_COBRA),
    ("naja", SPR_MONSTER_COBRA),
    ("chauve-souris", SPR_MONSTER_BAT),
    ("prince", SPR_MONSTER_DEMON),       # "prince-démon"
    ("chaos", SPR_MONSTER_DEMON),        # "seigneur du CHAOS"
    ("esprit", SPR_MONSTER_GHOST),
    ("cromancien", SPR_MONSTER_GHOST),   # "nécromanciens"
    ("guerrier", SPR_MONSTER_WARRIOR),
    ("garde", SPR_MONSTER_WARRIOR),
    ("cyborg", SPR_MONSTER_WARRIOR),
    ("homme", SPR_MONSTER_WARRIOR),
    ("ature", SPR_MONSTER_BRUTE),        # "créature"
    ("demi-dieu", SPR_MONSTER_BRUTE),
    ("caiman", SPR_MONSTER_BRUTE),
    ("crocodile", SPR_MONSTER_BRUTE),
]


def monster_sprite_id(name: str) -> int:
    """Choix DECORATIF, non confirme -- voir le commentaire au-dessus de
    `_MONSTER_KEYWORDS`. Renvoie `SPR_MONSTER_GENERIC` si aucun mot-cle
    ne correspond."""
    low = name.lower()
    for keyword, sprite_id in _MONSTER_KEYWORDS:
        if keyword in low:
            return sprite_id
    return SPR_MONSTER_GENERIC


@lru_cache(maxsize=None)
def load_sprite(sprite_id: int) -> pygame.Surface | None:
    """Charge (et met en cache) le PNG extrait pour un identifiant de
    sprite. Renvoie None si le fichier n'existe pas (extraction pas
    encore lancee -- voir CLAUDE.md, `python tools/extract_sprites.py`)."""
    path = SPRITE_DIR / f"sprite_{sprite_id:02d}.png"
    if not path.exists():
        return None
    return pygame.image.load(str(path)).convert_alpha()


def load_scaled(sprite_id: int, height: int) -> pygame.Surface | None:
    """Comme `load_sprite`, mis a l'echelle pour faire `height` pixels de
    haut (ratio conserve) -- les sprites originaux sont minuscules
    (quelques pixels, resolution CGA)."""
    base = load_sprite(sprite_id)
    if base is None:
        return None
    if base.get_height() == 0:
        return base
    ratio = height / base.get_height()
    size = (max(1, int(base.get_width() * ratio)), max(1, height))
    return pygame.transform.scale(base, size)
