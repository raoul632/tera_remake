"""Amarande : carte du monde, montagnes, villes, mots secrets.

Port de OVL_0010.PAS (generation) et OVL_00FB.PAS (deplacement), voir
docs/TERA_notes_moteur.md sections 19-20 pour le detail complet.

Amarande N'EST PAS un World/MapCell comme les donjons : c'est un espace
de 900 positions absolues (1000-1899) sans mur par case, bloque
uniquement par des chaines de montagnes lineaires. La grille 30x30 est
indexee column-major (+1/-1 = nord/sud, +30/-30 = est/ouest) et se
reboucle -- mais PAS comme un tore propre par ligne/colonne : le
rebouclage ne se declenche qu'aux bornes absolues 1000/1899 (voir
INNER_C377 dans OVL_00FB, porte telle quelle ci-dessous, y compris ce
comportement un peu surprenant plutot que de le "corriger").
"""
from __future__ import annotations
import random

POSITION_MIN = 1000
POSITION_MAX = 1899
GRID_HEIGHT = 30  # hauteur d'une colonne -- pas de rebouclage par colonne, voir docstring

DIRECTIONS = {"N": 1, "S": -1, "E": 30, "O": -30}

# Points de passage fixes sur Amarande/Alfol -- simplification assumee
# pour ce port (voir docstring de game.py) : l'original place ces points
# aleatoirement via un systeme de recrutement de PNJ non implemente
# (docs/TERA_notes_moteur.md section 20.3). Centralises ici pour que
# game.py ET le rendu (render/scene.py) partagent la meme source.
AMARANDE_CITE_ENTRANCE = 1000
AMARANDE_PYRAMIDE = 1050
AMARANDE_ALFOL = 1100
ALFOL_BASE_ENTRANCE = 50  # position sur la surface d'Alfol (0-99)

# 3 blocs de syllabes pour les mots secrets (OVL_0010.PAS INNER_BBA5),
# fantaisie a consonance mystique -- entierement reproductible.
_SYLLABLES_1 = ["EL", "A", "HA"]
_SYLLABLES_2 = ["KHY", "MA", "KHA", "RAH", "THY", "TOH"]
_SYLLABLES_3 = ["HAM", "HAAR", "LORK", "RMA", "LU"]


def generate_mountains(rng: random.Random) -> tuple[list[int], list[int]]:
    """Port de OVL_0010.PAS : une chaine H de 9 cases contigues (colonne),
    cinq chaines V de 5 cases contigues chacune (ligne)."""
    pos_h = [POSITION_MIN + rng.randrange(255)]
    for _ in range(8):
        pos_h.append(pos_h[-1] + 30)

    pos_v: list[int] = []
    for _ in range(5):
        start = POSITION_MIN + 3 * rng.randrange(255)
        chain = [start]
        for _ in range(4):
            chain.append(chain[-1] + 1)
        pos_v.extend(chain)

    return pos_h, pos_v


def generate_towns(rng: random.Random, count: int = 20) -> list[int]:
    """Port de OVL_0010.PAS : positions uniques 1000 + 4*RANDOM(225)."""
    towns: list[int] = []
    while len(towns) < count:
        candidate = POSITION_MIN + 4 * rng.randrange(225)
        if candidate not in towns:
            towns.append(candidate)
    return towns


def generate_secret_words(rng: random.Random, count: int = 4) -> list[str]:
    """Port de OVL_0010.PAS INNER_BBA5. Indexation Pascal 1-based dans les
    notes moteur (SecretWords[1..4]) -- ici liste Python 0-indexee, donc
    SecretWords[n] (Pascal) == generate_secret_words()[n - 1]."""
    return [
        rng.choice(_SYLLABLES_1) + rng.choice(_SYLLABLES_2) + rng.choice(_SYLLABLES_3)
        for _ in range(count)
    ]


def direction_hint(position: int, target: int) -> str:
    """Repere grossier vers `target` (entree de donjon, ville...) --
    direction cardinale/intercardinale + distance de Manhattan sur la
    grille 30x30. Ignore les montagnes et le rebouclage non-torique
    (docstring du module) : une aide de navigation approximative pour le
    joueur, pas un calcul de plus court chemin. Ajoutee suite a un retour
    de Merci -- avec les points de passage fixes de ce port (voir plus
    haut), un point unique dans une grille de 900 cases est difficile a
    retrouver sans aucun repere une fois qu'on s'en est eloigne."""
    if position == target:
        return "ici"
    row = (position - POSITION_MIN) % GRID_HEIGHT
    col = (position - POSITION_MIN) // GRID_HEIGHT
    t_row = (target - POSITION_MIN) % GRID_HEIGHT
    t_col = (target - POSITION_MIN) // GRID_HEIGHT
    d_row, d_col = t_row - row, t_col - col
    ns = "N" if d_row > 0 else "S" if d_row < 0 else ""
    eo = "E" if d_col > 0 else "O" if d_col < 0 else ""
    return f"{abs(d_row) + abs(d_col)}{ns}{eo}"


def try_move(position: int, direction: str, pos_h_mountains: list[int], pos_v_mountains: list[int]) -> tuple[int, bool]:
    """Port de INNER_C377 (OVL_00FB.PAS). Renvoie (nouvelle_position, deplace).

    Comportement fidele, y compris le rebouclage un peu surprenant : il
    ne se declenche qu'aux bornes absolues 1000/1899 du tableau, pas par
    ligne/colonne independamment -- voir la docstring du module.
    """
    if direction in ("N", "S"):
        blocked_positions = pos_h_mountains
        check_pos = position if direction == "N" else position - 1
    else:
        blocked_positions = pos_v_mountains
        check_pos = position if direction == "E" else position - 30

    if check_pos in blocked_positions:
        return position, False

    new_position = position + DIRECTIONS[direction]
    if new_position > POSITION_MAX:
        new_position -= 900
    elif new_position < POSITION_MIN:
        new_position += 900
    return new_position, True
