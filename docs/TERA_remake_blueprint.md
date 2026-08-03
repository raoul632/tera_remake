# TERA — Blueprint de remake

Ce document propose une architecture pour un remake moderne, basée sur la
compréhension complète du moteur original (voir `TERA_notes_moteur.md`).
L'objectif : garder l'esprit et les mécaniques du jeu de 1986, en
modernisant ce qui n'a de sens que pour des contraintes matérielles
disparues depuis longtemps (overlays, CGA, disquettes).

## 1. Choix technique

**Recommandation : Python.** C'est cohérent avec Oracle Toolbox/Herobook,
et largement suffisant pour ce type de jeu (pas de temps réel exigeant).

- **Pygame** si tu veux une fenêtre autonome, un rendu pixel-art proche de
  l'original, et un contrôle total sur le rendu vectoriel façon `DRAW`.
- **Une appli web (Flask + Canvas/SVG, dans l'esprit d'Herobook)** si tu
  préfères un format navigateur, plus facile à partager.

Les deux approches partagent exactement le même cœur logique (sections
2-4 ci-dessous) ; seule la couche de rendu change. Je recommande de
**découpler complètement logique et rendu dès le départ** (contrairement
à l'original, où `PutSprite`/`DRAW` sont mêlés à la logique de jeu dans
chaque overlay) — ça permettra de tester le moteur sans même avoir de
fenêtre graphique.

## 2. Modèle de données (remplace `tSavedata`)

```python
from dataclasses import dataclass, field

@dataclass
class Character:
    name: str
    gender: str
    stats: dict[str, int]      # force, intelligence, chance, endurance,
                                # volonte, vitalite, agilite, charme, habilete
    skills: dict[str, int]     # niveau de compétence par type, 0 = non formé
    hp: int
    mp: int
    equipment: list[str]       # jusqu'à 10 emplacements
    state: str                 # 'OK' / 'HS' / 'EM'
    in_party: bool

@dataclass
class MapCell:
    walls: dict[str, int]      # 'N','S','E','O' -> 0/1/2/3 (aucun/simple/porte/secret)
    chest: int | None          # None, ou code objet/or/clé/carte (voir table ci-dessous)
    encounter: int | None      # id de monstre présent
    extra: int                 # 7e attribut (encore incertain dans l'original)

@dataclass
class World:
    id: int
    cells: list[MapCell]       # 100 cellules, grille 10x10

@dataclass
class Monster:
    name: str
    hp_base: int
    resist_a: int
    resist_b: int
    weakness_item: str | None  # None si pas de faiblesse déclarée
    initiative: int
    hp_variance: int

@dataclass
class SaveGame:
    characters: list[Character]
    worlds: list[World]
    position: int
    world_id: int
    money: int
    difficulty: int
    days: int
    tasks_done: list[bool]      # 4 tâches
    secret_words: list[str]     # 4 mots secrets
    hint_strings: list[str]
    keys: list[int]
    has_map: list[bool]
```

Table des codes de coffre (`MapCell.chest`), fidèle à l'original :
- `0` = vide, `1-25` = objet (table `OBJETS`), `26-89` = or (`valeur-20`
  pièces), `90-98` = clé n°(valeur-90), `99` = carte, `100`/`101` =
  escalier montant/descendant, `103` = autel de soin, `None` = pas de coffre.

## 3. Génération procédurale (remplace `OVL_0000`)

Reprendre fidèlement l'algorithme validé :
1. **Creusement** : marche aléatoire auto-évitante de 225 pas par monde,
   posant des qualités de mur 1/2/3 à chaque intersection (probabilités
   9:1 / 4:1 / reste, comme dans `INNER_BB8C`).
2. **Contenu** : monstres/coffres pondérés par profondeur, une clé et un
   autel de soin garantis par étage, Arioch toujours en fin de
   progression, une case de sortie garantie.
3. **Auto-validation** : régénérer tant que les invariants structurels
   ne sont pas satisfaits (un escalier par étage, Arioch atteignable,
   une sortie existe) — **garder ce principe tel quel**, c'est une bonne
   pratique de génération procédurale qui garantit la jouabilité.

C'est la partie la plus directement portable telle quelle en Python —
pas besoin de la repenser, juste de la traduire.

## 4. Combat (remplace la formule `0x8315`/`0x842a`/`0x8391`)

```python
def resolve_attack(char: Character, action: str, target_item: str | None) -> int:
    stat_value = char.stats[ACTION_TO_STAT[action]]
    skill_bonus = char.skills.get(action, 0) or -2
    equipment_bonus = check_equipment_bonus(char, target_item)  # 3/2/1/0
    power = stat_value + skill_bonus + char.skills.get('bonus_fixe', 0) + equipment_bonus
    roll = power * (random.randint(0, 14) + 3)
    return max(0, roll - 35) if roll > 44 else 0

def monster_resistance(monster: Monster, item_used: str) -> int:
    if monster.weakness_item and item_used == monster.weakness_item:
        return monster.resist_a // 5   # diviseur favorable
    return monster.resist_b // 7       # diviseur normal
```

Recommandation : **garder les seuils numériques exacts** (44, 35, les
divisions par 5/6/7) pour préserver l'équilibrage original — ils ont
été testés et itérés par les auteurs en 1986, les changer à l'aveugle
casserait la difficulté calibrée.

## 5. Rendu (la vraie partie à moderniser)

L'original avait **trois moteurs de rendu séparés** par contrainte
technique (voir notes du moteur, section 5) — plus la découverte que
Cité des Crânes et Temple de Seth partagent déjà un module copié-collé.
Pour le remake, **un seul renderer de couloir paramétré** suffit pour
tous les donjons :

```python
def render_corridor(walls: dict[str, int], depth_scale: float, has_door_sprite: str | None):
    # walls: murs gauche/droite/face selon la direction de déplacement
    # depth_scale: remplace les 3 "gabarits de salle" figés de Seth/Alfol
    ...
```

Et un renderer de "vue du dessus" pour Amarande/Alfol (sprites de décor
sur seed procédural plutôt que le `modulo 23` d'origine — voir section 6).

## 6. Ce qu'on modernise vs ce qu'on garde à l'identique

| Système | Garder tel quel | Moderniser |
|---|---|---|
| Génération de donjon | Marche aléatoire + auto-validation | — |
| Formule de combat | Seuils numériques (44, 35, ÷5/6/7) | Présentation/UI seulement |
| Cartes du monde (Amarande) | — | Mur par case au lieu de "montagnes-lignes" |
| Décor procédural | — | Bruit/seed au lieu de `position MOD 23` |
| Rendu couloir | Principe du mur gauche/droite/face | Un seul module réutilisable partout |
| Sauvegarde | Structure de données | JSON/YAML au lieu de FCB DOS + disquette |
| Sprites | Style pixel-art assumé | Sprites modernes ou pixel-art redessiné |

## 7. Plan de développement suggéré

1. **Moteur de données pur** (pas de rendu) : `SaveGame`, génération de
   monde, résolution de combat — testable en ligne de commande.
2. **Boucle de jeu textuelle** (comme un MUD) pour valider toute la
   logique avant d'investir dans le rendu.
3. **Rendu 2D** (Pygame ou Canvas web) — vue du dessus d'abord (plus
   simple), puis le couloir en 1ère personne.
4. **Contenu** : les 4 tâches, les 22 monstres, les 25 objets, les 9
   PNJ recrutables (tout est déjà extrait dans `TERA_notes_moteur.md`,
   annexe).

Cette approche suit exactement la logique de séparation
logique/présentation qu'on retrouve dans Herobook — un terrain que tu
connais déjà bien.
