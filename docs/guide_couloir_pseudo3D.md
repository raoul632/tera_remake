# Guide — rendu de couloir pseudo-3D façon Wizardry/Bard's Tale

Guide générique, indépendant du projet TERA (voir `docs/TERA_notes_moteur.md`
section 5.2 pour le contexte d'origine — cette technique a été identifiée
en lisant le code source d'un jeu DOS de 1986). Pense-bête si tu veux
réutiliser l'idée dans un autre projet, jeu de labyrinthe en vue
1ère personne, style *donjon crawler* rétro.

## 1. Le principe

Donner une impression de profondeur dans un couloir de labyrinthe **sans**
faire de vrai calcul 3D (pas de raycasting, pas de matrice de projection).
Deux façons de le faire, des moins chère à la plus chère en développement :

1. **Technique historique (coordonnées figées)** — celle qu'utilisait le
   jeu d'origine. On dessine à la main 2-3 "vues" de mur latéral (une par
   profondeur : proche/moyen/loin), chacune avec ses coordonnées pixel
   codées en dur, et on choisit la bonne vue selon combien de cases de
   couloir ouvert il y a devant le joueur. Zéro calcul géométrique, juste
   une table de formes pré-dessinées. Idéal si tu veux le style rétro
   exact et que tu es prêt à dessiner/ajuster les coordonnées à la main.
2. **Technique calculée (mise à l'échelle géométrique)** — celle utilisée
   dans le remake. On calcule un rectangle qui rétrécit à chaque
   profondeur avec un facteur d'échelle fixe (ex. ×0.62 par case), centré
   sur le point de fuite. Aucune coordonnée à dessiner à la main, marche
   pour n'importe quelle résolution d'écran, mais rendu un peu plus
   "générique"/mécanique visuellement. Recommandé si tu veux un résultat
   correct rapidement.

Ce guide détaille la seconde (calculée), avec le principe de la première
expliqué pour référence.

## 2. Modèle de données minimal

Un labyrinthe = une grille de cases, chaque case sait quels côtés sont
ouverts (praticables) :

```python
@dataclass
class Cell:
    walls: dict[str, bool]  # {"N": True, "S": False, "E": True, "O": False}
                              # True = passage ouvert, False = mur bloquant
```

Le joueur a une **position** (index de case) et une **direction du
regard** (`facing`, un des 4 points cardinaux). C'est tout ce qu'il faut
— pas besoin d'angle libre, de coordonnées flottantes ni de moteur
physique : le déplacement reste discret, case par case, cardinal par
cardinal, comme dans un donjon crawler classique.

## 3. Contrôles : avancer/reculer/pivoter, pas de déplacement libre

Point confirmé par le manuel officiel du jeu d'origine (utile de le
reprendre tel quel, ça évite un piège classique) : dans une vue couloir,
les contrôles doivent être **relatifs à la direction du regard**, pas
absolus :

- **Avancer** : essaie de se déplacer dans la direction `facing` actuelle.
- **Reculer** : essaie de se déplacer dans la direction opposée à `facing`
  (et `facing` bascule à 180° si le mouvement réussit).
- **Pivoter à gauche/droite** : change `facing` de 90°, **sans bouger**.

```python
CLOCKWISE = ["N", "E", "S", "O"]

def turn_right(facing: str) -> str:
    return CLOCKWISE[(CLOCKWISE.index(facing) + 1) % 4]

def turn_left(facing: str) -> str:
    return CLOCKWISE[(CLOCKWISE.index(facing) - 1) % 4]
```

(Une carte "vue du dessus", si tu en as une, utilise elle des touches
absolues N/S/E/O directement — les deux schémas coexistent souvent dans
le même jeu selon l'écran affiché, ce n'est pas incohérent.)

## 4. L'algorithme de "regard" : combien de cases voit-on devant soi ?

À chaque case en avançant dans la direction du regard, on note si le mur
gauche/droit/face est ouvert ou bloqué, jusqu'à un mur de face (fin de
couloir) ou une distance de rendu maximale :

```python
MAX_DEPTH = 4

def turn_right(facing): ...  # voir section 3
def turn_left(facing): ...

DELTA = {"N": -width, "S": width, "E": 1, "O": -1}  # `width` = largeur de la grille

def compute_planes(grid: list[Cell], pos: int, facing: str, width: int):
    left_dir, right_dir = turn_left(facing), turn_right(facing)
    planes = []
    current = pos
    for _ in range(MAX_DEPTH):
        cell = grid[current]
        face_blocked = not cell.walls[facing]
        planes.append({
            "left_blocked": not cell.walls[left_dir],
            "right_blocked": not cell.walls[right_dir],
            "face_blocked": face_blocked,
        })
        if face_blocked:
            break
        current += DELTA[facing]  # + verifier les bords de la grille avant d'avancer
    return planes
```

## 5. Dessiner : la technique calculée

Un rectangle par profondeur, qui rétrécit vers le centre de l'écran ; les
murs latéraux sont des trapèzes reliant le rectangle "proche" au
rectangle "loin" ; le mur de face (s'il y en a un) est le rectangle final
rempli.

```python
DEPTH_SHRINK = 0.62  # a ajuster au gout -- plus petit = couloir qui "rentre" plus vite

def plane_rect(screen_rect, depth: int):
    scale = DEPTH_SHRINK ** depth
    w, h = screen_rect.width * scale, screen_rect.height * scale
    cx, cy = screen_rect.centerx, screen_rect.centery
    return pygame.Rect(cx - w / 2, cy - h / 2, w, h)

def draw_corridor(surface, planes, screen_rect):
    near = screen_rect
    for depth, plane in enumerate(planes):
        far = plane_rect(screen_rect, depth + 1)

        if plane["left_blocked"]:
            pygame.draw.polygon(surface, WALL_COLOR, [
                (near.left, near.top), (near.left, near.bottom),
                (far.left, far.bottom), (far.left, far.top),
            ])
        if plane["right_blocked"]:
            pygame.draw.polygon(surface, WALL_COLOR, [
                (near.right, near.top), (near.right, near.bottom),
                (far.right, far.bottom), (far.right, far.top),
            ])
        if plane["face_blocked"]:
            pygame.draw.rect(surface, WALL_COLOR, far)
            break

        near = far
```

C'est tout le moteur. Pas de sprites nécessaires pour un premier jet —
juste des rectangles/trapèzes pleins suffisent à donner l'effet de
profondeur. Ajouter des textures/sprites sur les murs vient après, une
fois la géométrie de base validée.

## 6. Dessiner : la technique historique (pour référence)

Si tu veux le style "vues pré-dessinées à la main" plutôt que du
calculé : au lieu de `plane_rect()`, tu définis à la main 2-3 jeux de
coordonnées fixes (un par profondeur), typiquement sous forme de petites
fonctions ou d'une table :

```python
WALL_VIEWS = {
    ("left", 1): [(25, 136), (35, 98), ...],   # polygone fait main, profondeur 1
    ("left", 2): [(25, 46), (64, 56), ...],    # profondeur 2, plus petit/central
    ("left", 3): [(25, 161), (80, -1), ...],   # profondeur 3
    # ... idem pour "right"
}

def draw_wall_view(surface, side: str, depth: int, color):
    pygame.draw.polygon(surface, color, WALL_VIEWS[(side, depth)])
```

Avantage : contrôle artistique total, look rétro précis. Inconvénient :
il faut redessiner les coordonnées à la main pour chaque profondeur/côté,
et ça ne s'adapte pas automatiquement si tu changes la résolution d'écran
— exactement le compromis qu'avait fait le jeu d'origine en 1986.

## 7. Points d'attention

- **Bords de grille** : vérifie toujours qu'avancer d'une case ne sort
  pas de la grille avant de le faire (sinon `DELTA` peut faire pointer
  `current` n'importe où).
- **`facing` ne change qu'en cas de déplacement réel** — si un mur
  bloque, ne mets pas à jour la direction (sinon le joueur peut "regarder"
  à travers un mur en avançant dans le vide).
- **`MAX_DEPTH`** : au-delà d'une certaine profondeur (3-5 cases), le
  rectangle devient trop petit pour être visible utilement — inutile de
  calculer plus loin.
- Cette technique suppose un labyrinthe en **grille cardinale stricte**
  (4 directions, pas d'angles libres). Si ton jeu a besoin d'angles
  libres ou de couloirs non alignés sur une grille, il te faut un vrai
  moteur de projection (raycasting ou 3D), pas cette astuce.
