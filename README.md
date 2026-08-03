# TERA Remake

Reconstruction moderne de **TERA** (LORICIELS, octobre 1986), un RPG
français inspiré de Fighting Fantasy/B-X, à partir de son code source
Pascal décompilé et d'un reverse engineering complet du binaire original
(`TERA.COM`). Voir `docs/TERA_notes_moteur.md` pour tout ce qui a été
confirmé sur le moteur d'origine, et `mapping_fonctions_pascal_python.md`
pour la correspondance fonction par fonction entre le Pascal original et
ce port Python.

## ⚠️ Ce dépôt ne contient PAS les fichiers du jeu original

TERA reste une œuvre sous droits d'auteur (LORICIELS). Ce dépôt contient
uniquement le code du remake et la documentation d'analyse — **aucun
asset original** (binaires du jeu, code Pascal décompilé, sprites
extraits, manuel scanné). Pour faire tourner le jeu, tu dois posséder ta
propre copie légale de TERA et placer les fichiers toi-même :

```
data/compiled program/   # fichiers originaux (VALMON, OBJETS, VERBES,
                          # NOMON, NOMPERS, MESSAGE*, CRAPIM.IMA, etc.)
data/source code/        # *.PAS/*.INC decompiles, si tu les as
manuel/Tera.pdf          # manuel scanne (optionnel)
```

Une fois ces fichiers en place, régénère les données extraites :

```bash
python tools/extract_sprites.py      # -> data/sprites_extracted/
python3 tools/extract_text_data.py   # -> data/game_data.json
```

## Structure

```
CLAUDE.md                    # guide methodologique complet (contexte, etat d'avancement, conventions)
docs/
  TERA_notes_moteur.md       # tout ce qu'on sait du moteur original (reference de verite)
  TERA_remake_blueprint.md   # architecture cible du remake
  TERA_guide_desassemblage.md
  guide_couloir_pseudo3D.md  # guide generique (independant de TERA) : technique de couloir pseudo-3D CGA
  guide_creer_rpg_donjon.md  # guide generique : creer un RPG donjon de zero
mapping_fonctions_pascal_python.md  # Pascal original <-> Python, fonction par fonction
src/tera/
  models.py, worldgen.py, combat.py, data_loader.py, save.py, amarande.py
  game.py                    # boucle de jeu textuelle (CLI)
  render/                    # rendu Pygame (scenes, couloirs, sprites, creation de personnage)
tools/
  extract_sprites.py         # extrait les sprites/fonds d'ecran des .IMA (Pillow requis)
  extract_text_data.py       # extrait le texte du jeu (verbes/objets/monstres/messages)
tests/test_smoke.py
```

## Démarrage rapide

**Sous Windows : double-clique sur `lancer_tera.bat`** — menu qui règle
`PYTHONPATH` et choisit le bon interpréteur automatiquement.

```bash
# Tests de fumee (aucune dependance externe)
python3 tests/test_smoke.py

# Boucle de jeu textuelle (CLI)
python3 -m tera.game

# Rendu Pygame (necessite pygame -- voir requirements.txt)
pip install -r requirements.txt
python -m tera.render.app
```

## Pour reprendre le travail avec Claude Code

Ouvre ce dossier dans Claude Code : `CLAUDE.md` est lu automatiquement
et contient tout le contexte (état d'avancement détaillé, conventions du
projet, comment relancer un désassemblage si besoin).
