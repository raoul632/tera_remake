# Frontend SDL3 / C — première boucle autonome

17 octobre 2026. **Combat/temps/audio : 37 suites CTest sur 37 réussies**.
Les sept tables de textes, 82 sprites et six pages CGA sont chargés en C,
la bannière apparaît au démarrage et les phrases/noms originaux sont affichés
avec les accents CP437. Voir [formats, usage et limites](TERA_ASSETS.md).
Création en mode texte 640x200, validation et fiches en 320x200 : nom, sexe,
trois répartitions, six bonus et refus/reprise sont utilisables. H/I conserve
sa fiche jusqu'à la prochaine commande ; H/S affiche chaque compagnon sur les
vraies pauses du core. Équipement, compétences et groupe utilisent des vues
sans mutation. Voir [spécification et limites](spec_character_screens_c.md).
Les dix lieux et les 23 paysages disposent maintenant de compositions SDL ;
couloirs, portes, coffres/pièges, dialogues, recrutements, achats, ventes,
hôpital et interactions Alfol/Meduz sont affichés. Le voyage utilise ASTRON
et les cibles/temps du core. Voir [périmètre et écarts](spec_scene_interactions_c.md).
Le combat terrestre est maintenant présenté avec les commandes originales,
les résultats successifs, les PV/PM/maxima et les sprites confirmés. Une
chronologie restitue les délais et les principaux effets PC speaker via SDL.
L'animation du monstre 15 respecte ses durées et son interruption clavier.
Voir le [comparatif détaillé et les écarts restants](spec_combat_media_sdl_c.md).
Les compléments procéduraux des trois monstres sont intégrés et contrôlés
contre les primitives originales. Les ambiances de marche, sept lieux,
l'entrée souterraine, le dépôt Alfol et les sons du voyage sont raccordés.
Dix-huit partitions concordent avec les routines originales exécutées.
Les sons de progression sont raccordés ; les détails de rendu restent à comparer.
Le [bloc scènes](spec_scene_completion_c.md) ajoute montagnes/lune,
immeubles, tours/portraits, décorations, bureau/déchets, catalyseur, rayon
et palettes par lieu. Les résultats
de 26/29 suites ci-dessous décrivent les jalons précédents.

Bloc suivant — menus : catalogue MESSAGE 72..90 aux positions de l'overlay,
accueil et vente de Kronopolis, verbes de confirmation issus de VERBES,
halte H sur le décor, menu N et bonifications en grille. La vue publique
expose la sous-commande H sans mutation. Le test SDL compare les pixels
du catalogue aux enregistrements DATA et les touches à une session sans rendu.
Voir [audit complet et limites par groupe](spec_menus_sdl_c.md).

**CONFIRMÉ — SOURCE C / tests exécutés** pour les éléments
ci-dessous. Ce jalon valide le travail initial demandé (audit, API, boucle,
commandes, diagnostic). Il ne certifie pas encore le prototype graphique
complet ni son parcours manuel jusqu'à la victoire.

## Architecture et API

`frontend/sdl3/` dépend de `tera_engine` et SDL3 >= 3.2. Aucun header SDL
dans `engine/`. L'option CMake TERA_BUILD_SDL3 est OFF par défaut ; aucune
recherche de SDL ni installation automatique pour compiler le core seul.

- main.c/app.c : fenêtre, renderer 2D, événements, cycle de vie, temps réel
  des tirs via tera_game_space_input_at, fichiers hôtes de sauvegarde.
- input.c : flèches/pavé -> 2468, Entrée/retour arrière/Echap -> octets
  du core ; caractères ASCII via SDL_EVENT_TEXT_INPUT (layout AZERTY compris).
  Le pavé avec Verr Num utilise la saisie texte, sans double commande.
- assets.c : lecture C autonome et validée de NOMPERS/VALMON, puis génération
  par world.h et création par game.h. Aucun profil/carte de remplacement.
- resources.c/encoding.c : formats originaux et conversion CP437 sans SDL.
- text.c/sprites.c : tables IBM BIOS/CGA CP437 et textures SDL3
  avec pixels indexés conservés ; BANNIERE affichée au démarrage.
- characters.c / battle.c / renderer.c : création, fiches, scènes et combat
  en 320 x 200 logique, letterboxing, noms/phrases
  originaux. Pas de décor inventé ni d'affectation spéculative de monstre.
- game_view.c (core) : sélection en lecture seule de l'état et du combat ou
  de la création réellement suspendus, y compris les combats imbriqués.
  Les pointeurs empruntés expirent à la mutation suivante.

Le routage est `SDL -> tera_game_key -> core -> tera_game_view /
TeraPresentationSink -> rendu`. Pas de formule de combat, obstacle, RNG, prix,
victoire ou progression dans SDL. Le flux général relaie aussi les événements
de combat existants.
Le [contrat de présentation](spec_presentation_c.md) expose désormais les
résultats transitoires et la saisie attendue, sans règles dans SDL.

Mise à jour du premier gros bloc : **29 suites CTest sur 29 réussies**, dont
deux campagnes complètes avec observation et reprise. Le résultat ci-dessous
de 26 suites conserve la preuve du premier jalon SDL. Les résultats courants
et les limites de couverture sont détaillés dans spec_presentation_c.md.

## Compilation locale Windows

Depuis la racine historique, avec l'installation SDL3 UCRT64 disponible ici :

```powershell
cmake -S reconstruction -B reconstruction/build/sdl3 -G 'MinGW Makefiles' -DCMAKE_C_COMPILER=E:/msys64/ucrt64/bin/gcc.exe -DCMAKE_MAKE_PROGRAM=E:/msys64/ucrt64/bin/mingw32-make.exe -DCMAKE_PREFIX_PATH=E:/msys64/ucrt64 -DTERA_BUILD_SDL3=ON
cmake --build reconstruction/build/sdl3 -j 4
ctest --test-dir reconstruction/build/sdl3 --output-on-failure
./reconstruction/build/sdl3/tera_sdl3.exe --data './data/compiled program'
```

Sur une autre machine, fournir SDL3 par son package CMake (`SDL3_DIR` ou
CMAKE_PREFIX_PATH), avec la même architecture que le compilateur. La cible
Windows copie SDL3.dll à côté de l'exécutable. Aucun téléchargement implicite.
Le frontend est uniquement C ; ni SDL2, ni wrapper, ni OpenGL/Vulkan direct.

Par défaut les données sont cherchées dans `data/` **à côté de l'exécutable**,
indépendamment du dossier courant. `--data DIR` permet d'utiliser les originaux
à leur place. `--seed N` (défaut reproductible 1986) et `--difficulty 0..9`
(défaut 0) paramètrent uniquement les APIs existantes de génération.
`--load FILE` reprend une sauvegarde sans régénération. Depuis le bloc
ressources, le paquet de présentation original reste nécessaire à l'affichage.

## Commandes et preuve initiale

La création suit les lectures du core : nom/Entrée, H ou F, trois répartitions
numériques, six bonus F/I/N/E/V/T/A/C/H, puis N pour refaire ou une autre
touche pour garder. La ligne des caractéristiques affiche les valeurs du core.
Exemple reproductible utilisé par le test : `TERA<Entrée>H333VVTTAAO`.
Les flèches représentent 8/2/4/6 ; le sens absolu/relatif est résolu par le core
selon le lieu. Toutes les lettres sont transmises sans filtrage de gameplay.
Les purges de lecture sont demandées par le core au service d'entrée SDL.
Les collages de plusieurs caractères et textes hors ASCII ne sont pas transmis.

F5 sauvegarde, F9 charge, seulement à READY selon le contrat save.h.
`--save FILE` choisit le chemin (défaut `tera.sav` dans le dossier courant).
Écriture dans un temporaire voisin, flush et fermeture contrôlés, renommage
par SDL_RenamePath uniquement au succès. Pas de format propre au frontend ;
la garantie de synchronisation physique en cas de panne reste celle de l'OS.
F11 bascule le plein écran. La fermeture de fenêtre quitte l'application ;
la commande de sortie du jeu reste H/Echap/confirmation, résolue par le core.

## Validation exécutée

- GCC UCRT64 13.2, SDL 3.2.8 : compilation avec -Wall -Wextra -Werror -pedantic.
- **26 suites sur 26 réussies**, aucune exclue : deux graines de campagne,
  avec/sans reprise, contrats existants, nouvelle vue publique et test SDL.
- `game_view_read_only` : état courant/imbriqué, création/succession,
  transfert/inventaire/coffre, immutabilité du jeu et du RNG.
- `sdl3_runtime_autonomy` : copie locale limitée à exe, SDL3.dll, NOMPERS,
  VALMON ; PATH limité à Windows/System32, PYTHONHOME/PYTHONPATH supprimés ;
  pilote vidéo dummy et rendu software. Création par événements texte/Entrée,
  flèche droite, état sérialisé strictement identique aux commandes C directes,
  sauvegarde/reprise identique, rendu et capture BMP. Pas de COM ni overlay.
- Même smoke exécuté avec la vraie fenêtre Windows et capture inspectée :
  texte lisible, mise à l'échelle 1280 x 800, aucun chevauchement constaté.
- Configuration distincte `build/core-autonomy`, TERA_BUILD_SDL3=OFF : core
  compilé et contrat de vue passé sans dépendance SDL ni Python.
- Imports PE de l'exécutable : SDL3.dll et DLL système Windows/UCRT seulement.
  SDL3.dll locale n'importe que des DLL Windows système.

Le paquet **local uniquement** de preuve est `build/sdl3/runtime-only/`.
Ses données originales ne sont pas redistribuées. Le script CMake de smoke
est un test de développement ; il n'est jamais nécessaire pour jouer.

## Écarts et suite, dans l'ordre demandé

**Intégré — présentation** : cartes/aide, textes de Seth, fins, mélodies
finales et tunnels Meduz. Le [profil IBM PC/XT](spec_visual_xt_c.md) corrige
la police CP437, le rapport des pixels en mode texte, les marqueurs de carte
et la cadence des tunnels à 4,77 MHz. Les 44 cartes et 245 images de tunnel
des fixtures correspondent au raster CGA original exécuté.
Les sons de progression sont désormais raccordés ; voir
[leur comparaison native](spec_progression_sound_c.md). Le
[comparatif final](spec_final_comparison_c.md) ajoute 458 références de pixels
exécutées dans GRAPH et les blitters originaux, testées en 320x200 et avec
agrandissement/letterbox. Le profil temporel XT reste un modèle nominal.
Montagnes, tours/portraits, décors aléatoires et palettes par lieu sont
maintenant intégrés. Le combat et les principaux sons sont présentés.
Les IMA et tables MESSAGE*/NOMON/OBJETS/VERBES sont maintenant chargés ;
CP437 est converti sans perte et les lettres françaises sont affichées.
Les deux tables IBM de référence sont documentées. Les écrans terminaux et erreurs
techniques conservent un diagnostic explicite.
Le groupe utilise désormais la sélection A-D du core, y compris les compagnons
aux indices non consécutifs et les personnages HS. Le texte n'utilise plus
la conversion partielle Latin-1 ni le glyphe de remplacement.

**À VÉRIFIER — entrées réelles** : le mapping des tirs utilise le vrai temps
et les échéances du core. Le nouveau test SDL exerce carburant et cinq images
de tir au temps virtuel ; la réactivité d'un vol humain reste à vérifier.
Les tests C spatiaux passent. Le monstre 15 garde son appel synchrone dans
le core, mais SDL présente désormais chaque image/durée avant son poll ;
l'interruption de la première image est couverte par le smoke réel.

Le rendu s'appuie maintenant sur les formats et coordonnées documentés,
ressource par ressource. Les anciens calculs de dessin
Python ne suffisent pas comme preuve. Les captures listées dans le comparatif
final correspondent au raster DOS exécuté, dans leur périmètre précis.
Les [campagnes SDL de bout en bout](spec_sdl_campaign_validation.md) traversent
désormais le vrai mapping, le rendu et F5/F9 sur deux graines : 7143 entrées,
3988 états READY identiques, 42 sauvegardes/restaurations et deux victoires.
Les 39 suites CTest passent. Ces parcours sont automatisés, avec attentes
accélérées et pilotes dummy ; une appréciation humaine sur écran et audio
réels reste distincte de cette preuve d'intégration.

Références SDL3 consultées :
[résolution logique](https://wiki.libsdl.org/SDL3/SDL_SetRenderLogicalPresentation),
[texte diagnostic](https://wiki.libsdl.org/SDL3/SDL_RenderDebugText),
[API SDL3](https://wiki.libsdl.org/SDL3/QuickReference), complétées par les
headers SDL3 3.2.8 locaux (événements, texte, lecture de pixels, renommage).
