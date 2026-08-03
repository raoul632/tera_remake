"""Vue du dessus d'un etage : grille 10x10, un mur dessine la ou le
deplacement est bloque (walls[direction] == WALL_NONE) -- c'est l'inverse
de l'intuition "case creusee = mur dessine", voir CLAUDE.md/notes moteur
section 5.3 sur la semantique des valeurs de mur.

Modernisation assumee (blueprint section 6, "Cartes du monde: mur par
case au lieu de montagnes-lignes") : ce renderer ne connait que le
modele World/MapCell deja utilise pour la Cite des Cranes. Amarande/Alfol
ne sont pas encore generes -- ce renderer leur sera directement
reutilisable le jour ou ils le seront, puisque le blueprint prevoit
justement de les faire passer sur le meme modele de mur par case.
"""
from __future__ import annotations

import pygame

from ..worldgen import CHEST_ALTAR, CHEST_STAIRS_DOWN, CHEST_STAIRS_UP, WALL_NONE

GRID_SIZE = 10

BG_COLOR = (10, 10, 18)
FLOOR_COLOR = (40, 40, 55)
WALL_COLOR = (200, 200, 210)
DOOR_COLOR = (150, 120, 60)
SECRET_COLOR = (90, 60, 110)
PLAYER_COLOR = (255, 210, 60)
STAIRS_UP_COLOR = (80, 200, 120)
STAIRS_DOWN_COLOR = (200, 90, 90)
ALTAR_COLOR = (120, 180, 255)
ARIOCH_COLOR = (220, 30, 30)

WALL_QUALITY_COLOR = {1: WALL_COLOR, 2: DOOR_COLOR, 3: SECRET_COLOR}

_FACING_ARROW = {
    "N": (0, -1), "S": (0, 1), "E": (1, 0), "O": (-1, 0),
}


def draw_topdown(surface: pygame.Surface, world, player_pos: int, facing: str, area: pygame.Rect) -> None:
    surface.fill(BG_COLOR, area)
    cell_size = min(area.width, area.height) // GRID_SIZE
    ox = area.x + (area.width - cell_size * GRID_SIZE) // 2
    oy = area.y + (area.height - cell_size * GRID_SIZE) // 2

    for pos in range(100):
        col, row = pos % GRID_SIZE, pos // GRID_SIZE
        x, y = ox + col * cell_size, oy + row * cell_size
        cell = world.cell(pos)
        rect = pygame.Rect(x, y, cell_size, cell_size)
        pygame.draw.rect(surface, FLOOR_COLOR, rect)

        if cell.walls["N"] == WALL_NONE:
            pygame.draw.line(surface, WALL_COLOR, (x, y), (x + cell_size, y), 2)
        if cell.walls["S"] == WALL_NONE:
            pygame.draw.line(surface, WALL_COLOR, (x, y + cell_size), (x + cell_size, y + cell_size), 2)
        if cell.walls["O"] == WALL_NONE:
            pygame.draw.line(surface, WALL_COLOR, (x, y), (x, y + cell_size), 2)
        if cell.walls["E"] == WALL_NONE:
            pygame.draw.line(surface, WALL_COLOR, (x + cell_size, y), (x + cell_size, y + cell_size), 2)

        marker_color = None
        if cell.chest == CHEST_STAIRS_UP:
            marker_color = STAIRS_UP_COLOR
        elif cell.chest == CHEST_STAIRS_DOWN:
            marker_color = STAIRS_DOWN_COLOR
        elif cell.chest == CHEST_ALTAR:
            marker_color = ALTAR_COLOR
        if getattr(world, "pos_arioch", None) == pos:
            marker_color = ARIOCH_COLOR
        if marker_color is not None:
            center = (x + cell_size // 2, y + cell_size // 2)
            pygame.draw.circle(surface, marker_color, center, max(3, cell_size // 5))

    pcol, prow = player_pos % GRID_SIZE, player_pos // GRID_SIZE
    px = ox + pcol * cell_size + cell_size // 2
    py = oy + prow * cell_size + cell_size // 2
    dx, dy = _FACING_ARROW[facing]
    tip = (px + dx * cell_size * 0.4, py + dy * cell_size * 0.4)
    back_l = (px - dy * cell_size * 0.3 - dx * cell_size * 0.2, py + dx * cell_size * 0.3 - dy * cell_size * 0.2)
    back_r = (px + dy * cell_size * 0.3 - dx * cell_size * 0.2, py - dx * cell_size * 0.3 - dy * cell_size * 0.2)
    pygame.draw.polygon(surface, PLAYER_COLOR, [tip, back_l, back_r])
