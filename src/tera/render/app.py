"""Application Pygame : Amarande -> Alfol -> Cite des Cranes/Meduz, avec
le meme SaveGame et les memes fonctions de resolution que la boucle de
jeu textuelle (game.py) -- aucune logique dupliquee, juste une couche de
presentation par-dessus.

Tout se passe dans la fenetre Pygame, y compris le combat/les mots
secrets/le paiement : `game.py` accepte des callbacks `ask`/`log`
injectables (par defaut `input`/`print` pour le CLI), et ce module leur
substitue une ligne de commande + un journal DANS la fenetre (voir
`textinput.py`) -- confirme par le manuel officiel (section 18 des
notes moteur) : l'original n'a jamais de console separee, un seul ecran
avec une ligne de commande en bas.

Lancer avec l'interpreteur qui a pygame installe (voir CLAUDE.md,
`python -m tera.render.app` -- pas `python3`).
"""
from __future__ import annotations
import random
import sys

import pygame

from ..worldgen import (
    generate_dungeon, generate_alfol_base, generate_meduz,
    CHEST_STAIRS_UP, CHEST_STAIRS_DOWN, CHEST_ALTAR, CHEST_BUREAU_MEDIAS, CHEST_MEDITER, CHEST_INVOQUER,
)
from ..amarande import (
    generate_mountains, generate_towns, generate_secret_words, direction_hint,
    AMARANDE_PYRAMIDE, AMARANDE_ALFOL,
)
from ..models import SaveGame
from ..game import (
    DUNGEON_FLOORS, AMARANDE_CITE_ENTRANCE, ALFOL_BASE_ENTRANCE,
    handle_amarande_command, handle_alfol_surface_command, handle_shop_command,
    amarande_here_lines, dungeon_here_lines,
    resolve_chest, resolve_altar, resolve_meduz_invoquer, resolve_bureau_medias, run_combat,
    try_move_dungeon, try_stairs_up, try_stairs_down, try_exit_dungeon, try_mediter,
)
from .topdown import draw_topdown
from .corridor import compute_planes, draw_corridor, turn_left, turn_right, draw_chest_marker, draw_monster_encounter
from .scene import draw_amarande_scene, draw_alfol_surface_scene, draw_shop_scene
from .textinput import pygame_ask
from .chargen import run_chargen

WINDOW_SIZE = (640, 480)
VIEW_AREA = pygame.Rect(0, 0, 640, 380)      # la "scene" -- topdown/couloir/carte
LOG_AREA = pygame.Rect(0, 380, 640, 100)     # journal + ligne de commande, TOUJOURS dans la fenetre
LOG_LINES = 4
LINE_HEIGHT = 18

OVERWORLD_KEYS = {
    pygame.K_UP: "n", pygame.K_w: "n",
    pygame.K_DOWN: "s", pygame.K_s: "s",
    pygame.K_RIGHT: "e", pygame.K_d: "e",
    pygame.K_LEFT: "o", pygame.K_a: "o",
}
TOPDOWN_KEYS = {
    pygame.K_UP: "N", pygame.K_w: "N",
    pygame.K_DOWN: "S", pygame.K_s: "S",
    pygame.K_RIGHT: "E", pygame.K_d: "E",
    pygame.K_LEFT: "O", pygame.K_a: "O",
}
CORRIDOR_FORWARD = (pygame.K_UP, pygame.K_w)
CORRIDOR_BACKWARD = (pygame.K_DOWN, pygame.K_s)
CORRIDOR_TURN_RIGHT = (pygame.K_RIGHT, pygame.K_d)
CORRIDOR_TURN_LEFT = (pygame.K_LEFT, pygame.K_a)
OPPOSITE = {"N": "S", "S": "N", "E": "O", "O": "E"}


class GameView:
    def __init__(
        self, save: SaveGame, party: list, rng: random.Random,
        screen: pygame.Surface | None = None, font: pygame.font.Font | None = None,
    ) -> None:
        self.save = save
        self.party = party
        self.rng = rng
        self.facing = "S"       # vue couloir des donjons uniquement
        # "corridor" (scene, comme l'original) par defaut -- "topdown" est
        # une carte de secours ajoutee pour le remake, pas dans l'original
        # (voir CLAUDE.md/notes moteur : le jeu se joue "image par image",
        # jamais en vue du dessus abstraite).
        self.dungeon_view = "corridor"
        self.log_lines: list[str] = []
        # References ecran/police pour dessiner le sprite du monstre au
        # tout debut d'un combat (voir `_on_monster`) -- le reste de la
        # boucle de combat est purement textuel (ask/log), donc sans ces
        # references il n'y a aucun autre moment ou dessiner le monstre.
        self.screen = screen
        self.font = font

    def on_monster(self, monster) -> None:
        if self.screen is None or self.font is None:
            return
        draw_monster_encounter(self.screen, VIEW_AREA, self.font, monster)
        pygame.display.flip()

    @property
    def world(self):
        return self.save.worlds[self.save.world_id]

    def log(self, text: str) -> None:
        for line in str(text).split("\n"):
            self.log_lines.append(line)
        self.log_lines = self.log_lines[-LOG_LINES:]


def build_game(screen: pygame.Surface, font: pygame.font.Font, clock: pygame.time.Clock, rng: random.Random) -> GameView:
    """Ecran de creation de personnage (mode texte fidele, `chargen.py`)
    puis generation du monde -- exactement l'ordre de l'original."""
    party = run_chargen(screen, font, clock, rng)
    save = SaveGame(characters=party, difficulty=5)
    save.pos_h_mountains, save.pos_v_mountains = generate_mountains(rng)
    save.loc_towns = generate_towns(rng)
    save.secret_words = generate_secret_words(rng)
    save.worlds = generate_dungeon(num_floors=DUNGEON_FLOORS, difficulty=save.difficulty, rng=rng)
    save.worlds[9] = generate_alfol_base(save.difficulty, rng)
    save.worlds[10] = generate_meduz(rng)
    save.location = "amarande"
    save.amarande_position = AMARANDE_CITE_ENTRANCE
    return GameView(save, party, rng, screen=screen, font=font)


def handle_amarande_key(view: GameView, key: int, ask) -> None:
    save = view.save
    prev_pos = save.amarande_position
    if key in OVERWORLD_KEYS:
        msg = handle_amarande_command(save, OVERWORLD_KEYS[key], ask=ask, log=view.log)
    elif key == pygame.K_e:
        msg = handle_amarande_command(save, "entrer", ask=ask, log=view.log)
    elif key == pygame.K_m:
        msg = handle_amarande_command(save, "monter", ask=ask, log=view.log)
    else:
        return
    if msg:
        view.log(msg)
    elif save.amarande_position != prev_pos:
        # Indications sur ce qu'il y a a voir/faire ici -- l'original
        # comme le CLI decrivent toujours la case actuelle, contrairement
        # a la scene seule qui n'affiche qu'un decor sans texte.
        for line in amarande_here_lines(save):
            view.log(line)


def handle_alfol_surface_key(view: GameView, key: int) -> None:
    save = view.save
    if key in OVERWORLD_KEYS:
        msg = handle_alfol_surface_command(save, OVERWORLD_KEYS[key])
    elif key == pygame.K_e:
        msg = handle_alfol_surface_command(save, "entrer")
    elif key == pygame.K_u:
        msg = handle_alfol_surface_command(save, "sortir")
    else:
        return
    if msg:
        view.log(msg)


def handle_shop_key(view: GameView, key: int, ask) -> None:
    if key == pygame.K_u:
        msg = handle_shop_command(view.save, view.party, "sortir", view.rng, ask=ask)
        if msg:
            view.log(msg)
        return
    if key != pygame.K_RETURN:
        return
    cmd = ask("Boutique (acheter <lettre>/vendre <nom>/hopital/sortir) : ").strip().lower()
    if not cmd:
        return
    msg = handle_shop_command(view.save, view.party, cmd, view.rng, ask=ask)
    if msg:
        view.log(msg)


def _log_dungeon_here(view: GameView, world) -> None:
    """Indications sur ce qu'il y a a voir/faire sur la case actuelle --
    sans repeter les passages/murs, deja visibles dans la vue couloir
    (contrairement au CLI, purement textuel)."""
    for line in dungeon_here_lines(world, view.save.position, view.save.world_id):
        view.log(line)


def handle_dungeon_key(view: GameView, key: int, ask) -> None:
    save = view.save
    world = view.world
    cell = world.cell(save.position)

    if key == pygame.K_t:
        view.dungeon_view = "corridor" if view.dungeon_view == "topdown" else "topdown"
        return

    if view.dungeon_view == "topdown" and key in TOPDOWN_KEYS:
        prev_pos = save.position
        msg = try_move_dungeon(save, view.party, world, TOPDOWN_KEYS[key], view.rng, ask=ask, log=view.log, on_monster=view.on_monster)
        if msg:
            view.log(msg)
        elif save.position != prev_pos:
            _log_dungeon_here(view, world)
        return
    if view.dungeon_view == "corridor":
        if key in CORRIDOR_FORWARD:
            prev_pos = save.position
            msg = try_move_dungeon(save, view.party, world, view.facing, view.rng, ask=ask, log=view.log, on_monster=view.on_monster)
            if msg:
                view.log(msg)
            elif save.position != prev_pos:
                _log_dungeon_here(view, world)
            return
        if key in CORRIDOR_BACKWARD:
            back = OPPOSITE[view.facing]
            prev_pos = save.position
            msg = try_move_dungeon(save, view.party, world, back, view.rng, ask=ask, log=view.log, on_monster=view.on_monster)
            if msg:
                view.log(msg)
            else:
                view.facing = back
                if save.position != prev_pos:
                    _log_dungeon_here(view, world)
            return
        if key in CORRIDOR_TURN_RIGHT:
            view.facing = turn_right(view.facing)
            return
        if key in CORRIDOR_TURN_LEFT:
            view.facing = turn_left(view.facing)
            return

    if key == pygame.K_o:
        view.log(resolve_chest(save, view.party, world, save.position, view.rng))
    elif key == pygame.K_c:
        if cell.encounter is not None:
            view.log(run_combat(save, view.party, world, save.position, view.rng, ask=ask, log=view.log, on_monster=view.on_monster))
        else:
            view.log("Rien a combattre ici.")
    elif key == pygame.K_i:
        if save.world_id == 10 and cell.chest == CHEST_INVOQUER:
            view.log(resolve_meduz_invoquer(save, ask=ask))
        elif cell.chest == CHEST_ALTAR:
            view.log(resolve_altar(view.party, view.rng))
        else:
            view.log("Il n'y a rien a invoquer ici.")
    elif key == pygame.K_m:
        if cell.chest == CHEST_STAIRS_UP:
            view.log(try_stairs_up(save, cell))
        else:
            view.log(try_mediter(save, view.party, cell))
    elif key == pygame.K_j:
        view.log(try_stairs_down(save, cell))
    elif key == pygame.K_p:
        if save.world_id == 9 and cell.chest == CHEST_BUREAU_MEDIAS:
            view.log(resolve_bureau_medias(save, ask=ask))
        else:
            view.log("Rien a payer ici.")
    elif key == pygame.K_u:
        view.log(try_exit_dungeon(save))


def draw_scene(screen: pygame.Surface, font: pygame.font.Font, view: GameView) -> str:
    """Dessine la scene (vue du dessus/couloir) et renvoie le texte
    d'aide contextuel pour la ligne de commandes."""
    save = view.save
    if save.location == "amarande":
        draw_amarande_scene(screen, save, VIEW_AREA, font)
        pos = save.amarande_position
        hint = (
            f"Cite {direction_hint(pos, AMARANDE_CITE_ENTRANCE)} / "
            f"Pyramide {direction_hint(pos, AMARANDE_PYRAMIDE)} / "
            f"Alfol {direction_hint(pos, AMARANDE_ALFOL)}"
        )
        return f"Amarande {pos} -- {hint}  [E]ntrer [M]onter"
    if save.location == "alfol_surface":
        draw_alfol_surface_scene(screen, save, VIEW_AREA, font)
        return f"Surface d'Alfol {save.alfol_surface_position}  [fleches] deplacer  [E] entrer  [U] sortir"
    if save.location == "shop":
        draw_shop_scene(screen, VIEW_AREA, font, save.money)
        return "Boutique  [Entree] taper une commande  [U] sortir"

    if view.dungeon_view == "corridor":
        planes = compute_planes(view.world, save.position, view.facing)
        draw_corridor(screen, planes, VIEW_AREA)
        draw_chest_marker(screen, VIEW_AREA, view.world.cell(save.position), view.world.cell(save.position).encounter)
    else:
        draw_topdown(screen, view.world, save.position, view.facing, VIEW_AREA)
    return (
        f"Monde {save.world_id} case {save.position}  [T] carte  [O]uvrir [C]ombattre "
        "[I]nvoquer [M]onter/mediter [J]descendre [P]ayer [U]sortir"
    )


def draw_log(screen: pygame.Surface, font: pygame.font.Font, view: GameView, controls: str) -> None:
    pygame.draw.rect(screen, (8, 8, 14), LOG_AREA)
    pygame.draw.line(screen, (70, 70, 85), LOG_AREA.topleft, LOG_AREA.topright, 1)
    for i, line in enumerate(view.log_lines):
        screen.blit(font.render(line[:100], True, (210, 210, 220)), (8, LOG_AREA.y + 6 + i * LINE_HEIGHT))
    controls_y = LOG_AREA.y + 6 + LOG_LINES * LINE_HEIGHT
    screen.blit(font.render(controls, True, (140, 140, 160)), (8, controls_y))


def main() -> None:
    pygame.init()
    pygame.display.set_caption("TERA remake -- prototype de rendu")
    screen = pygame.display.set_mode(WINDOW_SIZE)
    clock = pygame.time.Clock()
    font = pygame.font.SysFont(None, 18)
    input_bar = pygame.Rect(LOG_AREA.x, LOG_AREA.bottom - LINE_HEIGHT - 6, LOG_AREA.width, LINE_HEIGHT + 6)

    ask = lambda prompt: pygame_ask(screen, font, input_bar, clock, prompt)  # noqa: E731
    view = build_game(screen, font, clock, random.Random())

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key in (pygame.K_ESCAPE, pygame.K_q):
                    running = False
                elif view.save.location == "amarande":
                    handle_amarande_key(view, event.key, ask)
                elif view.save.location == "alfol_surface":
                    handle_alfol_surface_key(view, event.key)
                elif view.save.location == "shop":
                    handle_shop_key(view, event.key, ask)
                else:
                    handle_dungeon_key(view, event.key, ask)

        if not view.save.party_alive:
            view.log_lines = ["Le groupe est mort. Fin de partie."]
        elif view.save.victory:
            view.log_lines = ["Toutes les taches sont accomplies. Victoire !"]

        screen.fill((0, 0, 0))
        controls = draw_scene(screen, font, view)
        draw_log(screen, font, view, controls)
        pygame.display.flip()
        clock.tick(30)

    pygame.quit()
    sys.exit(0)


if __name__ == "__main__":
    main()
