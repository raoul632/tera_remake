"""Generation procedurale de donjon, portee depuis OVL_0000.

Voir docs/TERA_notes_moteur.md sections 4 et 5.3 pour le detail de
l'algorithme original. Ce module reste fidele a la logique reverse-engineered ;
les points encore approximatifs sont marques TODO.
"""
from __future__ import annotations
import random

from .models import World, MapCell

# Nombre de pas de la marche aleatoire (0xE1 dans l'original)
DIG_STEPS = 0xE1

WALL_NONE, WALL_SIMPLE, WALL_DOOR, WALL_SECRET = 0, 1, 2, 3

# Codes de coffre pour les marqueurs uniques (docs/TERA_notes_moteur.md section 5.3)
CHEST_KEY_BASE = 90        # 90-98 : cle n(valeur-90), une par etage
CHEST_MAP = 99
CHEST_STAIRS_UP = 100
CHEST_STAIRS_DOWN = 101
CHEST_ALTAR = 103           # autel de soin

# Case de sortie de l'etage 0 : ce n'est pas un marqueur de coffre mais une
# position precise (`UNTIL Sav.f_Position = 99` dans l'original) -- il faut
# juste que cette case soit atteignable depuis l'entree du donjon.
EXIT_POSITION = 99

# Marqueurs de la base d'Alfol (monde 9, docs/TERA_notes_moteur.md 19.2/20.6)
# -- meme champ MapCell.chest que les donjons, mais avec un sens different :
# ces codes bas (1/2) n'ont rien a voir avec la table OBJETS ici, c'est
# la meme convention que l'original (f_Maps[pos][5] reinterprete par
# monde/overlay).
CHEST_BUREAU_MEDIAS = 1   # tache 2 : payer 1000 ducats
CHEST_RADIOACTIF = 2      # piege d'ambiance, -1 PV par personnage present

# Marqueurs de Meduz (monde 10, docs/TERA_notes_moteur.md 19.3)
CHEST_MEDITER = 2         # +1 Volonte / +1 Intelligence, une seule fois
CHEST_INVOQUER = 3        # tache 1 : invoquer SecretWords[3] (index 2 ici)

MAX_GENERATION_ATTEMPTS = 500


def _random_wall_quality(rng: random.Random) -> int:
    """Reproduit INNER_BB8C : 1/9 secret, ~1/4 des restants porte, sinon simple."""
    if rng.randrange(9) == 1:
        return WALL_SECRET
    if rng.randrange(4) == 1:
        return WALL_DOOR
    return WALL_SIMPLE


def dig_world(world_id: int, rng: random.Random | None = None) -> World:
    """Creuse un monde 10x10 par marche aleatoire auto-evitante.

    Port de INNER_BBD3 + INNER_BA2F (choix de direction qui evite de
    revenir immediatement en arriere et respecte les bornes de la grille).
    """
    rng = rng or random.Random()
    world = World(id=world_id, cells=[MapCell() for _ in range(100)])
    pos = 0
    last_dir = 3  # D_9B7A initialise a 3 dans l'original

    for _ in range(DIG_STEPS):
        # Choix de direction en evitant de revenir en arriere et les bords
        # (reproduction simplifiee de INNER_BA2F -- a affiner si le comportement
        # exact aux bords doit coller pixel pour pixel a l'original)
        while True:
            direction = rng.randrange(4)
            if direction == last_dir:
                continue
            col = pos % 10
            row = pos // 10
            if direction == 0 and col == 9:   # est, deja au bord
                continue
            if direction == 1 and row == 0:   # nord, deja au bord
                continue
            if direction == 2 and col == 0:   # ouest, deja au bord
                continue
            if direction == 3 and row == 9:   # sud, deja au bord
                continue
            break
        # Met a jour la direction "opposee" interdite au prochain pas
        # (reproduit IF D_9B78 > 1 THEN D_9B7A := D_9B78-2 ELSE D_9B7A := D_9B78+2)
        last_dir = direction - 2 if direction > 1 else direction + 2

        quality = _random_wall_quality(rng)
        if direction == 0:      # est : pos -> pos+1
            world.cells[pos].walls["E"] = quality
            pos += 1
            world.cells[pos].walls["O"] = quality
        elif direction == 1:    # nord : pos -> pos-10
            world.cells[pos].walls["N"] = quality
            pos -= 10
            world.cells[pos].walls["S"] = quality
        elif direction == 2:    # ouest : pos -> pos-1
            world.cells[pos].walls["O"] = quality
            pos -= 1
            world.cells[pos].walls["E"] = quality
        elif direction == 3:    # sud : pos -> pos+10
            world.cells[pos].walls["S"] = quality
            pos += 10
            world.cells[pos].walls["N"] = quality

        world.cells[pos].extra = _random_wall_quality(rng)

    return world


def populate_dungeon(world: World, difficulty: int, is_deep: bool, rng: random.Random | None = None) -> None:
    """Place monstres et coffres (port partiel de INNER_BD96).

    is_deep: False pour les etages superficiels (monstres faibles),
    True pour les etages profonds (monstres plus costauds).
    """
    rng = rng or random.Random()
    for cell in world.cells:
        if rng.randrange(13 - difficulty) == 2:
            cell.encounter = (rng.randrange(11) + 1) if not is_deep else (rng.randrange(13) + 8)
            if is_deep and cell.encounter in (12, 13, 14):
                cell.encounter = None
        if cell.encounter is not None:
            item = rng.randrange(0x4B) + 1
            if item in (0x15, 0x12, 0x11):
                item = 0x19
            cell.chest = item
        elif rng.randrange(7 + difficulty) == 1:
            item = rng.randrange(0x41) + 1
            if item in (0x15, 0x12, 0x11):
                item = 0x19
            cell.chest = item
        else:
            cell.chest = None  # equivalent de $ff (pas de coffre)


def _find_reachable(world: World, start: int = 0) -> set[int]:
    """Cases atteignables depuis `start` par les couloirs creuses.

    Une case par defaut (MapCell()) n'a aucun mur pose (valeur WALL_NONE) :
    ce n'est pas "ouvert", c'est "jamais visite par la marche aleatoire de
    `dig_world`", donc hors du donjon. Un mur pose (1/2/3, quelle que soit
    sa "qualite") marque au contraire une arete effectivement creusee par
    la marche -- cf. section 5.2 : meme un mur "simple" continue le
    couloir, la qualite ne decrit que son traitement visuel (porte,
    secret...), pas un blocage. On explore donc le graphe des cases dont
    le mur commun est != WALL_NONE.
    """
    seen = {start}
    stack = [start]
    while stack:
        pos = stack.pop()
        col, row = pos % 10, pos // 10
        cell = world.cells[pos]
        neighbours = []
        if col < 9 and cell.walls["E"] != WALL_NONE:
            neighbours.append(pos + 1)
        if col > 0 and cell.walls["O"] != WALL_NONE:
            neighbours.append(pos - 1)
        if row > 0 and cell.walls["N"] != WALL_NONE:
            neighbours.append(pos - 10)
        if row < 9 and cell.walls["S"] != WALL_NONE:
            neighbours.append(pos + 10)
        for n in neighbours:
            if n not in seen:
                seen.add(n)
                stack.append(n)
    return seen


def _pick_free_cell(reachable: list[int], used: set[int], rng: random.Random) -> int | None:
    candidates = [pos for pos in reachable if pos not in used]
    if not candidates:
        return None
    return rng.choice(candidates)


def _place_unique_markers(worlds: dict[int, World], num_floors: int, rng: random.Random) -> int | None:
    """Place les marqueurs uniques par etage (docs/TERA_notes_moteur.md 5.3).

    Pour chaque etage : un escalier montant fixe (mondes 2..dernier, a la
    premiere case de l'etage), une cle garantie (codes 90-98, numerotee par
    etage), un autel de soin garanti (103). Chaque etage sauf le dernier
    recoit un escalier descendant (101) ; le dernier recoit Arioch a la
    place -- "toujours au monde 7" dans l'original a 8 etages, donc
    toujours au dernier etage genere ici.

    Retourne la position absolue d'Arioch (etage*100 + case), ou None si
    un etage n'a pas assez de cases atteignables pour tout placer (appelant
    doit alors regenerer, comme la boucle REPEAT...UNTIL de l'original).
    """
    assert num_floors <= 9, "les codes de cle 90-98 ne couvrent que 9 etages"
    pos_arioch: int | None = None

    for floor in range(num_floors):
        world = worlds[floor]
        reachable = sorted(_find_reachable(world))
        used: set[int] = set()

        if floor >= 2:
            # Escalier montant : toujours a la premiere case de l'etage,
            # arithmetique `Sav.f_Position := 100 * WorldId` de l'original.
            world.cells[0].chest = CHEST_STAIRS_UP
            used.add(0)

        key_cell = _pick_free_cell(reachable, used, rng)
        if key_cell is None:
            return None
        world.cells[key_cell].chest = CHEST_KEY_BASE + floor
        used.add(key_cell)

        altar_cell = _pick_free_cell(reachable, used, rng)
        if altar_cell is None:
            return None
        world.cells[altar_cell].chest = CHEST_ALTAR
        used.add(altar_cell)

        if floor == num_floors - 1:
            arioch_cell = _pick_free_cell(reachable, used, rng)
            if arioch_cell is None:
                return None
            world.pos_arioch = arioch_cell
            pos_arioch = floor * 100 + arioch_cell
            used.add(arioch_cell)
        else:
            # Position exacte de l'escalier descendant dans l'original non
            # confirmee (section 15, TERAUTIL.INC manquant) -- ceci
            # garantit juste l'invariant "un escalier par etage 0-6" sans
            # pretendre reproduire l'algorithme exact de selection.
            down_cell = _pick_free_cell(reachable, used, rng)
            if down_cell is None:
                return None
            world.cells[down_cell].chest = CHEST_STAIRS_DOWN
            used.add(down_cell)

    return pos_arioch


def _validate_dungeon(worlds: dict[int, World], num_floors: int) -> bool:
    """Invariants structurels avant d'accepter la generation.

    Equivalent partiel du `REPEAT...UNTIL BPFE3B = 11` de l'original :
    couvre les invariants propres a la Cite des Cranes (escalier par
    etage, Arioch place, sortie de l'etage 0 atteignable). Les mondes 9/10
    (Alfol) ne sont pas generes par cette fonction et ne sont donc pas
    valides ici.
    """
    if EXIT_POSITION not in _find_reachable(worlds[0]):
        return False
    for floor in range(num_floors - 1):
        if not any(cell.chest == CHEST_STAIRS_DOWN for cell in worlds[floor].cells):
            return False
    for floor in range(2, num_floors):
        if worlds[floor].cells[0].chest != CHEST_STAIRS_UP:
            return False
    if worlds[num_floors - 1].pos_arioch is None:
        return False
    return True


def generate_dungeon(num_floors: int, difficulty: int, rng: random.Random | None = None) -> dict[int, World]:
    """Genere un donjon complet a plusieurs etages, avec revalidation.

    Port de haut niveau de la boucle REPEAT...UNTIL BPFE3B = 11 de OVL_0000 :
    on genere puis on verifie des invariants de jouabilite (un escalier par
    etage, Arioch place, une sortie atteignable) avant d'accepter le
    resultat ; sinon on recommence avec le meme generateur aleatoire,
    fidele au comportement original.
    """
    rng = rng or random.Random()
    for _ in range(MAX_GENERATION_ATTEMPTS):
        worlds: dict[int, World] = {}
        for floor in range(num_floors):
            w = dig_world(floor, rng)
            populate_dungeon(w, difficulty, is_deep=(floor >= 2), rng=rng)
            worlds[floor] = w
        pos_arioch = _place_unique_markers(worlds, num_floors, rng)
        if pos_arioch is not None and _validate_dungeon(worlds, num_floors):
            return worlds
    raise RuntimeError(
        f"Impossible de generer un donjon valide a {num_floors} etages "
        f"apres {MAX_GENERATION_ATTEMPTS} tentatives"
    )


def generate_alfol_base(difficulty: int, rng: random.Random | None = None) -> World:
    """Genere la base d'Alfol (monde 9), docs/TERA_notes_moteur.md 19.2/20.6.

    Le creusement (dig_world) et les rencontres/depots radioactifs suivent
    l'algorithme confirme dans OVL_0010.PAS. **Point non confirme** : la
    position exacte du marqueur "BUREAU DES MEDIAS" (tache 2) n'a pas ete
    retrouvee dans OVL_0010 -- probablement placee par la passe generique
    INNER_BD96 de OVL_0000, non verifiee pour le monde 9. Ici, une case
    atteignable est choisie au hasard, ce qui respecte l'invariant "un
    bureau existe et est atteignable" sans pretendre reproduire
    l'algorithme exact de selection (meme esprit que l'escalier
    descendant de la Cite des Cranes, section 5.3).
    """
    rng = rng or random.Random()
    for _ in range(MAX_GENERATION_ATTEMPTS):
        world = dig_world(9, rng)
        reachable = sorted(_find_reachable(world))
        used: set[int] = set()

        bureau_cell = _pick_free_cell(reachable, used, rng)
        if bureau_cell is None:
            continue
        world.cells[bureau_cell].chest = CHEST_BUREAU_MEDIAS
        used.add(bureau_cell)

        for pos in range(100):
            if pos in used:
                continue
            cell = world.cells[pos]
            if rng.randrange(14 - difficulty) == 0:
                cell.chest = CHEST_RADIOACTIF
                used.add(pos)

        for pos in range(100):
            if pos in used:
                continue
            if rng.randrange(12 - difficulty) == 0:
                world.cells[pos].encounter = rng.randrange(3) + 12

        return world
    raise RuntimeError(f"Impossible de generer la base d'Alfol apres {MAX_GENERATION_ATTEMPTS} tentatives")


def generate_meduz(rng: random.Random | None = None) -> World:
    """Genere Meduz (monde 10), docs/TERA_notes_moteur.md 19.3.

    **Point non confirme** : comme pour la base d'Alfol, la position
    exacte des marqueurs "mediter"/"invoquer" n'a pas ete retrouvee dans
    le code lu cette session -- placees ici sur des cases atteignables
    choisies au hasard.
    """
    rng = rng or random.Random()
    for _ in range(MAX_GENERATION_ATTEMPTS):
        world = dig_world(10, rng)
        reachable = sorted(_find_reachable(world))
        used: set[int] = set()

        mediter_cell = _pick_free_cell(reachable, used, rng)
        if mediter_cell is None:
            continue
        world.cells[mediter_cell].chest = CHEST_MEDITER
        used.add(mediter_cell)

        invoquer_cell = _pick_free_cell(reachable, used, rng)
        if invoquer_cell is None:
            continue
        world.cells[invoquer_cell].chest = CHEST_INVOQUER
        used.add(invoquer_cell)

        return world
    raise RuntimeError(f"Impossible de generer Meduz apres {MAX_GENERATION_ATTEMPTS} tentatives")
