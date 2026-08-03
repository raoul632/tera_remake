# Guide — créer un RPG donjon-crawler procédural (façon Wizardry/Bard's Tale)

Guide générique, indépendant du projet TERA — synthèse de l'architecture
qu'on a mise en place ici, pour la réutiliser dans un autre projet. Voir
`docs/guide_couloir_pseudo3D.md` pour le rendu en particulier (ce guide-ci
couvre tout le reste : données, génération, combat, navigation).

## 1. Vue d'ensemble : séparer données, génération, combat et rendu

Le piège classique (et celui de beaucoup de jeux DOS des années 80,
TERA inclus) est de mélanger la logique de jeu avec le rendu à
l'intérieur des mêmes fonctions. Ça marche, mais c'est difficile à
tester et impossible à faire évoluer indépendamment. Structure
recommandée, en 4 couches qui ne se connaissent pas entre elles au-delà
de ce qui est strictement nécessaire :

```
modeles de donnees  (dataclasses/structs : Character, MapCell, World, Monster, SaveGame)
        |
generation procedurale  (construit des World a partir d'un seed/rng)
        |
resolution de combat / logique de jeu  (pure : (etat, action) -> nouvel etat)
        |
rendu  (lit l'etat, ne le modifie jamais directement)
```

Bénéfice concret vérifié sur ce projet : on a pu écrire toute la logique
(génération, combat, navigation) et la valider avec une boucle de jeu en
ligne de commande **avant** d'écrire une seule ligne de rendu graphique
— zéro dépendance à Pygame (ou autre) dans les 3 premières couches.

## 2. Le modèle de données central

Quatre structures suffisent pour la plupart des RPG donjon-crawler :

```python
@dataclass
class Character:
    name: str
    stats: dict[str, int]       # ex: force, intelligence, agilite...
    skills: dict[str, int]      # maitrise par stat, 0 = non formee
    hp: int
    max_hp: int
    equipment: list[str]
    state: str                  # "OK" / "HS" (hors combat) / autre statut
    in_party: bool

@dataclass
class MapCell:
    walls: dict[str, bool]      # {"N": True, "S": False, "E": True, "O": False}
    chest: int | None           # code de contenu (voir section 4)
    encounter: int | None       # id de monstre present, ou None

@dataclass
class World:                    # un etage/niveau
    id: int
    cells: list[MapCell]        # grille NxN, indexee ligne-major ou colonne-major (choisis-en un et garde-le partout)

@dataclass
class SaveGame:
    characters: list[Character]
    worlds: dict[int, World]
    position: int
    world_id: int
    money: int
    tasks_done: list[bool]      # objectifs de scenario
```

**Astuce qui a bien marché ici** : indexer `Character.skills` par le
**même nom que la stat associée** (`skills["force"]`, pas
`skills["lutter"]`) plutôt que par verbe/action. Une compétence est "la
maîtrise de telle statistique en combat", pas "la maîtrise de tel
bouton" — ça évite d'avoir deux systèmes de nommage à synchroniser, et
ça se généralise mieux si tu ajoutes des actions plus tard.

## 3. Génération procédurale de donjon : la marche aléatoire auto-évitante

La technique la plus simple qui donne un résultat correct (donjon
connexe, pas de zones isolées) : partir d'une case, faire N pas
aléatoires en évitant de revenir immédiatement en arrière, et "creuser"
(marquer comme praticable) chaque arête traversée.

```python
def dig_maze(width: int, height: int, steps: int, rng: random.Random) -> list[MapCell]:
    cells = [MapCell(walls={"N": False, "S": False, "E": False, "O": False}) for _ in range(width * height)]
    pos = 0
    last_dir = None

    for _ in range(steps):
        while True:
            direction = rng.choice(["N", "S", "E", "O"])
            if direction == last_dir:
                continue  # evite de faire des allers-retours triviaux
            col, row = pos % width, pos // width
            if direction == "E" and col == width - 1: continue
            if direction == "O" and col == 0: continue
            if direction == "N" and row == 0: continue
            if direction == "S" and row == height - 1: continue
            break
        last_dir = {"N": "S", "S": "N", "E": "O", "O": "E"}[direction]  # interdit le demi-tour au prochain pas

        delta = {"N": -width, "S": width, "E": 1, "O": -1}[direction]
        opposite = {"N": "S", "S": "N", "E": "O", "O": "E"}[direction]
        cells[pos].walls[direction] = True
        pos += delta
        cells[pos].walls[opposite] = True

    return cells
```

**Point important, source de bug si oublié** : une case "par défaut"
(tous les murs à `False`/fermés) ne veut PAS dire "case ouverte de tous
les côtés" — ça veut dire "case jamais visitée par la marche, donc hors
du donjon". Les murs qu'on POSE pendant le creusement sont les couloirs
praticables, pas des obstacles. Une fois qu'on a compris ça,
l'algorithme de recherche de cases atteignables est trivial (parcours
en largeur/profondeur classique sur le graphe des arêtes ouvertes).

## 4. Marqueurs uniques et validation de génération

Une fois le labyrinthe creusé, place les éléments uniques (clé,
escalier, boss, sortie...) sur des cases **atteignables** (calculées
via un parcours de graphe depuis l'entrée), pas des cases aléatoires
tout court — sinon tu risques de placer la clé dans une poche isolée.

```python
def find_reachable(cells: list[MapCell], width: int, start: int) -> set[int]:
    seen, stack = {start}, [start]
    while stack:
        pos = stack.pop()
        col, row = pos % width, pos // width
        cell = cells[pos]
        neighbours = []
        if cell.walls["E"] and col < width - 1: neighbours.append(pos + 1)
        if cell.walls["O"] and col > 0: neighbours.append(pos - 1)
        if cell.walls["N"] and row > 0: neighbours.append(pos - width)
        if cell.walls["S"] and row < width - 1: neighbours.append(pos + width)
        for n in neighbours:
            if n not in seen:
                seen.add(n)
                stack.append(n)
    return seen
```

Et surtout : **valide la génération avant de l'accepter**, et
recommence sinon (avec le même générateur aléatoire, pas un nouveau
seed — plus simple à raisonner). C'est la technique qui garantit qu'une
partie générée est toujours finissable :

```python
def generate_valid_dungeon(width, height, steps, rng, max_attempts=500):
    for _ in range(max_attempts):
        cells = dig_maze(width, height, steps, rng)
        reachable = find_reachable(cells, width, start=0)
        if EXIT_POSITION in reachable:          # invariant a verifier
            place_markers(cells, reachable, rng)  # cle, boss, etc, sur des cases de `reachable`
            return cells
    raise RuntimeError("generation invalide apres trop de tentatives")
```

## 5. Le système de combat : stats, seuil, bonus d'équipement

Une formule qui marche bien et qui est facile à équilibrer :

```python
def resolve_action(char, stat_key, skill_key, rng) -> int:
    power = char.stats[stat_key] + skill_modifier(char, skill_key)
    roll = power * rng.randint(3, 17)          # facteur aleatoire large -> volatilite
    return roll - THRESHOLD_OFFSET if roll > SUCCESS_THRESHOLD else 0

def skill_modifier(char, skill_key) -> int:
    level = char.skills.get(skill_key, 0)
    return level if level > 0 else -2           # penalise l'absence de formation
```

**Leçon apprise sur ce projet** (une erreur qu'on a faite puis
corrigée) : ne mélange pas le bonus d'équipement DANS le calcul de
`power` avant le jet aléatoire — sépare-le, applique-le APRES, et fais
des choix DIFFÉRENTS selon l'action :

```python
def resolve_with_equipment(char, party, stat_key, skill_key, required_item, style, rng):
    base = resolve_action(char, stat_key, skill_key, rng)
    equip = equipment_bonus(char, party, required_item)  # 0 si personne n'a l'objet
    if style == "soft":
        return base * (1 + equip)     # l'objet ameliore, sans etre indispensable
    if style == "hard":
        return base * equip           # SANS objet, le resultat est TOUJOURS 0
    return base                       # certaines actions n'utilisent pas d'objet du tout
```

Avoir ces 3 styles (souple / dur / absent) selon l'action donne
beaucoup plus de profondeur tactique qu'une seule formule uniforme —
c'est ce qui rend certaines actions "risquées sans le bon objet" et
d'autres "toujours utilisables en secours".

## 6. Navigation multi-échelle : hub + sous-lieux

Un RPG de ce genre a presque toujours 2-3 échelles de carte différentes
qui ne partagent pas le même modèle de déplacement :

- **Carte du monde** (hub) : souvent sans mur par case, juste des
  obstacles linéaires (montagnes, rivières) et des points d'intérêt
  fixes ou semi-aléatoires.
- **Donjons** : grille avec mur par case (section 3), déplacement
  cardinal strict.
- **Éventuellement une ville/zone intermédiaire** sans mur du tout,
  juste un bouclage modulo simple.

Le point de design le plus important : **utilise une machine à états
explicite** (`location: Literal["hub", "town", "dungeon"]`) plutôt que
d'essayer de faire rentrer les 3 dans le même système de coordonnées.
Chaque mode a sa propre logique de déplacement/collision/rendu ; seule
la transition entre modes (rentrer dans un donjon, en ressortir) a
besoin d'un point de contact.

```python
def game_loop(save):
    while True:
        if save.location == "hub":
            handle_hub_turn(save)
        elif save.location == "town":
            handle_town_turn(save)
        elif save.location == "dungeon":
            handle_dungeon_turn(save)
```

## 7. Boucle de jeu textuelle avant le rendu

Écris et fais tourner tout ce qui précède **en ligne de commande**,
sans aucun rendu graphique, avant d'investir dans Pygame/autre. Ça
permet de :

- Valider que la génération produit toujours des donjons finissables
  (en générant des centaines de parties en boucle dans un test).
- Jouer/déboguer le combat sans avoir à interagir avec une fenêtre.
- Découpler complètement les bugs de logique des bugs de rendu.

Une boucle minimale : afficher l'état courant, lire une commande texte,
la dispatcher vers un handler par mode (section 6), recommencer.

## 8. Rendu

Une fois la logique validée : le rendu du donjon en vue 1ère personne
est couvert en détail dans `docs/guide_couloir_pseudo3D.md` (technique
historique à coordonnées figées vs technique calculée). La vue du
dessus (hub/ville) est plus simple : une grille de rectangles, un mur
dessiné là où le déplacement est bloqué, un marqueur par point
d'intérêt.

## 9. Sauvegarde

Sérialise tout `SaveGame` en JSON (`dataclasses.asdict` + un
reconstructeur symétrique) plutôt que d'inventer un format binaire —
largement suffisant pour ce genre de jeu, lisible pour déboguer, et
trivial à faire évoluer (ajouter un champ = ajouter une clé optionnelle
avec valeur par défaut au chargement).

**Piège rencontré ici** : si `SaveGame` grossit au fil du
développement (nouveaux champs ajoutés), pense à mettre à jour
la fonction de (dé)sérialisation en même temps — sinon les nouveaux
champs sont silencieusement perdus au chargement, sans erreur visible.
Un test de fumée qui fait un aller-retour save→load et compare l'objet
complet attrape ça immédiatement.

## 10. Résumé des pièges à éviter

- Mur "par défaut" != case ouverte — vérifie toujours l'atteignabilité
  avant de placer un élément unique.
- Ne mélange pas le bonus d'objet dans le jet aléatoire — sépare-le et
  varie son application par action.
- Une seule échelle de coordonnées pour 3 modes de carte différents =
  complexité inutile ; une machine à états explicite est plus simple.
- Génère et valide en boucle plutôt que d'espérer qu'un seul essai
  aléatoire soit toujours jouable.
- Teste la logique en ligne de commande avant d'écrire le rendu.
- Garde `save.py` synchronisé avec `SaveGame` à chaque champ ajouté.
