"""Rendu Pygame, decouple de la logique de jeu (blueprint section 5).

Le jeu se joue "image par image" (une scene par case, jamais une carte
vue du dessus, confirme docs/TERA_notes_moteur.md section 5.1/19).

Sous-modules :
    corridor.py  -- vue couloir 1ere personne des donjons (vue par defaut)
    scene.py     -- vue scene d'Amarande/surface d'Alfol (decor procedural)
    topdown.py   -- carte de secours en vue du dessus (donjons, touche T,
                    pas dans l'original -- ajout du remake)
    textinput.py -- ligne de commande texte dans la fenetre (pas de console)
    app.py       -- boucle Pygame qui relie tout au moteur de donnees (game.py)
"""
