# Mapping fonctions Pascal (original) ↔ Python (remake)

Toutes les procédures/fonctions ci-dessous ont été retrouvées en lisant
directement `data/source code/*.PAS`/`*.INC`. Statuts :
- **Fidèle** : logique/formule reproduite à l'identique.
- **Approché** : principe repris, détails/coordonnées modernisés.
- **Non porté** : pas d'équivalent Python (fonctionnalité absente ou simplifiée ailleurs).

## `OVL_0000.PAS` — création du monde (`{WORLD CREATION}`)

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `INNER_BA2F` | tire une direction de creusement valide (évite les bords) | `src/tera/worldgen.py::dig_world` (boucle interne) | Fidèle |
| `INNER_BB8C` | qualité de mur aléatoire (1/9 secret, ~1/4 porte, sinon simple) | `worldgen.py::_random_wall_quality` | Fidèle |
| `INNER_BBD3` | creuse un monde entier (marche aléatoire 0xE1 pas) | `worldgen.py::dig_world` | Fidèle (principe) |
| `INNER_BD96` | place monstres/coffres/clés/carte/escaliers/Arioch sur tout le complexe (mondes 0-8 en un seul passage) | `worldgen.py::populate_dungeon` (monstres/coffres, formules exactes) + `worldgen.py::_place_unique_markers` (clés/carte/escaliers/Arioch) | **Partiellement approché** — voir note ci-dessous |
| corps principal (difficulté, argent initial, `Sav.f_Energy`) | init partie | `game.py::create_party`/`SaveGame(difficulty=...)` | Approché (`f_Energy` non modélisé, minijeu spatial absent) |

**Note `INNER_BD96`** : le taux/type de monstre et la formule de coffre
sont **fidèles** (`RANDOM(13-Difficulty)=2`, `RANDOM(11)+1` étages 0-1
vs `RANDOM(13)+8` étages 2+). En revanche la table de marqueurs uniques
(carte=code 101, clé=`0x5b+monde`, escalier haut=code 100 en case 0 des
mondes 2-7, Arioch=cellule `extra=2` en position ≥700, etc., avec des
compteurs `BBP_44[monde][0..5]` suivant quel marqueur a déjà été posé
par monde) n'est **pas** reproduite avec cette précision : le port
choisit une case atteignable au hasard par étage pour clé/carte/
escalier/Arioch, ce qui respecte l'invariant ("un de chaque par étage")
sans reproduire l'algorithme exact de sélection (voir aussi
`docs/TERA_notes_moteur.md`, la position de l'escalier descendant est
explicitement documentée comme non confirmée).

## `OVL_0010.PAS` — génération Amarande/PNJ (`{INIT CONSTANTS?}`)

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `INNER_BA2F(monde)` | case atteignable aléatoire pour placer un PNJ compagnon | — | **Non porté** (système de recrutement de 9 PNJ absent) |
| `INNER_BB28(charID)` | place un PNJ "sous"/"dans" un monde 0/1 | — | **Non porté** |
| `INNER_BBA5` | génère les 4 mots secrets (3 blocs de syllabes) | `src/tera/amarande.py::generate_secret_words` | Fidèle |
| `INNER_C509(idx)` | place un des 9 "lieux extra" (`f_LocExtra`), évite les villes | — | **Non porté** — remplacé par des positions fixes (`AMARANDE_CITE_ENTRANCE`/`AMARANDE_PYRAMIDE`/`AMARANDE_ALFOL` dans `amarande.py`), simplification documentée |
| corps principal | noms de perso aléatoires, noms des 9 compétences, montagnes, 20 villes | `amarande.py::generate_mountains`/`generate_towns` (montagnes+villes fidèles) ; noms de compétences → `combat.py::STAT_BY_INDEX` | Montagnes/villes fidèles ; recrutement PNJ non porté |

## `OVL_0022.PAS` — création de personnage (`{CHARACTER CREATION}`)

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `INNER_BA2F` | primitive d'affichage boîte de dialogue mode texte | `src/tera/render/chargen.py::_draw_box` | Approché (redessiné en Pygame, même habillage : fenêtre bordée, fond bleu, texte jaune/blanc/rouge clair) |
| corps principal | Force=`RANDOM(5)+5`, Intelligence conditionnelle, Chance=reste, répartition interactive des 2 sous-stats, 6 points bonus, boucle de reroll | `game.py::_roll_primary_stats`, `_split_stat`, `_create_one_character`, `create_party` | Fidèle |

## `OVL_00FB.PAS` — Amarande (déplacement + décor)

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `INNER_C377` | déplacement N/S/E/O, blocage montagnes, rebouclage, dispatch scène | `amarande.py::try_move` (déplacement) + `render/scene.py::draw_amarande_scene` (dispatch) | Fidèle |
| `INNER_BA2F(h)` / `INNER_BBA3(h)` | 2 profils de polyligne d'horizon (2 segments chacun) | `render/scene.py::_draw_stars_and_horizon` | Fidèle (coordonnées recopiées) |
| `INNER_BD0C` | étoiles aléatoires + horizon ; `Circle`/`FillShape` conditionnel non reproduit | `render/scene.py::_draw_stars_and_horizon` | Fidèle (sauf le Circle/FillShape mineur, non porté) |
| `INNER_BDCC`/`INNER_BE5A`/`INNER_BEA8` | bouquets d'arbres (3, 3, 5 sprites) | `render/amarande_scenes.py::_tree_small_a`/`_tree_small_b3`/`_tree_small_b5` | Fidèle |
| `INNER_BE1A` | décor fixe (2 arbres + 1 rocher) | `render/amarande_scenes.py::_be1a` | Fidèle |
| `INNER_BF20` | bosquet dense avec découpes noires | `render/amarande_scenes.py::_bf20` | Fidèle |
| `INNER_C01C` | bosquet avec le grand arbre "signature" | `render/amarande_scenes.py::_c01c` | Fidèle |
| `INNER_C0EC` | décor de sol dispersé (cailloux/touffes) | `render/scene.py::_draw_ground_scatter` | Fidèle |
| `INNER_C1DE` | créature/carcasse au sol (3 sprites) | `render/amarande_scenes.py::_c1de` | Fidèle |
| `INNER_C22C`/`INNER_C298`/`INNER_C2F2` | clusters de rochers/montagne | `render/amarande_scenes.py::_c22c`/`_c298`/`_c2f2` | Fidèle |
| `INNER_C34C` | son + message "montagne infranchissable" | `game.py::handle_amarande_command` (message texte, pas de son) | Approché |
| grand `CASE (Position MOD 23)` | 23 scènes fixes | `render/amarande_scenes.py::SCENES` (table complète) | Fidèle (23/23) |
| `DrawMountains` (appelée, jamais définie dans les overlays) | silhouette de montagne rapprochée | `render/scene.py::_draw_mountain_silhouette` | **Non confirmé** — primitive résidente jamais désassemblée, remplacée par une silhouette vectorielle approximative |

## `OVL_002E.PAS` — Cité des Crânes, monde 0 (couloir + combat)

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `INNER_BFCB`/`INNER_C211` | bord reculé gauche/droit en zigzag (tunnel emboîté) | `render/corridor.py::draw_corridor` (principe : plans emboîtés) | Approché — même PRINCIPE (rectangles emboîtés en filigrane magenta), coordonnées génériques pas recopiées telles quelles (spécifiques à ce monde dans l'original) |
| `INNER_C1A4`/`INNER_BDB1`/`INNER_C44C`/`INNER_C3DF`/`INNER_BEBC`/`INNER_C524`/`INNER_BB70`/`INNER_BBB8` | assemblage du tunnel selon qualité de mur à chaque profondeur/côté | `render/corridor.py::compute_planes`/`draw_corridor` | Approché (système générique, pas cette table de cas précise) |
| `INNER_C5FC` | indicateur de coffre (`SPR_05` si monstre 3/6/9, `SPR_3C` sinon) + déclenche `SpawnMonster` | `render/corridor.py::draw_chest_marker` (indicateur) + `game.py::handle_encounter`/`try_move_dungeon` (déclenchement combat) | Fidèle (indicateur) ; combat auto-déclenché confirmé par le manuel |
| `SpawnMonster` (appelée, jamais définie) | affiche réellement le monstre en combat | `render/corridor.py::draw_monster_encounter` + `render/sprites.py::monster_sprite_id` | **Non confirmé** — sprite purement décoratif par mot-clé sur le nom |
| `INNER_C792` | boucle principale déplacement + affichage couloir monde 0 | `render/app.py::handle_dungeon_key` + `game.py::try_move_dungeon` | Approché (logique de déplacement fidèle, rendu modernisé) |
| gestion `O`uvrir/`P`rendre + pièges (gaz/explosion/cyanure/lame) | résolution de coffre | `game.py::resolve_chest`/`_apply_trap` | Fidèle (formule de désamorçage confirmée par le manuel : Agilité+Habileté) |

## `OVL_00BC.PAS` — Kronopolis (commerce)

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `__DisplayPartyMoney` | affiche l'argent du groupe | `game.py::describe_shop` (inclus dans le texte) | Approché |
| `__Buying` | achat : 19 objets (A-S), prix fixes, formule `prix*30/(30+C_8315(perso,8,8)/5)` | `game.py::SHOP_ITEMS`, `_buy_price`, `combat.py::negotiation_roll`, `handle_shop_command` | **Fidèle** (corrigé — utilisait la stat Charme brute au lieu du jet `C_8315` ; clé n°9 mise à `9` fixe au lieu d'un incrément) |
| `__ComputeSellingPrice` | table de prix de rachat FIXES par objet (1-25) | `game.py::SELL_PRICES` | **Fidèle** (corrigé — table réelle portée, remplace l'ancienne approximation) |
| `__Selling` | vente : `prix_rachat + (prix_rachat DIV 100) * C_8315(perso,8,8)` | `game.py::_sell_price` | **Fidèle** (corrigé) |
| corps principal (`H`ôpital) | soin = `(PV manquants du perso le plus blessé) + 1` semaines, soigne tout le monde (PV + états) | `game.py::handle_shop_command` (branche `hopital`) | **Fidèle** (corrigé — faisait la somme des PV manquants de chaque perso au lieu du max) |
| corps principal (menu boutique) | **Simplification toujours assumée** : les 20 villes partagent cette boutique générique dans le port ; l'original a un dispatcher d'entrée par ville non décodé (`C_7309`) | `game.py::handle_amarande_command` (entrer sur une ville → boutique) | Approché, assumé |

## `OVL_0085.PAS` — Alfol (24 procédures internes)

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `INNER_CCBE`/`INNER_CAFC` | entrée dans la base intérieure (monde 9, cases 900-999) | `worldgen.py::generate_alfol_base`, `ALFOL_BASE_ENTRANCE` (amarande.py) | Approché (structure labyrinthe fidèle, position d'entrée fixée plutôt qu'aléatoire `Sav._75D0`) |
| marqueur `[5]=1` "BUREAU DES MÉDIAS" | tâche 2 (payer 1000 ducats) | `game.py::resolve_bureau_medias`, `CHEST_BUREAU_MEDIAS` | Fidèle (montant, condition de succès) |
| marqueur `[5]=2` dépôt radioactif | piège d'ambiance -1 PV | `worldgen.py::CHEST_RADIOACTIF` (placement) | Placement confirmé (section 20.6) ; effet -1 PV non retrouvé dans le port actuel |
| 20 autres procédures (surface, déplacement, décor) | scène de la surface d'Alfol | `render/scene.py::draw_alfol_surface_scene` | **Non individuellement mappées** — décor modernisé (seed pseudo-aléatoire par position), pas de table de scènes fixes comme pour Amarande |

## `OVL_00A4.PAS` — Meduz

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| corps + 4 procédures internes | donjon couloir, marqueurs méditer/invoquer | `worldgen.py::generate_meduz`, `game.py::resolve_meduz_invoquer`/`try_mediter` | Approché — structure/tâche fidèle, position exacte des marqueurs non retrouvée (section 20.6) |

## `OVL_00DB.PAS` — chargement des ressources

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| corps principal (lecture `CRAPIM.IMA`, `VALMON`, `OBJETS`, `VERBES`, `MESSAGE*`) | format sprite (largeur en octets, 2 plans entrelacés octet par octet, échelle par identifiant) | `tools/extract_sprites.py` (sprites), `tools/extract_text_data.py` (texte), `data_loader.py` | Fidèle (validé : 23206/23206 octets consommés exactement) |

## `TERAUTIL.INC` — moteur de combat (reconstruit par désassemblage x86)

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `C_8215`/`C_8270`/`C_82E5` | résolution de dégâts/riposte | `combat.py` (intégré dans `resolve_action`) | Fidèle |
| `C_8315(competence,stat,perso)` | jet de base : `(stat+competence-2)*(RANDOM(15)+3)`, seuil 44→jet-35 | `combat.py::skill_modifier`/logique de jet dans `resolve_action` | Fidèle — **seuils 44/35 à ne pas changer sans demande explicite** |
| `C_8391` | variante de jet | `combat.py::resolve_action` | Fidèle |
| `C_842A` | 9 branches par type d'action (souple/dur/sans objet) | `combat.py::resolve_action`, `ACTION_TYPES`, `FIXED_ITEMS` | Fidèle (9/9 branches confirmées) |
| `C_8D20` | résistance du monstre | `combat.py::monster_resistance` | **Non résolu** — bloqué par un lien statique de fonction imbriquée (section 15) |
| `C_8790`/`C_AFE8` | lecture clavier bas niveau pour le combat | `render/textinput.py`/`render/chargen.py` (ask/log Pygame) | Approché — modernisé (blueprint : gestion des entrées non fidèle intentionnellement) |
| `C_95D9_SpawnHP` | PV du monstre à l'invocation | `combat.py::spawn_monster_hp` | Fidèle |
| — (jamais retrouvée) | attaque du monstre sur le groupe | `game.py::_monster_turn` | **Explicitement non fidèle** — provisoire, marqué comme tel dans le code |

## `KBD.INC`

| Pascal | Rôle | Python | Statut |
|---|---|---|---|
| `C_4277` | lecture de touche bas niveau | `render/textinput.py::pygame_ask` | Approché (modernisé, blueprint section 5-6) |

## Fichiers non explorés / non pertinents pour le port actuel

`OVL_0069.PAS` (Temple de Seth ? — tâche 4, jamais codée), `OVL_00AE.PAS`
(minijeu spatial, hors scope), `OVL_00CB.PAS` (fin de partie),
`OVL_00D2.PAS`/`OVL_00D5.PAS` (sauvegarde disque bas niveau — remplacé
par `save.py` en JSON), `OVL_00E2.PAS` (écran d'aide), `OVL_00EC.PAS`/
`OVL_00F0.PAS`/`OVL_00F3.PAS`/`OVL_0142.PAS`/`OVL_0148.PAS` (écrans de
lieux "Draw..." dédiés — Sanctuaire de MAHALORE, Temple de Seth, Cité
des Crânes, Pyramide, Spatioport — jamais examinés en détail),
`OVL_0113.PAS`/`OVL_0115.PAS`/`OVL_012C.PAS` (écran titre/démos),
`OVL_0145.PAS` (animation de transition Meduz), `OVL_0049.PAS` (monde 1,
probable quasi-copie de `OVL_002E.PAS` pour un autre étage).

## Divergences trouvées en construisant ce mapping — CORRIGÉES

En relisant `OVL_00BC.PAS` en entier pour ce document, quatre écarts
concrets sont apparus entre la boutique de ville et l'original ; tous
corrigés dans `game.py`/`combat.py` (voir tableau `OVL_00BC.PAS` ci-dessus) :

1. **Clé n°9** : l'original fixe `Sav.f_Keys[9] := 9`, le port faisait
   `save.keys[9] += 1`.
2. **Formule d'achat réelle** : `prix*30/(30+C_8315(perso,8,8)/5)` —
   le port utilisait la stat Charme brute au lieu du jet `C_8315`
   (`combat.py::negotiation_roll`, même formule de base que le combat).
3. **Formule de vente réelle** : `prix_rachat + (prix_rachat DIV 100) *
   C_8315(perso,8,8)` à partir d'une **table de prix de rachat fixe par
   objet** (`__ComputeSellingPrice`, 25 valeurs, `game.py::SELL_PRICES`)
   — pas l'ancienne approximation (`base_price//2 + charme//5`).
4. **Hôpital** : coût = `(PV manquants du personnage le plus blessé) + 1`
   semaines pour soigner **tout le groupe d'un coup** (et guérir les
   états) — pas la somme des PV manquants de chaque personnage.
