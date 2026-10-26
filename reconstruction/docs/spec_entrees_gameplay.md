# Entrées qui influencent le gameplay — contexte 3

## Portée et provenance

Le moteur expose des frontières de clavier et un temps virtuel, sans clavier
OS, horloge réelle, attente ni interface. Les références BIN relevées dans le
travail précédent sont 7872/792E pour le monstre 15, C4210 avant lecture, et
BFB2/C01E dans OVL00AE pour les purges spatiales. Les règles de voyage et les
campagnes numériques sont décrites dans `spec_spatioport_alfol.md`.

**CONFIRMÉ — tests C** : les contrats ci-dessous sont exercés directement par
`tests/test_input_c.c`. Ces tests ne constituent pas une nouvelle exécution
différentielle du binaire DOS. La comparaison native C décrite plus bas complète
maintenant cette preuve pour les cas exécutés ; le temps réel et le clavier SDL
appartiennent au contexte 4.

## Présentation du monstre 15

`TeraInputSource` est un service facultatif emprunté à l'appelant. Son contexte
et son pointeur de fonction ne sont jamais des données de sauvegarde. Le callback
ne bloque pas, ne rappelle pas le moteur et ne modifie pas son état ou son RNG.
Une source absente ou sans callback signifie qu'aucune touche n'est disponible.

La présentation purge d'abord les anciennes touches. Chaque image consomme
successivement R60, R24 et R255, puis teste la présence d'une nouvelle touche.
Elle termine après la première détection ou après six images. Une interruption
après n images consomme donc 3*n tirages, son actif ou inactif. Le test de présence
ne lit pas la touche. L'orchestration purge celle-ci avant la nouvelle commande
de combat : interrompre le dessin ne choisit pas une action.

Les tests couvrent les six interruptions possibles, l'absence d'interruption,
les deux états sonores, les sources facultatives et l'entrée effective dans un
combat par `tera_game_key`.

## Tir spatial horodaté

`tera_space_input_at` reçoit un temps en millisecondes depuis le début du tir
courant et une touche 0..255 ; -1 avance le temps sans nouvelle touche.
Les événements doivent être monotones. La date maximale est `delay_ms+990`.

Le délai initial vaut R10*100. Les purges ont lieu à `delay_ms` et
`delay_ms+300`. Un événement exactement à une purge est effacé. Ensuite la
première touche est conservée jusqu'au poll final, même si elle est incorrecte.
Une touche exactement au poll final est acceptée. Les étapes de présentation
230/200/170/90 ms n'ajoutent ni lecture de touche ni tirage aléatoire.

L'API n'ajoute aucun tirage au chemin sémantique `tera_space_key`. Elle résout
le tir à l'échéance ; si un nouveau tir commence, le temps, l'étape et la touche
en attente reviennent à leur état initial. L'appelant fournit explicitement les
événements du tir suivant, sans transférer automatiquement un dépassement de temps.

Le temps rétrograde, une échéance dépassée, une touche hors domaine, une session
nulle ou une phase autre que SHOT sont refusés sans modifier la session. La façade
`tera_game_space_input_at` réintègre aussi la fin du voyage et l'atterrissage.

Les tests couvrent avant/à/après les frontières 0/300/530/730/900/990 ms, les deux
états sonores, la priorité à la première touche, les entrées invalides, deux tirs
successifs avec réussite ou échec, l'absence de touche et le retour global sur Alfol.

## Validation du 16 octobre 2026 après l'alerte antivirus

La cible `test_input_c` a été recompilée avec les avertissements traités comme
erreurs, puis exécutée avec succès. Les autres cibles n'ont pas été recompilées
pendant cette reprise. Les commandes exécutées sont :

```powershell
cmake --build reconstruction/build/cmake --target test_input_c
ctest --test-dir reconstruction/build/cmake -R input_gameplay_boundaries --output-on-failure
ctest --test-dir reconstruction/build/cmake -E '^space_module_contracts$' --output-on-failure
```

Résultat : **20 suites exécutées, 20 réussies**, dont les deux campagnes générées,
sans Python. **Une suite exclue**, `space_module_contracts`, car son exécutable
est celui de l'alerte. Il n'a été ni restauré, ni recompilé, ni exécuté. Cette
exclusion n'est pas une validation de cette suite ; les résultats antérieurs
restent distincts. L'analyse de sécurité et ses limites sont consignées dans
`audit_securite_test_space_2026-10-16.md`. Aucune protection n'a été modifiée.

## Ce qui reste dans le contexte 3

- La comparaison globale est maintenant exécutée sur deux parcours jusqu'à la
  victoire : voir `validation_teramain_dos.md`. Les autres graines, difficultés
  et branches non parcourues ne sont pas certifiées exhaustivement.
- Nouvelle validation du test spatial signalé lorsque le fichier exact et la
  chaîne d'outils auront un niveau de confiance établi.

Les parcours complets existants portent sur deux graines, difficulté zéro et
son désactivé. Ils ne prouvent pas toutes les configurations. Graphismes et son
audible restent le contexte 4 ; aucune interface temporaire n'est nécessaire.

La sauvegarde/reprise C est désormais implémentée et testée, avec 34 et 30
reprises dans les deux campagnes complètes : voir `spec_sauvegarde_c.md`.

## Comparaison BIN/C native ajoutée ensuite — 16 octobre 2026

**CONFIRMÉ — BIN/C, cas exécutés** : `tests/test_binary_input_c.c` utilise
l'API C native d'Unicorn, sans interpréteur Python. Le banc facultatif est
indépendant du core et des suites C ordinaires. L'installation locale fournit
une DLL Windows 64 bits ; une configuration séparée utilise GCC UCRT64. Cela
ne restaure ni ne reconstruit l'exécutable spatial de l'alerte antivirus.

Les SHA-256 des originaux sont vérifiés à la configuration CMake :

- TERA.COM : `22fb836aa93f46dc2437e8b9586415d13552bfac35e8ac6647ba6157fd9b1a37`.
- TERA.000 : `539d95781f9296d40ecd05698eb98255835a45cb03e60b49cf581e3ada7038f4`.

Le binaire exécute ses instructions de branchement et son vrai RANDOM. Les
hooks remplacent le clavier et les sorties matérielles connues, avec leurs
nettoyages de pile Pascal ; ils ne substituent aucun résultat aléatoire.

| Région | Cas | Contrôles |
| --- | --- | --- |
| COM 7868..795D, monstre 15 | 70 | Cinq graines, sept points d'interruption dont aucune touche, son 0/1 ; purge d'une touche ancienne, images, bornes et nombre de tirages, graine finale, touche encore présente, retour/pile |
| OVL00AE BF72..poll BAFD | 166 | Cinq graines, son 0/1, événements autour des bornes de délai/300/530/730/900/990 ms ; vrais R10/R3/R3, coordonnées, six délais, boucles de purge et disponibilité de la touche au poll final |

**236 cas exécutés, zéro écart.** Quatre combinaisons donnant un temps négatif
avec un délai initial nul sont exclues des 170 combinaisons spatiales proposées.
Les deux états sonores n'ajoutent pas de tirage dans ces régions ciblées.

Pour le test spatial, la graine au début de BF72 est celle suivant les tirages
de préparation du vol déjà documentés ; les dégâts, explosions, pirates et
l'atterrissage ne sont pas rejoués dans cet oracle ciblé. La comparaison s'arrête
au premier poll final. L'ordre exact d'une touche à la même milliseconde qu'une
purge suit la convention documentée : arrivée dans le tampon avant la purge.
L'ordonnancement d'un clavier réel reste à fournir par SDL.

Exemple local Windows, dépendance native déjà disponible, sans téléchargement :

```powershell
cmake -S reconstruction -B reconstruction/build/native-oracle -G 'MinGW Makefiles' -DCMAKE_C_COMPILER=E:/msys64/ucrt64/bin/gcc.exe -DCMAKE_MAKE_PROGRAM=E:/msys64/ucrt64/bin/mingw32-make.exe -DTERA_UNICORN_ROOT=E:/dev/PYTHON/PROJETS/tera_remake/reconstruction/build/emulator_deps/unicorn
cmake --build reconstruction/build/native-oracle --target test_binary_input_c test_save_c
ctest --test-dir reconstruction/build/native-oracle -R '^(binary_input_boundaries|save_resume_contracts)$' --output-on-failure
```

Les deux suites sélectionnées passent, dont la sauvegarde en 64 bits. Dans la
configuration ordinaire 32 bits, **23 suites exécutées sur 23 passent**, dont
les deux parcours de base et leurs deux variantes avec reprise. La suite
`space_module_contracts` reste exclue. Ces résultats ne lèvent pas la réserve
de sécurité sur son ancien exécutable.
