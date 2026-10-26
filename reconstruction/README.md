# Reconstruction fidèle de TERA — nouveaux travaux Codex

Tout le travail introduit par Codex dans cette session est regroupé dans
`reconstruction/`. Le dépôt historique (`src/`, `docs/`, `tools/`, `tests/`,
README et CLAUDE) reste inchangé. Les originaux dans `data/` et `manuel/`
sont consultés comme sources, sans déplacement ni modification.

Référence historique : commit `0ec307bd095268d5dc63985e858397299164a2b7`.
Les nouveaux documents, outils, tests, correctifs et le moteur C sont issus
du travail de Codex dans cette session ; `docs/cadre_projet.md` conserve
le texte fourni par l'utilisateur. Les sources originales gardent leur
provenance et leur place dans le dépôt historique.

État courant : **lieux et interactions intégrés en C / SDL3**, contexte 4.
La [cible utilisateur de 95–98 %](docs/TERA_FIDELITY_TARGET.md) donne la priorité
aux différences perceptibles en jeu. L'[audit exécuté](docs/TERA_PORT_AUDIT.md)
et le [registre des écarts](docs/TERA_PORT_ISSUES.md) distinguent les preuves
des réserves ; aucune fidélité globale chiffrée n'est encore mesurée.
Dernière vérification : **44/44 suites SDL/oracle** passent ; la photographie
d'équipement après achat reste deux secondes puis le catalogue revient,
avec purge des touches anciennes comme dans le binaire original.
L'[audit runtime](docs/TERA_RUNTIME_DEPENDENCIES.md) confirme TERA.COM comme
REFERENCE-ONLY et l'absence de Python au lancement. Le nouveau frontend
`frontend/sdl3/` crée une partie, transmet les commandes au core et affiche
la création en 80 colonnes, les fiches, paysages, lieux et interactions en 320 x 200.
Compilation, lancement, commandes, tests et écarts :
[statut SDL3](docs/TERA_SDL3_STATUS.md). **39 suites CTest sur 39 passent**,
dont un test SDL dans un paquet local sans COM/overlays/Python. Les sept
tables de textes, 82 sprites et six pages CGA sont chargés directement ;
la bannière de démarrage et les noms/phrases originaux sont affichés avec
les accents. [Formats, preuves et limites des assets](docs/TERA_ASSETS.md).
La création, H/I, H/S et les sélections d'inventaire disposent de leurs écrans.
Deux [campagnes complètes SDL](docs/spec_sdl_campaign_validation.md) atteignent
la victoire par les événements clavier et le rendu de production, avec
comparaison des états et sauvegardes F5/F9 pendant le parcours.
Voir [portée et limites graphiques](docs/spec_character_screens_c.md).
Les points 4 et 5 de la mission couvrent désormais les dix lieux, les 23
compositions d'Amarande, les couloirs orientés, Alfol/Meduz, coffres/pièges,
dialogues/recrutement, catalogue/achat/vente/hôpital et voyage spatial.
[Spécification, validation et limites du rendu](docs/spec_scene_interactions_c.md).
Le combat terrestre dispose de son affichage : combattant, confirmations,
cri et résultats. Sons de combat, pas, alertes, explosions et délais sont
raccordés à SDL ; le monstre 15 peut être interrompu après chaque image.
[Comparatif original/C/SDL des étapes 6 et 7](docs/spec_combat_media_sdl_c.md).
Les trois figures procédurales manquantes sont intégrées. Ambiances de
marche, sept lieux, entrée souterraine, dépôt Alfol et voyage spatial sont
sonorisés ; 11 partitions sont comparées aux routines originales exécutées.
Les [compléments des scènes](docs/spec_scene_completion_c.md) ajoutent
montagnes/lune, tours/portraits, décors aléatoires, Alfol, catalyseur de Meduz,
rayon spatial et palettes par lieu. Cartes/aide, textes de Seth, fins et
tunnels sont intégrés. Le [profil IBM PC/XT](docs/spec_visual_xt_c.md) ajoute
les polices CP437 IBM, les proportions du mode texte, le raster natif des
cartes/tunnels et leur cadence variable à 4,77 MHz. Restent notamment les
sons de progression et les autres courbes/remplissages à comparer.

Bloc menus : textes et catalogue de Kronopolis repris des sources PAS/DATA,
liste de revente originale, halte H sur le décor (sans fiche anticipée),
confirmations distinctes et grille de bonifications sur trois lignes.
[Audit des menus et écarts restants](docs/spec_menus_sdl_c.md).

Le premier bloc de présentation du core est disponible :
[contrat, événements typés et invites](docs/spec_presentation_c.md).
SDL reçoit mouvements/refus, messages/indices, coffres/pièges, inventaire,
commerce/soins, recrutement, changements de zone, objectifs et fin. Deux
campagnes avec observation et reprises conservent les résultats antérieurs.

L'orchestration commune du moteur C provient du contexte 3.
L'API `engine/include/tera/game.h` relie création, exploration, lieux,
combats, voyages, succession et fin de partie. Voir
[le contrat et les tests directs C](docs/spec_orchestration_c.md).
Compilation et tests C ne nécessitent aucun Python. Les outils terminal
présentés dans les anciens jalons ci-dessous restent historiques et
optionnels ; aucune nouvelle interface temporaire n'est développée.
Le frontend SDL définitif appartient au contexte 4.

Validation courante après reconstruction demandée du test spatial :
**24 suites exécutées, 24 réussies, aucune exclusion**. Le nouvel exécutable
spatial et les tests d'entrées passent sans blocage observé. Aucune liste
blanche n'a été ajoutée. Le fichier initial de l'alerte reste non analysé ; voir
[l'audit de sécurité](docs/audit_securite_test_space_2026-10-16.md).
La nouvelle suite couvre les interruptions du monstre 15 et les entrées
spatiales horodatées, sans interface :
[contrats, validation et tâches restantes](docs/spec_entrees_gameplay.md).
La [sauvegarde/reprise en C](docs/spec_sauvegarde_c.md) conserve le monde et
le RNG aux frontières de commande : les deux campagnes atteignent la même
victoire après 34 et 30 reprises. Un banc C natif facultatif compare aussi
236 cas d'entrée au binaire original, sans écart et sans interpréteur Python.
La [comparaison globale TERAMAIN/DOS](docs/validation_teramain_dos.md) suit
désormais deux parties jusqu'à la victoire : 7 143 touches et 3 990 frontières
comparées sans écart après correction de deux divergences du port C.
Les résultats des six suites de modules ci-dessous sont des jalons antérieurs.
Voir [leur couverture et leurs limites](docs/validation_modules_c.md).
Deux [parcours complets C sur mondes générés](docs/validation_parcours_c.md)
atteignent les quatre objectifs et la victoire avec score, par les seules
commandes du moteur, sans repositionnement ni objectifs prévalidés.

| Dossier | Contenu / provenance |
|---|---|
| docs/ | Cadre utilisateur, nouvel audit, spécification BIN/PAS/DATA |
| engine/ | Nouveau moteur C portable, dont le combat interactif |
| frontend/ | Frontends terminal et ponts du moteur C : combat, monde, exploration, objets |
| tests/ | Nouveaux tests de provenance, contrats C et intégration frontend |
| tools/ | Nouveaux outils de capture BIN et de test isolé |
| patches/ | Corrections Python issues de l'audit, conservées séparément et non appliquées au prototype historique |
| build/ | Preuves locales, compilations et copie Python corrigée ; exclu de Git |
| AGENTS.md | Consignes pour ces nouveaux travaux |

Les corrections Python ont été retirées de `src/`. Pour les examiner,
lire `patches/python-combat-corrections.patch`. Le script de test copie
le Python historique dans `build/python_reference`, y applique le patch
et utilise les données originales en lecture seule. Le Python historique
lancé normalement conserve donc son ancien comportement.

Depuis la racine du dépôt :

```powershell
python reconstruction/tools/audit_combat_binary.py
python reconstruction/tools/test_python_reference.py
cmake -S reconstruction -B reconstruction/build/cmake -G "MinGW Makefiles"
cmake --build reconstruction/build/cmake
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

Le moteur propose maintenant un **combat interactif complet et jouable
isolément** : initialisation des monstres, menu, confirmations/annulations,
mot, RNG original, tours, fuite liée aux cartes et récompenses.
Voir [la spécification du combat interactif](docs/spec_combat_interactif.md)
et [la comparaison exécutée avec le binaire](docs/validation_binaire.md).
La génération de partie, la création du héros et le recrutement sont aussi
disponibles ci-dessous. La marche sur Amarande et son calendrier quotidien sont aussi disponibles
(lots 9 et 10 ci-dessous). La cité des crânes, son entrée/retour et les
interactions des niveaux sont disponibles (lots 11 et 12), ainsi que les
coffres/pièges et l'inventaire (lots 13 et 14). Les autres lieux/mondes,
leurs objets spécifiques et le rendu du jeu complet restent à reconstruire.

Pour manipuler le combat avec une fixture **synthétique annoncée** :

```powershell
python reconstruction/frontend/combat.py --demo --monster 7 --seed 1986
```

T/L/J/D/A/P/C/F lisent une touche ; O ou Entrée confirme, toute autre
touche annule. Une deuxième annulation consomme le tour, comme dans
l'original. C demande ensuite le mot. La sortie complète est enregistrée
par défaut dans `build/combat_result.json`.

Pour un véritable état de partie, fournir un JSON avec les champs décrits
par `frontend/combat.py::pack_game`, ou un bloc brut `tSavedata` de 10 162
octets. Ce bloc fournit personnages, équipements, H, mots et cartes ;
les champs hors record sont explicitement requis :

```powershell
python reconstruction/frontend/combat.py --savedata CHEMIN_DU_BLOC --world 4 --on-alfol 0 --previous-position 419 --monster 7 --seed 1986
python reconstruction/frontend/combat.py --state reconstruction/build/combat_result.json --monster 8
```

Aucune carte ni statistique n'est créée en mode `--state`/`--savedata`.
`--resources` indique le dossier local des ressources originales.
`--output` indique le fichier JSON de sortie ; le fichier d'entrée est lu
sans modification. Sans `--seed`, un état JSON conserve sa graine ; une
fixture/import prend l'heure locale au format RANDOMIZE original.
Le bloc brut n'est pas un conteneur de sauvegarde DOS certifié, et l'export
JSON ne prétend pas réécrire une disquette originale.

Vérifier les nouveaux blocs :

```powershell
python reconstruction/tools/compare_interactive_binary.py
python reconstruction/tests/test_combat_frontend.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

Le banc interactif requiert Unicorn installé localement sous
`build/emulator_deps` (voir `docs/validation_binaire.md`). Le combat jouable
n'utilise que Python standard et le moteur C compilé : aucun émulateur requis.
Le port C99 compile ici avec GCC `-Wall -Wextra -Werror -pedantic`.

## Création des personnages (lot 6)

Le [bloc personnages](docs/spec_creation_personnages.md) ajoute les neuf
compagnons (noms NOMPERS, spécialités, caractéristiques, équipement), la
création interactive du héros, ses répartitions et bonus, les refus et
la recréation après défaite. Le héros neuf commence sans compétence ni arme.

```powershell
python reconstruction/frontend/creation.py --seed 1986
python reconstruction/frontend/combat.py --demo --characters reconstruction/build/characters.json --monster 7
```

Le nom utilise lettres/retour arrière/Entrée ; H ou F est une touche,
les répartitions sont des chiffres, puis six touches de bonus F/I/N/E/V/T/A/C/H.
À la dernière invite, N recommence ; toute autre touche conserve le héros.
Ces détails sont ceux du binaire, y compris zéro accepté dans les répartitions.

`build/characters.json` exporte le roster, sans fabriquer de cartes.
Le combat `--demo --characters` conserve ce véritable profil et utilise
un contexte de test annoncé. Avec `--state`, les cartes/H/mots de cet
état sont préservés. Pour recréer le héros en conservant les compagnons
et l'équipement : `creation.py --roster CHEMIN_ROSTER`.

Ce frontend isolé commence au bloc personnages. Pour conserver le
calendrier RNG complet des cartes jusqu’au héros, utiliser `world.py`
ci-dessous. L’initialisation des compagnons ne les recrute pas.

```powershell
python reconstruction/tools/audit_creation_binary.py
python reconstruction/tools/compare_creation_binary.py
python reconstruction/tests/test_creation_frontend.py
```

Validation : 662 scénarios contre les instructions originales, 11 128
frontières clavier, aucun écart. Six suites CTest et sept tests des deux
frontends passent. Les nouveaux travaux restent dans `reconstruction/`.

Les campagnes numériques antérieures restent disponibles :
`tools/compare_combat_binary.py`, `tools/compare_round_binary.py`,
`tools/compare_encounter_binary.py`, `tools/compare_combat_exit_binary.py`.
Leurs injections sont documentées dans les spécifications correspondantes ;
le nouveau banc interactif exécute le RNG et le menu originaux.

Pour les commits : utiliser le chemin `reconstruction/` pour ces travaux.
Les sujets peuvent distinguer `docs(reconstruction): ...`,
`feat(reconstruction-engine): ...`, `test(reconstruction): ...` et
`tools(reconstruction): ...`. Un éventuel transfert au Python historique
devra être un commit séparé et explicitement nommé. Aucun commit n'a été
créé automatiquement.


## Génération mondiale et recrutement (lots 7 et 8)

Les deux nouveaux blocs sont décrits dans [la génération du monde](docs/spec_generation_monde.md)
et [le recrutement](docs/spec_recrutement.md). Les cartes, villes, montagnes,
placements, indices, mots secrets et H sont issus des overlays originaux.
La création du héros poursuit leur graine. Pour tester depuis la racine :

```powershell
python reconstruction/frontend/world.py --seed 1986 --difficulty 1
python reconstruction/frontend/combat.py --state reconstruction/build/new_game.json --monster 7
```

Le premier programme génère le monde puis demande nom, genre, trois
répartitions et six bonus. Le second lance le banc de combat avec cet état.
`--monster` est requis ici : les rencontres automatiques attendent la boucle
d’exploration. Il n’y a pas encore de sortie graphique.

Tester le recrutement à une position de compagnon générée, sans simuler
le déplacement (banc explicitement annoncé). Pour cette graine et difficulté,
les compagnons 7 et 9 se trouvent tous deux à 957 :

```powershell
python reconstruction/frontend/recruitment.py --state reconstruction/build/new_game.json --at 957
python reconstruction/frontend/recruitment.py --state reconstruction/build/new_game.json --request solliciter
```

O/Entrée confirme chaque commande ; une autre touche annule. La sortie
est `build/recruited_game.json`. Sans `--at`, le lieu reste celui de l’état.
Un échec de négociation peut payer 250, comme dans le jeu. La commande
peut activer plusieurs compagnons ; les anomalies originales sont conservées.
L’export JSON ne prétend pas être une sauvegarde DOS.

Validation : **122 générations + 6 créations après génération, 696 recrutements,
970 tours et 792 combats interactifs, aucun écart**. Sept suites CTest et
onze tests des frontends passent. La campagne mondiale vérifie 2 186 881
appels au RNG original et deux générations rejetées. Les campagnes de combat
couvrent aussi les groupes de cinq à dix personnages issus de recrutements multiples.

```powershell
python reconstruction/tools/audit_world_recruitment_binary.py
python reconstruction/tools/compare_world_binary.py
python reconstruction/tools/compare_recruitment_binary.py
python reconstruction/tools/compare_interactive_binary.py
python reconstruction/tools/compare_encounter_binary.py
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

Les rapports et preuves sont sous `build/audit/`. Tout reste dans
`reconstruction/`, séparé du dépôt historique. Les lots 9 et 10 ajoutent maintenant la marche sur Amarande et le passage
du temps. Les autres déplacements et interactions/objets/événements, puis
leur rendu et leur validation dans un parcours complet restent à traiter.


## Exploration d’Amarande et calendrier quotidien (lots 9 et 10)

La [spécification exploration/temps](docs/spec_exploration_temps.md)
décrit les deux nouveaux blocs C :

- marche 2/4/6/8, montagnes, bouclage de la grande carte et détection des vingt lieux ;
- jours, récupération, poison, rencontres automatiques, combat puis reprise,
  niveaux, caractéristiques et compétences du groupe.

Les tirages des décors sont conservés : le terminal ne change pas la
prochaine rencontre en supprimant le dessin. Le passage du temps attend
la fin effective du combat avant de reprendre ses opérations.

Depuis la racine, après compilation :

```powershell
python reconstruction/frontend/world.py --seed 1986 --difficulty 1
python reconstruction/frontend/exploration.py --state reconstruction/build/new_game.json
```

Le premier programme crée le monde et le héros. Le second insère l’arrivée
originale, puis permet **8 nord, 2 sud, 6 est, 4 ouest**. Un combat éventuel
utilise T/L/J/A/D/P/C/F, puis O/Entrée pour confirmer. **Q** exporte et quitte
l’hôte terminal ; la sortie est `build/exploration_result.json` et peut être
rechargée avec `--state`. Le fichier source est préservé.

**S** exécute séduire avec la sollicitation déjà présente dans l’état.
Il n’ajoute pas une sollicitation automatique. Le dialogue solliciter reste
un banc séparé (`recruitment.py`) en attendant le port des dialogues complets.
Les lieux sont détectés ; E entre dans la cité des crânes au lieu 2.
Les autres entrées ne sont pas encore jouables. Les déplacements d’Alfol, Meduz, les autres
interactions/objets, objectifs et le rendu graphique restent à construire.

Validation : **798 scénarios contre le binaire, 848 frontières, 49 015 appels
au RNG original et 25 rencontres imbriquées, aucun écart**. Les onze types
possibles de rencontre errante ont chacun un cas dédié. Huit suites CTest
et quinze tests frontend passent, dont création → arrivée → marche → export,
interruption sans export et combat → reprise du calendrier.

```powershell
python reconstruction/tools/audit_exploration_binary.py
python reconstruction/tools/compare_exploration_binary.py
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

Rapports locaux : `build/audit/exploration_comparison.json` et
`exploration_manifest.json`. Le chemin d’animation du monstre 15 sans
interruption clavier est validé (six images) ; sa coupure anticipée reste
à vérifier. Les erreurs de division DOS et les groupes impossibles sans
compétence connue sont signalés, sans état final fabriqué.
Tout le travail reste dans `reconstruction/`, sans commit automatique.

## Cité des crânes et interactions des niveaux (lots 11 et 12)

La [spécification de la cité](docs/spec_navigation_cite.md) détaille le
port C des overlays 002E/0049, leur calendrier et leurs rencontres fixes.
Le terminal d'exploration accepte les états de surface et de cité :

```powershell
python reconstruction/frontend/exploration.py --state reconstruction/build/new_game.json
python reconstruction/frontend/exploration.py --state reconstruction/build/exploration_result.json
```

Sur Amarande, **E** au lieu 2 entre après confirmation. Dans la cité,
**8** avance, **2** recule, **6** va à droite et **4** à gauche, selon
l'orientation courante. **O** ouvre une porte ; **U** utilise la clé.
**D/M** descendent/montent, **L** lit les indices, **I** invoque à l'autel
ou demande un mot selon le lieu. O/Entrée confirme ces dernières actions.
Les monstres fixes déclenchent le combat existant. F reprend la visite à
la destination originale ; sortir restaure la position de surface.

Q exporte vers `build/exploration_result.json`. La reprise conserve les
portes, monstres retirés, clés, objectifs, position/précédente, personnages
et graine. Les sons restent inaudibles, mais **leurs tirages sont exécutés**
dans le moteur, y compris les pas, sorts, coups et musiques. Les effets
de couleur du combat conservent aussi leurs tirages, même en silence.

```powershell
python reconstruction/tools/audit_navigation_binary.py
python reconstruction/tools/compare_navigation_binary.py
python reconstruction/tools/compare_city_entry_binary.py
python reconstruction/tools/compare_interactive_binary.py
python reconstruction/tools/compare_exploration_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
```

Validation actuelle : **875 cas de navigation, 51 d'entrée/retour,
854 d'exploration et 946 combats**, tous sans écart avec le binaire.
**9 suites C et 20 tests frontend passent.** Les anciennes campagnes
d'exploration/combat indiquées plus haut sont remplacées par ce périmètre
qui conserve les tirages des sons et couleurs.

Les rapports datés et leurs limites figurent dans
[validation_binaire.md](docs/validation_binaire.md). Les coffres O/P et
l'inventaire sont disponibles dans les lots suivants. Les autres lieux
et mondes, boucle de fin et rendu graphique
restent à porter. Ces lots ne déclarent pas le jeu entier terminé.

## Coffres/pièges et inventaire/équipement (lots 13 et 14)

Ces deux blocs C sont reliés au terminal existant. Voir
[la spécification des objets](docs/spec_coffres_inventaire.md).

- **O** ouvre un coffre dans la cité ; O/Entrée confirme, puis A-D choisit
  le volontaire. Le piège, son éventuel désamorçage et les sons utilisent
  le RNG original. L'argent est obtenu à l'ouverture.
- **P** prend le contenu d'un coffre ouvert : objet, clé ou carte. Un
  inventaire plein/HS laisse l'objet dans la case. Changer de vue referme
  le coffre ; ouvrir et prendre sont deux actions distinctes.
- **A puis O** abandonne un objet ; **A puis C** abandonne un compagnon.
- **B** boit une potion : personnage A-D puis slot 0..9. La potion 23
  restaure HP/MP, la 24 guérit EM. Les autres objets sont conservés.
- **N puis D**, O/Entrée, source, slot, cible : donne un objet. Un refus
  de réception conserve l'objet source. Les autres dialogues de N sont
  maintenant reliés par le lot 15 ci-dessous.

Les noms d'objets et les dix slots viennent des données originales.
L'équipement contribue directement aux bonus de combat ; aucun système
supplémentaire « équiper/déséquiper » n'est ajouté. Le lecteur de slot
redemande sur ESC/lettres comme le COM. Q exporte l'état moderne ; une
interruption ne publie pas de sortie. Le fichier d'entrée reste inchangé.

```powershell
python reconstruction/frontend/exploration.py --state reconstruction/build/new_game.json
python reconstruction/tools/audit_inventory_chest_binary.py
python reconstruction/tools/compare_chest_binary.py
python reconstruction/tools/compare_inventory_binary.py
```

Validation : **1 076 cas de coffres/pièges et 632 cas d'inventaire,
aucun écart avec le binaire**. Les 875 cas de navigation et 51 cas
d'entrée/retour sont relancés sans écart. **10 suites C et 26 tests
frontend passent**, avec ouverture/prise, inventaire plein, abandon,
transfert, potions et sauvegarde/rechargement.

Les autres lieux et leurs objets spécifiques, mondes 9/10,
Alfol/Meduz, voyages, autres objectifs/fin victorieuse,
puis rendu graphique et sonore restent à porter. Tout le
nouveau travail est sous `reconstruction/`, sans commit automatique.

## Dialogues et reprise après défaite (lots 15 et 16)

Le menu **N** propose maintenant informations, acheter, vendre, donner
et solliciter. **S** direct utilise le contexte de sollicitation/information
pour la séduction. Les réponses A/V viennent des ressources originales ;
les transactions des boutiques restent propres aux lieux à porter.

Après la mort du groupe, les condoléances sont suivies de la création
d'un successeur dans le même monde, avec le butin conservé, puis de
l'arrivée automatique. La graine continue après les tirages de la musique.
Si les quatre objectifs sont déjà vrais, la boucle se termine après les
condoléances, conformément au binaire.

Voir [la spécification des dialogues et de la succession](docs/spec_dialogues_succession.md).
**466 scénarios de dialogue et 120 de succession sans écart au binaire ;
11 suites C et 33 tests frontend passent.** Les originaux restent inchangés.

```powershell
python reconstruction/frontend/exploration.py --state reconstruction/build/new_game.json
python reconstruction/tools/audit_dialogue_lifecycle_binary.py
python reconstruction/tools/compare_dialogue_binary.py
python reconstruction/tools/compare_lifecycle_binary.py
```

La fin victorieuse avec score, les autres lieux/mondes/voyages et objectifs,
puis le rendu graphique et sonore restent à porter et à valider.

## Kronopolis et temple de Seth (lots 17 et 18)

**E** entre désormais à Kronopolis (lieu 0) et chez Seth (lieu 4), après
O/Entrée. [Règles et preuves](docs/spec_kronopolis_seth.md).

- **Kronopolis** : A acheter, V vendre, H hôpital. Achats A..S, ventes par
  slot 0..9 ; objets, potions, énergie, cartes et clé avec prix négociés.
  ESC revient au menu, puis sort. L'hôpital gratuit avance le temps et
  restaure HP/MP selon les règles originales.
- **Seth** : déplacements relatifs, portes/clés, monstres, calendrier,
  coffres et pièges propres au temple ; L lit, I invoque à l'autel avec
  le quatrième mot secret. Sa victoire valide le quatrième objectif une
  seule fois. Revenir à 799 retrouve la surface et sa position sauvegardée.

Les façades conservent leurs tirages et l'état est repris au rechargement.
**512 cas de commerce, 274 de Seth et 16 de façades sans écart au binaire** ;
887 cas de cité, 1 076 de coffres et 51 d'entrée/retour relancés sans écart.
**11 suites C et 41 tests frontend passent.** Tous les ajouts restent dans
`reconstruction/`, dépôt historique inchangé, aucun commit automatique.

```powershell
python reconstruction/frontend/exploration.py --state reconstruction/build/new_game.json
python reconstruction/tools/audit_commerce_seth_binary.py
python reconstruction/tools/compare_commerce_binary.py
python reconstruction/tools/compare_seth_binary.py
python reconstruction/tools/compare_places_binary.py
```

Les autres lieux et voyages, mondes 9/10, Alfol/Meduz, autres objectifs,
fin victorieuse/score et rendu graphique/sonore restent à terminer.

## Spatioport et Alfol (lots 19 et 20)

**E** au spatioport (lieu 1) permet désormais de partir pour Alfol :
sélection du pilote, carburant, tirs contre les pirates et voyage retour.
Le combat spatial utilise le pavé **1..9**, avec une fenêtre de tir en
terminal ; manquer laisse le vaisseau encaisser. Sa destruction termine
la partie, comme le HALT original.

Sur **Alfol**, 2468 parcourt la surface circulaire ; **E** à la ville ouvre
les bâtiments du monde 9, avec portes/clés, dépôts radioactifs, PNJ,
monstres et bureau des médias. Le paiement de 1000 valide le deuxième
objectif. **R** au point zéro déclenche le retour au spatioport.
L'export/rechargement conserve la position, les pirates, le carburant,
les objectifs et les particularités de l'original.

[Règles, preuves et limites du terminal](docs/spec_spatioport_alfol.md).
**173 scénarios de vol, 120 de pilotage et 613 d'Alfol sans écart avec
les instructions originales**, soit 906 cas et 2 133 états comparés.
**11 suites C et 54 tests frontend passent**, dont treize nouveaux parcours
de voyage, reprise, médias, radiations, combat et mort du groupe sur Alfol.
Les sons et dessins conservent leurs tirages ; les graphismes et sons
audibles restent abstraits. Tous les nouveaux travaux sont dans
`reconstruction/`, sans modification de l'historique ni commit automatique.

```powershell
python reconstruction/frontend/exploration.py --state reconstruction/build/new_game.json
python reconstruction/tools/audit_space_alfol_binary.py
python reconstruction/tools/compare_space_binary.py
python reconstruction/tools/compare_alfol_binary.py
```

Restent principalement **Meduz/monde 10 et son objectif**, les autres
lieux/événements d'Amarande, la victoire et le score, puis la validation
d'un parcours complet et la sortie graphique/sonore.

## Meduz et fin victorieuse (lots 21 et 22)

À la **pyramide** (lieu 3), **M**, O/Entrée et le mot de voyage ouvrent
Meduz, monde 10. Navigation relative **2468**, combats, calendrier et
fuites ; **M** au catalyseur étend les facultés mentales une seule fois,
puis **I** au sanctuaire avec le mot requis valide l'objectif de Meduz.
Le retour par 999 restaure la position de la pyramide et ajoute sept jours.
Les animations de voyage conservent tous leurs tirages.

Les **quatre objectifs accomplis** déclenchent maintenant la fin au retour
sur Amarande : félicitations et score si le groupe est disponible,
condoléances sans successeur sinon. Le calcul et le texte du score sont
comparés au runtime Pascal, avec sa précision Real48, ses troncatures et
l'erreur de division par zéro. Le snapshot conserve la fin acquittée.

[Règles et limites détaillées](docs/spec_meduz_fin.md).
**326 scénarios Meduz, 48 de voyage et 510 de fin sans écart au binaire**,
soit 884 cas et 1 904 états comparés. Les quatre mécanismes d'objectif
et la fin sont portés ; le parcours complet du jeu reste à valider.
**11 suites C et 67 tests frontend passent**, dont treize nouveaux parcours
de voyage, méditation, invocation, mort, retour, score et reprise de fin.

```powershell
python reconstruction/frontend/exploration.py --state reconstruction/build/new_game.json
python reconstruction/tools/audit_meduz_ending_binary.py
python reconstruction/tools/compare_meduz_binary.py
python reconstruction/tools/compare_ending_binary.py
```

Restent les autres lieux/événements secondaires d'Amarande, le conteneur
de sauvegarde DOS, la validation d'un parcours intégral et le rendu
graphique/sonore. Tous les ajouts sont sous `reconstruction/`, code commenté,
dépôt historique inchangé ; aucun commit automatique.

## Lieux et événements secondaires d'Amarande

Le **sanctuaire de Mahalore**, le **domaine de Balgence**, l'**ermitage de
la Roche aux Goules**, le **hameau de la Porte des sables** et l'**abbaye
de Mortefontaine** sont raccordés : façades, conditions originales de
présence des personnages, informations, sollicitation, séduction et combats.
Leurs événements utilisent N/S/C ; E affiche sa réponse originale sur Amarande.

Les panneaux **L** indiquent la direction signée de Kronopolis avec leur
confirmation et leur pause. **H** donne accès à la halte : déplacements
rapides, carte locale, dormir/manger avec le vrai calendrier, aide,
introspection, sondage des compagnons et sortie confirmée. **I** permet
l'invocation résidente ; **#** contrôle le drapeau sonore. Les réponses
génériques proviennent des ressources originales, sans tirage Python.

Les dessins conservent leurs tirages RANDOM, y compris les façades de
retour de la Cité et d'Alfol. La conservation de D9B66 après un jour
d'Alfol a également été corrigée et recontrôlée avec le COM.

[Règles, preuves et limites](docs/spec_amarande_secondaire.md).
**320 scénarios de façades et 311 de commandes/panneaux sans écart**,
dont les ID/variantes des textes, les victoires de PNJ et les pauses.
Régressions exploration : **854 cas sans écart** ; Alfol : **613 cas
sans écart**. Les tests du terminal couvrent arrivées, indices, carte,
halte, annulations, combats, retours, interruptions et reprise JSON.
**11 suites CTest et 78 tests frontend passent.**

```powershell
python reconstruction/frontend/exploration.py --state reconstruction/build/new_game.json
python reconstruction/tools/audit_amarande_binary.py
python reconstruction/tools/compare_amarande_facades_binary.py
python reconstruction/tools/compare_amarande_binary.py
```

Restent la validation d'un parcours intégral sur un monde généré, le
conteneur de sauvegarde DOS et le rendu graphique/sonore. Les travaux
restent isolés sous `reconstruction/`, sans modification de l'historique.
