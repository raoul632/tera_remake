"""Ligne de commande textuelle DANS la fenetre Pygame.

Confirme par le manuel officiel (docs/TERA_notes_moteur.md section 18) :
l'original n'a jamais de console separee, tout se passe sur un seul
ecran graphique avec une seule ligne de commande en bas ("aucun texte
n'est affiche en permanence, a l'exception de la ligne inferieure de
commande"). Ce module reproduit ce principe : `pygame_ask` capture du
texte au clavier par-dessus l'ecran de jeu deja dessine, sans jamais
ouvrir de fenetre/console a part.
"""
from __future__ import annotations
import sys

import pygame

INPUT_BG = (15, 15, 25)
INPUT_BORDER = (90, 90, 110)
INPUT_TEXT = (255, 230, 160)


def pygame_ask(
    screen: pygame.Surface, font: pygame.font.Font, bar: pygame.Rect,
    clock: pygame.time.Clock, prompt: str,
) -> str:
    """Boucle bloquante : capture le texte tape jusqu'a Entree (Echap
    annule, renvoie une chaine vide). Ne redessine que la barre `bar` --
    le reste de l'ecran, deja dessine par l'appelant avant cet appel,
    reste visible derriere, exactement comme l'original."""
    text = ""
    while True:
        # pygame.event.get() draine toute la file d'un coup -- si on
        # rend la main des la premiere touche Entree/Echap trouvee, les
        # evenements suivants du meme lot (deja retires de la file par
        # ce get()) seraient perdus. On les repousse en file avant de
        # sortir (voir le meme correctif dans chargen.py).
        events = pygame.event.get()
        for i, event in enumerate(events):
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit(0)
            elif event.type == pygame.KEYDOWN:
                if event.key in (pygame.K_RETURN, pygame.K_ESCAPE):
                    for leftover in events[i + 1:]:
                        pygame.event.post(leftover)
                    return text.strip() if event.key == pygame.K_RETURN else ""
                if event.key == pygame.K_BACKSPACE:
                    text = text[:-1]
                elif event.unicode and event.unicode.isprintable():
                    text += event.unicode

        pygame.draw.rect(screen, INPUT_BG, bar)
        pygame.draw.rect(screen, INPUT_BORDER, bar, 1)
        screen.blit(font.render(f"{prompt}{text}_", True, INPUT_TEXT), (bar.x + 4, bar.y + 4))
        pygame.display.flip()
        clock.tick(30)
