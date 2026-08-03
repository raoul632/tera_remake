"""Vue couloir 1ere personne -- port du principe de INNER_BFCB/INNER_C211
(section 5.3 des notes moteur) : plusieurs plans de mur qui s'enfoncent
en profondeur vers un point de fuite, pas un mur unique par ecran.

**Style CONFIRME (retrouve dans `OVL_002E.PAS`, "WORLD 0 - cite des cranes
1")** : les murs ne sont JAMAIS des polygones pleins -- l'original les
dessine en **filigrane** (lignes `DRAW(...,2)`, couleur CGA 2 = magenta,
jamais de remplissage) formant des rectangles emboites qui reculent vers
un point de fuite (`INNER_BFCB`/`INNER_C211` dessinent litteralement une
suite de segments en zigzag pour le bord recule gauche/droit ;
`INNER_C1A4`/`INNER_BB70`/`INNER_BBB8` assemblent ces bords avec le cadre
de chaque profondeur pour former le tunnel). Fond noir uni (pas de sol/
plafond colore -- meme convention CGA que `scene.py`, confirme
visuellement par la capture d'ecran de reference).

Fidelite assumee : le PRINCIPE (delta de position -> direction -> mur
gauche/droite/face relatifs, perspective a plans multiples, **rendu en
filigrane magenta**) est repris tel que documente. Les coordonnees EXACTES
de l'original (calculs tres specifiques a chaque profondeur/cote dans
`OVL_002E.PAS`, propres au monde 0 -- chaque monde a sa propre copie
quasi-identique de cet overlay, section 5.2) ne sont pas reproduites
pixel pour pixel : notre systeme de plans generique (`compute_planes`)
est une modernisation geometrique approximative qui capture le meme
PRINCIPE (emboitement de rectangles vers un point de fuite) sans recopier
les coordonnees figees d'un seul monde (blueprint section 6).
"""
from __future__ import annotations
from dataclasses import dataclass

import pygame

from ..worldgen import WALL_NONE, WALL_DOOR, CHEST_STAIRS_UP, CHEST_STAIRS_DOWN, CHEST_ALTAR
from .sprites import load_scaled, SPR_DOOR, SPR_CHEST_MARKER_A, SPR_CHEST_MARKER_B, monster_sprite_id

CLOCKWISE = ["N", "E", "S", "O"]
DELTA = {"N": -10, "S": 10, "E": 1, "O": -1}
MAX_DEPTH = 4
DEPTH_SHRINK = 0.62

BG_COLOR = (0, 0, 0)               # CGA : fond noir uni, pas de sol/plafond colore
WALL_COLOR = (255, 85, 255)        # magenta CGA (couleur 2) -- DRAW(...,2) dans OVL_002E.PAS
SIDE_WALL_COLOR = WALL_COLOR
LINE_WIDTH = 1                      # filigrane : contour seul, jamais de remplissage


def turn_right(facing: str) -> str:
    return CLOCKWISE[(CLOCKWISE.index(facing) + 1) % 4]


def turn_left(facing: str) -> str:
    return CLOCKWISE[(CLOCKWISE.index(facing) - 1) % 4]


@dataclass
class Plane:
    left_blocked: bool
    right_blocked: bool
    face_blocked: bool
    face_is_door: bool = False


def compute_planes(world, pos: int, facing: str) -> list[Plane]:
    """Renvoie jusqu'a MAX_DEPTH plans devant le joueur.

    S'arrete des qu'un mur de face bloque le passage (fin de couloir) ou
    que le bord de la grille est atteint.
    """
    left_dir = turn_left(facing)
    right_dir = turn_right(facing)
    planes: list[Plane] = []
    current = pos

    for _ in range(MAX_DEPTH):
        cell = world.cell(current)
        face_wall = cell.walls[facing]
        face_blocked = face_wall == WALL_NONE
        planes.append(Plane(
            left_blocked=cell.walls[left_dir] == WALL_NONE,
            right_blocked=cell.walls[right_dir] == WALL_NONE,
            face_blocked=face_blocked,
            face_is_door=face_wall == WALL_DOOR,
        ))
        if face_blocked:
            break
        col, row = current % 10, current // 10
        if (facing == "E" and col == 9) or (facing == "O" and col == 0) \
                or (facing == "N" and row == 0) or (facing == "S" and row == 9):
            break
        current += DELTA[facing]

    return planes


def _plane_rect(area: pygame.Rect, depth: int) -> pygame.Rect:
    scale = DEPTH_SHRINK ** depth
    w, h = area.width * scale, area.height * scale
    cx, cy = area.centerx, area.centery
    return pygame.Rect(cx - w / 2, cy - h / 2, w, h)


def draw_corridor(surface: pygame.Surface, planes: list[Plane], area: pygame.Rect) -> None:
    """Tunnel en filigrane -- fond noir uni, murs en contours magenta
    uniquement (jamais de remplissage), rectangles de chaque profondeur
    emboites vers un point de fuite, meme quand le passage est ouvert
    (l'original dessine le cadre de la scene a chaque profondeur, pas
    seulement les murs pleins -- section 5.2/OVL_002E.PAS)."""
    surface.fill(BG_COLOR, area)

    near = area
    for depth, plane in enumerate(planes):
        far = _plane_rect(area, depth + 1)

        if plane.left_blocked:
            pygame.draw.polygon(surface, SIDE_WALL_COLOR, [
                (near.left, near.top), (near.left, near.bottom),
                (far.left, far.bottom), (far.left, far.top),
            ], LINE_WIDTH)
        if plane.right_blocked:
            pygame.draw.polygon(surface, SIDE_WALL_COLOR, [
                (near.right, near.top), (near.right, near.bottom),
                (far.right, far.bottom), (far.right, far.top),
            ], LINE_WIDTH)
        if plane.face_blocked:
            pygame.draw.rect(surface, WALL_COLOR, far, LINE_WIDTH)
            break

        # Cadre de la profondeur, meme si le couloir continue -- c'est ce
        # cadre repete qui donne l'effet de tunnel en filigrane emboite.
        pygame.draw.rect(surface, WALL_COLOR, far, LINE_WIDTH)

        if plane.face_is_door:
            # SPR_DOOR est un sprite minuscule et large (8x6px natif) --
            # le mettre a l'echelle de TOUTE la hauteur du mur (comme
            # avant) le transformait en gros aplat cyan/blanc qui
            # ressemblait a un fond bizarre plutot qu'a une porte (retour
            # de Merci). Le limiter a une fraction de la hauteur du mur
            # le fait lire comme un accent de porte, avec le filigrane
            # magenta visible tout autour.
            door_sprite = load_scaled(SPR_DOOR, max(1, int(far.height * 0.45)))
            if door_sprite is not None:
                surface.blit(door_sprite, (far.centerx - door_sprite.get_width() // 2, far.bottom - door_sprite.get_height()))

        near = far


def draw_chest_marker(surface: pygame.Surface, area: pygame.Rect, cell, encounter_id: int | None) -> None:
    """Indicateur de coffre non ouvert sur la case actuelle -- CONFIRME
    (`OVL_002E.PAS`, monde 0, `INNER_C5FC`) : `SPR_05` si un monstre
    d'id 3/6/9 y est associe, `SPR_3C` sinon (avec ou sans monstre).
    Affiche des qu'un coffre existe (`cell.chest` pose), pas seulement
    quand on peut le voir depuis loin -- l'original l'affiche au premier
    plan de la vue couloir."""
    if cell.chest is None or cell.chest in (CHEST_STAIRS_UP, CHEST_STAIRS_DOWN, CHEST_ALTAR):
        return
    sprite_id = SPR_CHEST_MARKER_A if encounter_id in (3, 6, 9) else SPR_CHEST_MARKER_B
    sprite = load_scaled(sprite_id, int(area.height * 0.22))
    if sprite is None:
        return
    surface.blit(sprite, (area.centerx - sprite.get_width() // 2, area.bottom - sprite.get_height() - 8))


def draw_monster_encounter(surface: pygame.Surface, area: pygame.Rect, font: pygame.font.Font, monster) -> None:
    """Affiche le monstre rencontre au premier plan pendant le combat.

    **NON confirme** : `SpawnMonster` (la routine qui affiche reellement
    le monstre en combat dans l'original) n'a jamais ete desassemblee
    (section 15/17 des notes moteur) -- le sprite choisi ici est une
    association PUREMENT DECORATIVE par ressemblance visuelle avec le nom
    du monstre (voir `sprites.py::monster_sprite_id`), pas une
    reconstruction fidele. Objectif : donner quelque chose a voir plutot
    que rien."""
    sprite_id = monster_sprite_id(monster.name)
    sprite = load_scaled(sprite_id, int(area.height * 0.55))
    if sprite is not None:
        rect = sprite.get_rect(midbottom=(area.centerx, area.bottom - 12))
        surface.blit(sprite, rect)
    label = font.render(monster.name, True, (230, 210, 90))
    surface.blit(label, (area.centerx - label.get_width() // 2, area.y + 8))
