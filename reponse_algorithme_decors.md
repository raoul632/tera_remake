# Logique de l'algorithme de placement des décors (Amarande)

Source Pascal : `data/source code/OVL_00FB.PAS`, procédure `INNER_C377`.
Port Python : `src/tera/render/amarande_scenes.py` (table) +
`src/tera/render/scene.py` (interprète).

L'algorithme repose sur deux couches indépendantes qui se combinent à
chaque affichage de scène.

## 1. Quelle "scène" afficher — une simple table, pas de hasard

`Sav.f_Position MOD 23` choisit une parmi 23 recettes fixes. Une même
case donne toujours la même composition de base : ce n'est pas de la
génération procédurale, juste une table de correspondance à 23 entrées
écrite à la main.

**Pourquoi 23 précisément** (hypothèse personnelle, non confirmée par
le code source) : les déplacements sur Amarande se font par pas de ±1
(nord/sud) et ±30 (est/ouest). 23 est premier avec ces deux valeurs,
donc se déplacer en ligne droite dans n'importe quelle direction fait
défiler les 23 variantes avant toute répétition — un moyen économique
(23 arrangements à la main plutôt qu'un art unique par case) d'éviter
un motif de répétition trop évident en marchant.

## 2. Ce que chaque recette dessine réellement — une bibliothèque de "tampons" réutilisables

Plutôt que de placer chaque sprite à la main dans chacune des 23 scènes,
le développeur a écrit une dizaine de fonctions "amas" paramétrées (un
bouquet de 3 arbres, un bouquet de 5 arbres, une formation rocheuse, une
carcasse de créature, un semis de débris au sol, etc.), chacune prenant
un décalage X (ou X,Y). Chaque scène parmi les 23 n'est qu'une courte
recette qui appelle 3 à 6 de ces tampons à des décalages différents —
par exemple la scène 1 : "gros bouquet d'arbres ici + débris au sol +
formation rocheuse + un sprite + petit bouquet d'arbres là". C'est ce
qui explique la compacité de la table à 23 entrées : elle compose des
morceaux réutilisables, elle ne décrit pas des placements de sprites
bruts.

## 3. Deux couches déclenchées indépendamment, par-dessus

- **Arrière-plan** (`INNER_BD0C`) : dessiné presque à chaque affichage,
  quel que soit l'index de scène — 16 étoiles aléatoires + une des deux
  polylignes d'horizon possibles, tirée au hasard, avec un paramètre de
  hauteur/irrégularité lui aussi aléatoire. C'est pourquoi une même
  scène (même index) a un aspect légèrement différent à chaque visite :
  la composition du premier plan est fixe, mais étoiles/horizon sont
  retirés au hasard à chaque fois.
- **Montagne en surcouche** : une vérification *séparée* (est-on
  adjacent à une vraie chaîne `PosHMountains`/`PosVMountains` ?)
  déclenche indépendamment une silhouette de montagne plus proche
  (`DrawMountains`, jamais désassemblée) — sans rapport avec la table
  des 23 scènes.

## 4. Le piège d'unité de coordonnées

(Cause du bug "tout empaqueté en bas à gauche" corrigé précédemment.)
Les coordonnées de placement de sprites (`PutSprite_off`/
`FillRect_off`) sont en **octets** (le CGA compresse 4 pixels par
octet, donc les valeurs X ne dépassent jamais 0-80), alors que les
coordonnées de l'horizon/des étoiles (`DRAW`/`PLOT`) sont de vrais
**pixels** (0-319). Deux primitives différentes, deux conventions
différentes, sur le même écran de 320 pixels de large.

## 5. Une condition d'exclusion

Si le joueur se trouve sur une case de ville (certains index), toute la
table des 23 scènes est court-circuitée — les villes passent par un
chemin de code séparé, toujours non décodé, au lieu du décor de
nature.
