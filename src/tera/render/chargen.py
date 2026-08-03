"""Ecran de creation de personnage, mode texte fidele a OVL_0022.PAS.

Confirme par lecture du code source decompile (docs/TERA_notes_moteur.md
sections 19/20) : contrairement au reste du jeu (scenes/couloirs), cet
ecran est en **mode texte** -- une fenetre bordee de caracteres de
dessin de boite, fond bleu, texte jaune/blanc/rouge clair. Ce module
fournit les callbacks `ask`/`log` (voir le type `Ask`/`Log` dans
game.py) qui dessinent dans cette fenetre plutot que d'utiliser
`pygame_ask` generique de textinput.py, pour reproduire cet habillage
specifique -- toute la LOGIQUE de creation (formule des stats, reroll,
points bonus) reste entierement dans `game.py::create_party`, non
dupliquee ici.
"""
from __future__ import annotations
import sys
import textwrap

import pygame

from ..game import create_party
from ..models import Character

BOX_BG = (0, 0, 170)           # bleu CGA
BORDER_COLOR = (255, 255, 255)
TEXT_WHITE = (255, 255, 255)
TEXT_YELLOW = (255, 255, 85)
TEXT_LIGHTRED = (255, 85, 85)

TITLE = "CREATION DU PERSONNAGE"


def _wrap(text: str, width: int = 58) -> list[str]:
    return textwrap.wrap(text, width) or [""]


def _draw_box(
    screen: pygame.Surface, font: pygame.font.Font, box: pygame.Rect,
    lines: list[tuple[tuple[int, int, int], str]],
    current_prompt: str | None = None, current_input: str = "",
) -> None:
    screen.fill((0, 0, 0), box.inflate(20, 20))
    pygame.draw.rect(screen, BOX_BG, box)
    pygame.draw.rect(screen, BORDER_COLOR, box, 2)

    line_h = font.get_height() + 2
    title_surf = font.render(TITLE, True, TEXT_LIGHTRED)
    screen.blit(title_surf, (box.centerx - title_surf.get_width() // 2, box.y + 6))

    y = box.y + 6 + line_h + 6
    max_lines = max(1, (box.bottom - line_h - 12 - y) // line_h)
    for color, text in lines[-max_lines:]:
        screen.blit(font.render(text, True, color), (box.x + 12, y))
        y += line_h

    if current_prompt is not None:
        prompt_lines = _wrap(f"{current_prompt}{current_input}_")
        py = box.bottom - line_h * len(prompt_lines) - 8
        for pl in prompt_lines:
            screen.blit(font.render(pl, True, TEXT_YELLOW), (box.x + 12, py))
            py += line_h

    pygame.display.flip()


def make_callbacks(screen: pygame.Surface, font: pygame.font.Font, clock: pygame.time.Clock, box: pygame.Rect):
    """Construit une paire (ask, log) qui dessine dans `box`. Separe de
    `run_chargen` pour pouvoir etre teste independamment (SDL dummy)."""
    lines: list[tuple[tuple[int, int, int], str]] = []

    def log(msg: str) -> None:
        for wl in _wrap(msg):
            lines.append((TEXT_WHITE, wl))
        _draw_box(screen, font, box, lines)
        # Ne consomme que les QUIT -- toute autre touche deja en file
        # (ex. reponse tapee juste avant que ce log() ne s'affiche) doit
        # rester disponible pour le prochain ask().
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit(0)
            else:
                pygame.event.post(event)
        clock.tick(30)

    def ask(prompt: str) -> str:
        text = ""
        while True:
            # pygame.event.get() draine TOUTE la file d'un coup -- si
            # plusieurs touches (dont un Entree) arrivent dans la meme
            # frame (typage rapide, ou evenements pousses par un script
            # de test), il faut remettre en file les evenements APRES
            # celui qui declenche le retour, sinon ils sont perdus
            # silencieusement (jamais vus par le prochain ask()).
            events = pygame.event.get()
            for i, event in enumerate(events):
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit(0)
                elif event.type == pygame.KEYDOWN:
                    if event.key in (pygame.K_RETURN, pygame.K_ESCAPE):
                        for leftover in events[i + 1:]:
                            pygame.event.post(leftover)
                        lines.append((TEXT_YELLOW, f"{prompt}{text}"))
                        return text.strip() if event.key == pygame.K_RETURN else ""
                    if event.key == pygame.K_BACKSPACE:
                        text = text[:-1]
                    elif event.unicode and event.unicode.isprintable():
                        text += event.unicode
            _draw_box(screen, font, box, lines, current_prompt=prompt, current_input=text)
            clock.tick(30)

    return ask, log


def run_chargen(
    screen: pygame.Surface, font: pygame.font.Font, clock: pygame.time.Clock, rng,
) -> list[Character]:
    """Boucle bloquante : affiche l'ecran de creation mode texte et
    renvoie le groupe cree (delegue a `game.create_party`, y compris la
    boucle de reroll "Gardes-tu ce personnage ?")."""
    box = pygame.Rect(0, 0, min(560, screen.get_width() - 40), min(360, screen.get_height() - 40))
    box.center = screen.get_rect().center
    ask, log = make_callbacks(screen, font, clock, box)
    return create_party(rng, ask=ask, log=log)
