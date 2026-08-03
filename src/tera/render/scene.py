"""Vue "scene" pour Amarande et la surface d'Alfol -- PAS une carte vue
du dessus.

Confirme par lecture de OVL_00FB.PAS/OVL_0085.PAS (docs/TERA_notes_moteur.md
sections 5.1/19) : le joueur ne voit jamais de carte abstraite sur ces
deux lieux. A chaque case, l'original affiche une scene plein ecran
(decor procedural choisi par `position MOD 23`, silhouette de montagne
en vectoriel si une chaine est adjacente) -- "l'image change image par
image" au fil du deplacement, jamais un plan d'ensemble avec un point
de position.

**Amarande** est maintenant un port fidele : `amarande_scenes.SCENES`
reproduit exactement les 23 scenes fixes d'`OVL_00FB.PAS` (coordonnees et
identifiants de sprites recopies du Pascal, pas un generateur aleatoire).
Voir la docstring d'`amarande_scenes.py` pour le detail et les points non
confirmes (DrawMountains, ancre exacte de PutSprite_off).

**Alfol (surface)** n'a pas de table equivalente confirmee (`OVL_0085.PAS`
n'a pas ete depouille au meme niveau de detail) -- reste sur la
modernisation precedente (vegetation placee par un generateur
pseudo-aleatoire seede par la position, blueprint section 6).
"""
from __future__ import annotations
import random

import pygame

from ..amarande import (
    AMARANDE_CITE_ENTRANCE, AMARANDE_PYRAMIDE, AMARANDE_ALFOL, ALFOL_BASE_ENTRANCE,
)
from .sprites import load_sprite, load_scaled, SPR_TREE_A, SPR_TREE_B, SPR_ROCK_A, SPR_ROCK_B, SPR_VILLAGE
from .amarande_scenes import SCENES

SKY_COLOR = (0, 0, 0)  # CGA : pas de degrade sky/sol, fond noir uniforme (confirme visuellement)
GROUND_COLOR = (0, 0, 0)
STAR_COLOR = (255, 255, 255)
HORIZON_COLOR = (255, 85, 255)   # magenta CGA (couleur 2), DRAW(...,2) dans INNER_BA2F/BBA3
MOUNTAIN_COLOR = (70, 70, 95)
TREE_TRUNK_COLOR = (110, 80, 60)
TREE_LEAF_COLOR = (60, 140, 90)
ROCK_COLOR = (90, 90, 100)
LANDMARK_COLOR = (220, 190, 90)

# Espace de coordonnees original (CGA, OVL_00FB.PAS) -- voir amarande_scenes.py.
ORIGIN_W, ORIGIN_H = 320, 200

# Zoom supplementaire au-dela de la simple mise a l'echelle CGA->area :
# retour de Merci sur une premiere version de ce port ("les sprites sont
# trop petits/perdus dans l'ecran") -- les sprites originaux sont minuscules
# en resolution CGA native, et le remake affiche la scene dans une fenetre
# bien plus grande que 320x200 avec beaucoup d'espace vide autour. Ancre en
# BAS (voir `_origin_scale`) pour agrandir le premier plan (arbres/rochers,
# concentres pres du sol) plutot que le ciel -- au prix de rogner le haut de
# l'image (une partie des etoiles disparait au zoom le plus fort).
SCENE_ZOOM = 1.3


def _local_rng(position: int) -> random.Random:
    """Meme position -> meme scene a chaque visite (pas un vrai hasard)."""
    return random.Random(position * 2654435761 & 0xFFFFFFFF)


def _draw_sky_and_ground(surface: pygame.Surface, area: pygame.Rect, rng: random.Random) -> int:
    surface.fill(SKY_COLOR, area)
    ground_y = area.y + int(area.height * 0.6)
    pygame.draw.rect(surface, GROUND_COLOR, (area.x, ground_y, area.width, area.bottom - ground_y))
    for _ in range(24):
        x = area.x + rng.randrange(area.width)
        y = area.y + rng.randrange(int(area.height * 0.45))
        pygame.draw.circle(surface, STAR_COLOR, (x, y), 1)
    return ground_y


def _draw_mountain_silhouette(surface: pygame.Surface, area: pygame.Rect, ground_y: int, rng: random.Random) -> None:
    """Remplace `DrawMountains`, une primitive residente jamais retrouvee
    (section 15 des notes moteur) -- silhouette vectorielle approximative,
    affichee en plus de la ligne d'horizon fidele quand une chaine de
    montagnes est adjacente."""
    points = [(area.x, ground_y)]
    x = area.x
    while x < area.right:
        x += rng.randrange(30, 70)
        peak_h = rng.randrange(30, 90)
        points.append((min(x, area.right), ground_y - peak_h))
    points.append((area.right, ground_y))
    pygame.draw.polygon(surface, MOUNTAIN_COLOR, points)


def _draw_tree(surface: pygame.Surface, x: int, ground_y: int, rng: random.Random) -> None:
    height = rng.randrange(24, 48)
    sprite_id = rng.choice((SPR_TREE_A, SPR_TREE_B))
    sprite = load_scaled(sprite_id, height)
    if sprite is not None:
        surface.blit(sprite, (x - sprite.get_width() // 2, ground_y - sprite.get_height()))
        return
    trunk_w = max(2, height // 10)
    pygame.draw.rect(surface, TREE_TRUNK_COLOR, (x - trunk_w // 2, ground_y - height // 3, trunk_w, height // 3))
    pygame.draw.polygon(surface, TREE_LEAF_COLOR, [
        (x, ground_y - height), (x - height // 3, ground_y - height // 3), (x + height // 3, ground_y - height // 3),
    ])


def _draw_rock(surface: pygame.Surface, x: int, ground_y: int, rng: random.Random) -> None:
    r = rng.randrange(6, 16)
    sprite_id = rng.choice((SPR_ROCK_A, SPR_ROCK_B))
    sprite = load_scaled(sprite_id, r * 2)
    if sprite is not None:
        surface.blit(sprite, (x - sprite.get_width() // 2, ground_y - sprite.get_height()))
        return
    pygame.draw.circle(surface, ROCK_COLOR, (x, ground_y - r // 2), r)


def _draw_landmark(surface: pygame.Surface, area: pygame.Rect, ground_y: int, font: pygame.font.Font, label: str) -> None:
    """Marqueur d'un lieu notable (ville/Cite/Pyramide/Alfol) sur la case
    actuelle. Utilise `SPR_VILLAGE` (identification au mieux, non
    confirmee -- voir sprites.py) avec repli sur une forme vectorielle si
    le PNG n'existe pas."""
    cx = area.centerx
    sprite = load_scaled(SPR_VILLAGE, 120)
    if sprite is not None:
        rect = sprite.get_rect(midbottom=(cx, ground_y))
        surface.blit(sprite, rect)
    else:
        width, height = 70, 90
        rect = pygame.Rect(cx - width // 2, ground_y - height, width, height)
        pygame.draw.rect(surface, LANDMARK_COLOR, rect, 3)
        pygame.draw.polygon(surface, LANDMARK_COLOR, [
            (rect.left - 10, rect.top), (rect.centerx, rect.top - 40), (rect.right + 10, rect.top),
        ], 3)
    surface.blit(font.render(label, True, LANDMARK_COLOR), (rect.x, rect.bottom + 4))


def _mountain_adjacent_amarande(save, pos: int) -> bool:
    checks = [pos, pos - 1]  # N/S (f_PosHMountains)
    if any(c in save.pos_h_mountains for c in checks):
        return True
    checks = [pos, pos - 30]  # E/O (f_PosVMountains)
    return any(c in save.pos_v_mountains for c in checks)


def _amarande_landmark(save, pos: int) -> str | None:
    if pos == AMARANDE_CITE_ENTRANCE:
        return "Cite des Cranes"
    if pos == AMARANDE_PYRAMIDE:
        return "Pyramide"
    if pos == AMARANDE_ALFOL:
        return "Alfol"
    if pos in save.loc_towns:
        return "Ville"
    return None


# --- Port fidele de la scene Amarande (OVL_00FB.PAS) --------------------

# `PutSprite_off`/`FillRect_off` prennent leur coordonnee X en OCTETS, pas
# en pixels -- confirme en comparant TOUS les appels PutSprite(_off) des
# overlays decompiles (pas seulement OVL_00FB) : l'argument X ne depasse
# jamais ~80 dans tout le code source, alors que Y va normalement jusqu'a
# ~190 ; `FillRect_off` utilise meme la constante litterale `80` comme
# "bord droit de l'ecran". Coherent avec le format des sprites eux-memes
# (largeur stockee EN OCTETS dans CRAPIM.IMA, 4 pixels CGA par octet,
# 320px / 4 = 80 octets par ligne) : la routine de blit adresse
# directement la memoire video par octet, comme le fait le chargement de
# sprite lui-meme (section 10 des notes moteur). `DRAW`/`PLOT` (utilises
# par la ligne d'horizon/les etoiles, section 21.2) restent en revanche
# des coordonnees PIXEL classiques (vont jusqu'a 319) -- deux primitives
# differentes, deux conventions differentes.
#
# Bug corrige suite au retour de Merci ("les sprites de decor sont
# toujours packes en bas a gauche") : la premiere version de ce port
# traitait X comme un pixel brut, ce qui compressait tout le decor (dont
# les coordonnees originales vont bien jusqu'a ~76-80) dans le quart
# gauche de l'ecran au lieu de l'etaler sur (presque) toute la largeur.
SPRITE_X_UNIT = 4


def _origin_scale(area: pygame.Rect) -> tuple[float, int, int]:
    """Echelle (avec zoom, voir `SCENE_ZOOM`) + decalages pour placer
    l'espace original 320x200 dans `area` : centre horizontalement, ancre
    en BAS verticalement (`y_offset` <= 0 -- rogne le ciel, pas le sol)."""
    scale = (area.height / ORIGIN_H) * SCENE_ZOOM
    x_offset = round((area.width - ORIGIN_W * scale) / 2)
    y_offset = round(area.height - ORIGIN_H * scale)
    return scale, x_offset, y_offset


def _put_sprite(surface: pygame.Surface, area: pygame.Rect, scale: float, x_offset: int, y_offset: int,
                 x: int, y: int, sprite_id: int) -> None:
    base = load_sprite(sprite_id)
    if base is None:
        return
    target_h = max(1, round(base.get_height() * scale))
    sprite = load_scaled(sprite_id, target_h)
    if sprite is None:
        return
    px = area.x + x_offset + round(x * SPRITE_X_UNIT * scale)
    py = area.y + y_offset + round(y * scale)
    surface.blit(sprite, (px, py))


def _fill_rect(surface: pygame.Surface, area: pygame.Rect, scale: float, x_offset: int, y_offset: int,
               x1: int, y1: int, x2: int, y2: int) -> None:
    """Port de `FillRect_off(...,0)` -- toujours noir dans OVL_00FB.PAS,
    utilise pour "decouper" des bosquets de sprites superposes. x1/x2 en
    octets comme `PutSprite_off` (voir `SPRITE_X_UNIT`)."""
    left = area.x + x_offset + round(min(x1, x2) * SPRITE_X_UNIT * scale)
    top = area.y + y_offset + round(min(y1, y2) * scale)
    right = area.x + x_offset + round(max(x1, x2) * SPRITE_X_UNIT * scale)
    bottom = area.y + y_offset + round(max(y1, y2) * scale)
    pygame.draw.rect(surface, (0, 0, 0), pygame.Rect(left, top, max(1, right - left), max(1, bottom - top)))


def _draw_ground_scatter(surface: pygame.Surface, area: pygame.Rect, scale: float, x_offset: int, y_offset: int,
                          count: int, rng: random.Random) -> None:
    """Port de INNER_C0EC(count) : cailloux/touffes d'herbe disperses en
    bas de la scene, sur presque toute la largeur une fois convertis en
    pixels (x aleatoire 0-75 OCTETS = 0-300 pixels, y aleatoire pair
    148-186 pixels -- reproduit tel quel, pas une modernisation)."""
    from .sprites import SPR_GROUND_PEBBLE, SPR_GROUND_TUFT_A, SPR_GROUND_TUFT_B
    for _ in range(count):
        for sprite_id, n in ((SPR_GROUND_PEBBLE, 4), (SPR_GROUND_TUFT_A, 4), (SPR_GROUND_TUFT_B, 5)):
            for _ in range(n):
                x = rng.randrange(0x4c)
                y = 2 * rng.randrange(0x14) + 0x94
                _put_sprite(surface, area, scale, x_offset, y_offset, x, y, sprite_id)


def _draw_stars_and_horizon(surface: pygame.Surface, area: pygame.Rect, scale: float, x_offset: int, y_offset: int,
                             rng: random.Random) -> None:
    """Port de INNER_BD0C : 16 etoiles aleatoires + une des deux
    silhouettes d'horizon en polyligne magenta (INNER_BA2F/INNER_BBA3).
    Le `Circle`/`FillShape` conditionnel a la derniere touche pressee
    (`D_A679='6'`) n'est pas reproduit (fonction visuelle mineure et
    incertaine -- voir docstring d'amarande_scenes.py). Avec `SCENE_ZOOM`
    > 1, une partie des etoiles (les plus hautes) sort du cadre -- cout
    accepte pour agrandir le premier plan.
    """
    for _ in range(16):
        x = area.x + x_offset + round((10 + rng.randrange(300)) * scale)
        y = area.y + y_offset + round((4 + rng.randrange(0x46)) * scale)
        surface.set_at((x, y), STAR_COLOR) if area.collidepoint(x, y) else None

    h = rng.randrange(20)
    if rng.randrange(2) == 0:
        # INNER_BA2F(h) : deux polylignes DISTINCTES (pas une seule chaine
        # continue -- la 2e commence a x=180, avant la fin de la 1ere a
        # x=220, donc elles se chevauchent en x sans etre reliees).
        raw_polylines = [
            [(0, 126), (80, 126 - h), (110, 126 - h - 1), (0x8c, 126 - h), (0xdc, 126)],
            [(0xb4, 126 - h // 2), (0xf0, 126 - (2 * h) // 3), (280, 126 - (3 * h) // 5), (319, 126 - h // 3)],
        ]
    else:
        # INNER_BBA3(h) : idem, deux polylignes distinctes.
        raw_polylines = [
            [(0, 126), (60, 126 - h // 2), (100, 126 - h // 2 - 2), (160, 126 - h // 2), (200, 126)],
            [(0xb4, 126 - h // 4), (0xf0, 126 - h), (280, 126 - h), (319, 126 - (3 * h) // 5)],
        ]
    for raw_points in raw_polylines:
        points = [(area.x + x_offset + round(px * scale), area.y + y_offset + round(py * scale)) for px, py in raw_points]
        pygame.draw.lines(surface, HORIZON_COLOR, False, points, 1)


def draw_amarande_scene(surface: pygame.Surface, save, area: pygame.Rect, font: pygame.font.Font) -> None:
    pos = save.amarande_position
    rng = _local_rng(pos)
    scale, x_offset, y_offset = _origin_scale(area)
    ground_y = area.y + y_offset + round(126 * scale)

    prev_clip = surface.get_clip()
    surface.set_clip(area)
    try:
        surface.fill(SKY_COLOR, area)
        _draw_stars_and_horizon(surface, area, scale, x_offset, y_offset, rng)

        if _mountain_adjacent_amarande(save, pos):
            _draw_mountain_silhouette(surface, area, ground_y, rng)

        for call in SCENES[pos % 23]:
            kind = call[0]
            if kind == "put":
                _, x, y, sprite_id = call
                _put_sprite(surface, area, scale, x_offset, y_offset, x, y, sprite_id)
            elif kind == "fill":
                _, x1, y1, x2, y2 = call
                _fill_rect(surface, area, scale, x_offset, y_offset, x1, y1, x2, y2)
            elif kind == "scatter":
                _, count, _key = call
                _draw_ground_scatter(surface, area, scale, x_offset, y_offset, count, rng)

        label = _amarande_landmark(save, pos)
        if label:
            _draw_landmark(surface, area, ground_y, font, label)
    finally:
        surface.set_clip(prev_clip)


def draw_shop_scene(surface: pygame.Surface, area: pygame.Rect, font: pygame.font.Font, money: int) -> None:
    """Boutique generique de ville (port simplifie de OVL_00BC, section 7
    des notes moteur) -- toutes les villes d'Amarande partagent cet ecran,
    faute d'avoir retrouve le dispatcher d'entree par ville (`C_7309`,
    non decode, voir CLAUDE.md/section 19.1)."""
    from ..game import SHOP_ITEMS

    surface.fill((10, 10, 30), area)
    line_h = font.get_height() + 2
    surface.blit(font.render(f"BOUTIQUE -- {money} ducats", True, LANDMARK_COLOR), (area.x + 10, area.y + 10))

    col_x = [area.x + 10, area.x + area.width // 2 + 10]
    half = (len(SHOP_ITEMS) + 1) // 2
    for i, (letter, name, price) in enumerate(SHOP_ITEMS):
        col, row = divmod(i, half)
        x = col_x[col]
        y = area.y + 10 + line_h * 2 + row * line_h
        surface.blit(font.render(f"[{letter}] {name} -- {price}", True, (220, 220, 230)), (x, y))


def draw_alfol_surface_scene(surface: pygame.Surface, save, area: pygame.Rect, font: pygame.font.Font) -> None:
    pos = save.alfol_surface_position
    rng = _local_rng(pos + 100_000)  # decale du seed Amarande (positions 0-99 sinon identiques)
    ground_y = _draw_sky_and_ground(surface, area, rng)

    for _ in range(rng.randrange(3, 7)):
        _draw_tree(surface, area.x + rng.randrange(area.width), ground_y, rng)
    for _ in range(rng.randrange(1, 3)):
        _draw_rock(surface, area.x + rng.randrange(area.width), ground_y, rng)

    if pos == ALFOL_BASE_ENTRANCE:
        _draw_landmark(surface, area, ground_y, font, "Base d'Alfol")
