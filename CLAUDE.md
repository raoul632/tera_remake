# CLAUDE.md — Projet TERA Remake

## Contexte

Ce projet reconstruit **TERA** (LORICIELS, octobre 1986, RPG français
inspiré de Fighting Fantasy/B-X), un jeu que Merci a retrouvé sur un
forum abandonware il y a 15 ans sous forme de code source Pascal
décompilé, complété par les fichiers de données originaux et le binaire
`TERA.COM`. Le moteur a été intégralement analysé par reverse engineering
(overlays Pascal reconstruits + désassemblage x86 du binaire pour les
parties manquantes comme le combat).

**Lire en premier, dans l'ordre :**
1. `docs/TERA_notes_moteur.md` — tout ce qu'on sait du jeu original,
   confirmé par lecture de code source ET/OU désassemblage. C'est la
   référence de vérité pour toute mécanique de jeu.
2. `docs/TERA_remake_blueprint.md` — l'architecture cible et les choix
   de conception pour le remake.
3. `manuel/Tera.pdf` — le manuel papier officiel (scanné, sans texte —
   voir section 18 de `TERA_notes_moteur.md` pour ce qui en a été
   extrait). **Source de bien meilleure confiance que le reverse
   engineering** pour les mécaniques telles que vécues par le joueur —
   en cas de conflit avec une hypothèse de reverse engineering, le
   manuel gagne.

## État d'avancement

- Le squelette Python (`src/tera/`) contient : modèles de données
  (`models.py`), génération procédurale de donjon (`worldgen.py`,
  creusement + contenu + **marqueurs uniques par étage et
  auto-validation portés**, voir note ci-dessous), résolution de combat
  (`combat.py`, formule complète), chargeur de données de jeu
  (`data_loader.py`, données réelles extraites dans `data/game_data.json`),
  **sauvegarde/chargement JSON** (`save.py`), **boucle de jeu textuelle**
  (`game.py`, `python -m tera.game`) et un **premier rendu Pygame**
  (`render/`, `python -m tera.render.app` — voir "Commandes utiles").
- Toutes les étapes du plan de développement suggéré par le blueprint
  (section 7) ont maintenant un premier jet : moteur de données → boucle
  textuelle → rendu 2D (vue du dessus + couloir 1ère personne).
- **Amarande, Alfol (surface + base) et Meduz sont maintenant codés et
  jouables** (`src/tera/amarande.py` pour Amarande, `worldgen.py` pour
  Alfol-base/Meduz — voir `docs/TERA_notes_moteur.md` sections 19-20
  pour tout le détail, trouvé en lisant `OVL_00FB`/`OVL_0085`/`OVL_00A4`/
  `OVL_0010`, déjà décompilés, pas de nouveau désassemblage nécessaire) :
  - Amarande : grille 30×30 (positions 1000-1899), montagnes-lignes
    (une chaîne H de 9 cases, cinq chaînes V de 5 cases), 20 villes,
    algorithme de génération et de rebouclage porté tel quel (y compris
    son comportement non-torique surprenant, voir docstring
    `amarande.py`).
  - Alfol : surface 10×10 sans mur (simple arithmétique modulo) + base
    intérieure en couloir (monde 9, marqueurs Bureau des Médias/dépôt
    radioactif).
  - Meduz : couloir (monde 10), marqueurs méditer/invoquer.
  - **Les 20 villes (`save.loc_towns`) sont maintenant visitables**
    ("entrer" en étant sur la case) et ouvrent une **boutique générique**
    (`SHOP_ITEMS`/`describe_shop`/`handle_shop_command` dans `game.py`,
    `render/scene.py::draw_shop_scene` côté Pygame) : achat, vente et
    hôpital, tous les trois **désormais fidèles** après relecture
    complète d'`OVL_00BC.PAS` (voir `mapping_fonctions_pascal_python.md`
    pour le détail fonction par fonction). Trois écarts trouvés et
    corrigés dans cette passe :
    - **Formule d'achat corrigée** : `prix*30/(30+jet/5)` où `jet` est
      `C_8315(perso,8,8)` (`combat.py::negotiation_roll` — le même jet de
      base que le combat, appliqué au Charme) et PAS la stat Charme brute
      comme le supposait une hypothèse précédente.
    - **Vraie table de vente portée** (`game.py::SELL_PRICES`, 15 objets,
      confirmée par `__ComputeSellingPrice`) avec la formule réelle
      `prix_rachat + (prix_rachat DIV 100) * jet` — remplace l'ancienne
      approximation (`base_price//2 + charme//5`).
    - **Hôpital corrigé** : coût = `(PV manquants du personnage le PLUS
      blessé) + 1` semaines pour soigner TOUT le groupe d'un coup (PV et
      états) — pas la somme des PV manquants de chaque personnage.
    - **Clé n°9 corrigée** : `save.keys[9] = 9` (valeur fixe confirmée,
      `Sav.f_Keys[9] := 9`) au lieu d'un incrément.
    **Simplification toujours assumée** : les 20 villes partagent
    toutes cette même boutique — le dispatcher d'entrée en ville exact
    par ville (`C_7309`, index >9) n'a jamais été décodé (section
    19.1). Objets "spéciaux" de l'original (énergie, cartes de niveau)
    restent simplifiés en simples entrées d'équipement, faute des
    sous-systèmes correspondants (minijeu spatial section 8, cartes)
    dans ce port.
  - **Tâches 1, 2 et 3 jouables jusqu'au bout** (méditer+invoquer sur
    Meduz / payer 1000 ducats à Alfol / vaincre Arioch). **Tâche 4
    (Temple de Seth, Autel des Ténèbres) reste non codée** — aucun
    générateur écrit pour ce lieu cette session, seule tâche encore
    bloquante pour une victoire complète.
  - **Simplification assumée** : les points de passage Amarande↔Cité
    des Crânes/Pyramide/Alfol sont des **positions fixes** choisies pour
    ce port (`AMARANDE_CITE_ENTRANCE` etc., définies dans `amarande.py`
    et réutilisées par `game.py` et `render/`), pas les positions
    aléatoires (`Sav.f_LocExtra`) de l'original, qui dépendent d'un
    système de recrutement de 9 PNJ non implémenté (section 20.3).
  - **Deux positions non confirmées** : le marqueur "BUREAU DES MEDIAS"
    (Alfol) et les marqueurs méditer/invoquer (Meduz) sont placés sur
    une case atteignable choisie au hasard — l'algorithme exact de
    placement de l'original pour ces marqueurs précis n'a pas été
    retrouvé dans `OVL_0010.PAS` (voir section 20.6), probablement dans
    la passe générique `INNER_BD96` de `OVL_0000`, non vérifiée pour les
    mondes 9/10.
- **Manque connu dans la boucle de combat de `game.py`** : aucune
  formule d'attaque de monstre n'a été retrouvée dans le désassemblage
  (voir section 17 des notes moteur — seules les attaques du groupe
  contre le monstre ont été confirmées). `_monster_turn()` dans
  `game.py` est donc un mécanisme **provisoire, explicitement non
  fidèle**, juste pour que le combat soit jouable — à ne pas prendre
  comme référence si la vraie formule est un jour retrouvée.
- **Le rendu Pygame couvre maintenant tous les lieux jouables**
  (`render/app.py` + `render/scene.py` pour Amarande/surface d'Alfol,
  `corridor.py` pour les donjons — `topdown.py` gardé comme carte de
  secours secondaire, touche `T`). **Le jeu se joue "image par image",
  jamais en carte vue du dessus** — confirmé en relisant `OVL_00FB.PAS`/
  `OVL_0085.PAS` (section 5.1/19 des notes moteur) : l'original affiche
  une vraie scène (décor procédural, silhouette de montagne si une
  chaîne est adjacente) à chaque case, jamais un plan d'ensemble avec un
  point de position — `render/scene.py` reproduit ce principe avec un
  décor procédural **par seed de position** (modernisation du
  modulo-23 d'origine, déjà prévue par le blueprint section 6) plutôt
  que la grille abstraite utilisée dans une première version de ce
  rendu (`overworld.py`, supprimé). **Aucune logique dupliquée** :
  `render/app.py` appelle directement les mêmes fonctions que `game.py`
  (`resolve_chest`, `run_combat`, `try_stairs_up`, `handle_amarande_command`,
  etc. — factorisées hors de `main()` pour cette raison), donc les deux
  interfaces restent forcément synchronisées.
  **Tout se passe dans la fenêtre Pygame, y compris le combat/les mots
  secrets/le paiement** — confirmé par le manuel officiel (section 18
  des notes moteur : *"aucun texte n'est affiché en permanence, à
  l'exception de la ligne inférieure de commande"*, l'original n'a
  **jamais** de console séparée). `game.py` accepte des callbacks
  `ask`/`log` injectables (par défaut `input`/`print` pour le CLI,
  définis en tête de fichier) ; `render/app.py` leur substitue
  `textinput.pygame_ask` (ligne de saisie superposée à l'écran de jeu,
  sans jamais ouvrir de fenêtre à part) et un petit journal de
  4 lignes. Contrôle : `Échap` annule une saisie en cours.
- **Les vrais sprites extraits (`data/sprites_extracted/crapim/`) sont
  maintenant utilisés dans le rendu**, via `render/sprites.py`
  (`load_sprite`/`load_scaled`, mise en cache).
  - `render/corridor.py` dessine le sprite de porte confirmé
    (`SPR_2D`/`SPR_DOOR`) sur le mur de face d'un plan quand le
    couloir continue à travers une porte (`WALL_DOOR`), conformément à
    la note confirmée section 5.2 des notes moteur ("un mur 'face'
    dessine aussi une porte"). Les murs pleins (fin de couloir,
    `WALL_NONE`) restent un simple rectangle vectoriel.
  - **Amarande est maintenant un port fidele des 23 scenes fixes de
    l'original** (`Sav.f_Position MOD 23`), pas une modernisation
    aleatoire : suite a une capture d'ecran du vrai `TERA.COM` fournie
    par Merci, la fin d'`OVL_00FB.PAS` a ete relue en detail et revele
    une table figee (coordonnees + sprites codes en dur), portee telle
    quelle dans `render/amarande_scenes.py` (table `SCENES`, 23 entrees)
    et interpretee par `render/scene.py::draw_amarande_scene` (section 21
    des notes moteur pour le detail complet). Inclut la vraie ligne
    d'horizon en polyligne magenta + etoiles (`INNER_BD0C`) et le decor
    de sol disperse (cailloux/touffes, `INNER_C0EC`).
  - **Correction importante** : `SPR_44`/`SPR_45` avaient ete identifies
    comme des rochers dans une session precedente (inspection visuelle
    rapide) -- en recoupant avec l'usage reel dans `OVL_00FB.PAS` et un
    zoom pixel par pixel, ce sont en realite des **arbres**. Les vrais
    rochers/silhouettes de montagne sont `SPR_2A`/`SPR_2B`/`SPR_2C`/
    `SPR_39`/`SPR_3A`/`SPR_4F`/`SPR_50`/`SPR_52`. Table complete
    (role confirme par sprite) dans `render/sprites.py` et section 21.1
    des notes moteur.
  - **La surface d'Alfol reste sur l'ancienne modernisation** (decor
    pseudo-aleatoire seede par position, `OVL_0085.PAS` pas depouille au
    meme niveau de detail) -- seul Amarande a eu cette passe de
    fidelisation.
  - **Zoom supplementaire (`scene.py::SCENE_ZOOM = 1.3`)** : retour de
    Merci sur la premiere version de ce port ("les sprites sont trop
    petits/perdus dans l'ecran") -- les sprites originaux sont minuscules
    en resolution CGA native, et la fenetre du remake est bien plus
    grande que 320x200. Le zoom est ancre en BAS (`_origin_scale`
    calcule un `y_offset` negatif) pour agrandir le premier plan
    (arbres/rochers, concentres pres du sol dans la table des 23 scenes)
    plutot que le ciel -- au prix de rogner une partie des etoiles les
    plus hautes. Purement une decision de rendu moderne (fenetre plus
    grande que l'original), ne change aucune coordonnee/donnee portee.
  - **Vraie cause du decor "packe en bas a gauche" trouvee (retour de
    Merci, apres qu'un premier correctif d'ancrage se soit revele
    insuffisant -- "il faut aller revoir dans le code source comment il
    procede")** : en comparant TOUS les appels `PutSprite(_off)` de tous
    les overlays decompiles (pas seulement `OVL_00FB.PAS`), l'argument X
    ne depasse jamais ~80 nulle part dans le code source, alors que Y va
    normalement jusqu'a ~190 -- et `FillRect_off` utilise meme la
    constante litterale `80` comme "bord droit de l'ecran". Sachant que
    les sprites `CRAPIM.IMA` stockent deja leur largeur EN OCTETS (4
    pixels CGA par octet, section 10), la conclusion s'impose : **la
    coordonnee X de `PutSprite_off`/`FillRect_off` est en OCTETS, pas en
    pixels** (320px / 4 = 80 octets par ligne, d'ou le plafond ~80 partout)
    -- `DRAW`/`PLOT` (ligne d'horizon/etoiles) restent eux de vraies
    coordonnees pixel (vont jusqu'a 319), une convention differente pour
    une primitive differente. La premiere version de ce port traitait X
    comme un pixel brut, ce qui compressait tout le decor (dont les
    coordonnees vont bien jusqu'a ~76-80) dans le quart gauche de l'ecran
    au lieu de l'etaler sur (presque) toute la largeur -- confirme
    visuellement apres correction (`scene.py::SPRITE_X_UNIT = 4`,
    multiplie X avant mise a l'echelle dans `_put_sprite`/`_fill_rect`) :
    le resultat correspond maintenant nettement mieux a la capture
    d'ecran de reference (arbres/rochers repartis sur toute la largeur,
    pas juste dans un coin).
  - **Sprites de monstres/coffres en donjon** (retour de Merci : "on ne
    voit pas leur sprites" en rencontrant des monstres) :
    - **Indicateur de coffre, CONFIRME** (`OVL_002E.PAS`, monde 0,
      `INNER_C5FC`) : `SPR_05` si un monstre d'id 3/6/9 est associe au
      coffre, `SPR_3C` sinon -- porte dans `corridor.py::draw_chest_marker`,
      affiche au premier plan de la vue couloir des qu'un coffre non
      ouvert est present (hors escaliers/autel, qui partagent le meme
      champ `cell.chest` sans etre des coffres).
    - **Sprite de monstre en combat, NON CONFIRME** : `SpawnMonster` (la
      routine qui affiche reellement le monstre dans l'original) n'a
      jamais ete desassemblee (section 15/17 des notes moteur) -- aucune
      table monstre->sprite ne peut donc etre reconstruite fidelement.
      `sprites.py::monster_sprite_id` choisit un sprite PUREMENT
      DECORATIF par ressemblance visuelle avec le nom du monstre
      (`NOMON`) -- cobra/naja -> capuchon de cobra confirme au zoom,
      chauve-souris -> silhouette ailee, prince-demon/seigneur du CHAOS
      -> figure cornue, esprits/necromanciens -> silhouette encapuchonnee,
      guerriers/gardes/cyborg/homme -> humanoide arme, creature/caiman/
      crocodile/demi-dieu -> silhouette massive, repli generique pour le
      reste (cafards/mygales/tarentule/insectes/cristaux, sans equivalent
      visuel clair dans les 82 sprites). Objectif assume : donner quelque
      chose a voir plutot que rien, PAS reproduire l'original.
      `game.py::run_combat`/`handle_encounter`/`try_move_dungeon` ont un
      nouveau parametre optionnel `on_monster` (callback appele une fois
      avec le `Monster` des le debut du combat, no-op par defaut/CLI) --
      `render/app.py::GameView.on_monster` l'utilise pour dessiner le
      sprite + nom sur `VIEW_AREA` et flip immediatement l'ecran, visible
      pendant toute la boucle de combat textuelle qui suit (la ligne de
      saisie ne redessine que sa propre barre, jamais `VIEW_AREA`).
    - **Bug decouvert au passage (sans rapport avec le rendu)** : les
      noms de monstres contiennent de vrais accents UTF-8 (`"un
      crocodile géant"`) -- une recherche de sous-chaine sur un mot-cle
      SANS accent (`"demon"`) ne matche donc jamais `"prince-démon"`.
      Pas un bug de `game_data.json` (verifie octet par octet, l'UTF-8
      y est correct) : juste un artefact d'affichage de ce terminal qui
      ne rend pas les caracteres accentues. Mots-cles choisis pour rester
      du cote non-accentue du mot (`"prince"`, `"cromancien"`, `"ature"`).
  - **Couloirs remis en filigrane magenta (retour de Merci : "reste sur
    de la fausse 3D en filigrame")** : en re-analysant `OVL_002E.PAS`
    ("WORLD 0 - cite des cranes 1", deja lu pour les sprites de
    monstres/coffres), confirmation que l'original ne remplit JAMAIS ses
    murs de couloir -- `INNER_BFCB`/`INNER_C211`/`INNER_C1A4`/
    `INNER_BB70`/`INNER_BBB8` dessinent uniquement des lignes
    (`DRAW(...,2)`, couleur CGA 2 = magenta) formant des rectangles
    emboites vers un point de fuite, sur fond noir uni (pas de sol/
    plafond colore). `corridor.py::draw_corridor` remplissait au
    contraire des polygones pleins en gris -- corrige : contours seuls
    (`pygame.draw.polygon/rect(..., 1)`), couleur magenta unique
    (`WALL_COLOR = SIDE_WALL_COLOR = (255,85,255)`), fond noir, et le
    cadre de chaque profondeur est maintenant dessine meme quand le
    couloir continue (pas seulement sur un mur plein) pour obtenir
    l'effet de tunnel emboite. Le PRINCIPE geometrique (plans generiques
    via `compute_planes`, pas les coordonnees figees d'un seul monde)
    reste une modernisation assumee -- seul le STYLE (filigrane, pas de
    remplissage) est maintenant fidele.
  - **Porte trop grande corrigee juste apres (retour de Merci : "un fond
    bleu et blanc un peu etrange" en se deplacant)** : `SPR_DOOR` est un
    sprite minuscule et large (8x6px natif) -- le mettre a l'echelle de
    TOUTE la hauteur du mur de face (comme avant) le transformait en gros
    aplat cyan/blanc/magenta qui recouvrait une grande partie du mur et
    ressemblait a un fond parasite plutot qu'a une porte, surtout visible
    sur le nouveau fond noir uni. Corrige en le limitant a 45% de la
    hauteur du mur (`corridor.py`) : il se lit maintenant comme un accent
    de porte avec le filigrane magenta visible tout autour, au lieu de
    dominer l'ecran.
  - **Reste non confirme** : `DrawMountains` (silhouette "proche" quand
    une vraie chaine de montagnes est adjacente) est une primitive
    residente jamais retrouvee (section 15) -- remplacee par une
    silhouette vectorielle approximative, inchangee. L'ancre exacte de
    `PutSprite_off(x,y,sprite)` (coin haut-gauche suppose) n'est pas non
    plus confirmee par desassemblage.
  - **Marqueur de ville (`SPR_VILLAGE = 0x42`)** : identification au
    mieux, PAS confirmee par reference textuelle (contrairement au reste
    de la table -- aucun sprite de ville n'est cite dans `OVL_00FB.PAS`,
    les 20 villes passent par une gestion non decodee, section 19.1/21.3).
    Remplace l'ancien placeholder vectoriel (rectangle + toit triangle)
    dans `render/scene.py::_draw_landmark`, avec repli sur ce vectoriel
    si le PNG est absent.
  - **Indications textuelles sur ce qu'il y a a voir/faire** : le rendu
    Pygame n'affichait auparavant AUCUN texte au sujet du contenu de la
    case actuelle (contrairement au CLI, qui decrit toujours la case a
    chaque tour) -- seul le decor visuel etait present, sans indication
    du genre "une ville est visible ici" ou "un escalier descend ici".
    Corrige : `game.py::amarande_here_lines`/`dungeon_here_lines`
    (factorisees depuis `describe_amarande`/`describe_cell`, reutilisees
    telles quelles par le CLI) sont maintenant aussi appelees par
    `render/app.py` (`handle_amarande_key`/`handle_dungeon_key`) et
    loggees dans le journal du bas des qu'on arrive sur une case notable
    (ville, Cite/Pyramide/Alfol, coffre, escalier, autel, Arioch...).
    Volontairement SANS repeter les passages/murs (deja visibles dans la
    vue couloir), contrairement au CLI qui doit tout dire en texte.
- **Écran de création de personnage fidèle, en mode texte**
  (`render/chargen.py`) : contrairement au reste du rendu (scènes/
  couloirs en formes vectorielles/sprites plein écran), `OVL_0022.PAS`
  affiche cet écran en **mode texte** — fenêtre bordée, fond bleu CGA,
  texte jaune (questions)/blanc (messages)/rouge clair (titre). Le
  module ne duplique **aucune logique** : `run_chargen()` fournit juste
  des callbacks `ask`/`log` qui dessinent dans cette fenêtre bordée puis
  délègue tout le déroulé (formule des stats, répartition interactive,
  reroll "Gardes-tu ce personnage ?") à `game.py::create_party`, déjà
  utilisée telle quelle par le CLI. Branché dans `render/app.py::build_game`,
  appelé avant la génération du monde (même ordre que l'original).
  - **Bug corrigé au passage, latent depuis la première version de
    `textinput.pygame_ask`** : la boucle de saisie appelait
    `pygame.event.get()` (qui vide toute la file d'un coup) puis
    retournait dès qu'elle trouvait un `K_RETURN`/`K_ESCAPE` — si
    plusieurs touches arrivaient dans la même frame (typage rapide, ou
    un script poussant plusieurs événements d'un coup, ce qui a permis
    de découvrir le bug lors des tests automatisés de `chargen.py`),
    les événements du même lot situés après ce `K_RETURN` étaient
    perdus silencieusement (déjà retirés de la file par ce `get()`,
    jamais traités). Corrigé dans `chargen.py` **et** `textinput.py` en
    remettant en file (`pygame.event.post`) les événements non
    consommés avant de retourner.
- Le manuel officiel a corrigé deux points du code déjà écrit (voir
  section 18 des notes moteur pour le détail) : la formule de
  désamorçage de piège dans `game.py` (Agilité+Habileté, pas Force), et
  surtout un vrai bug — `game.py` laissait le joueur traverser une case
  à monstre sans jamais déclencher de combat ; corrigé
  (`handle_encounter()`, déclenchée automatiquement à l'entrée d'une
  case avec rencontre). Les contrôles du rendu couloir (`render/app.py`)
  ont aussi été corrigés pour suivre le schéma confirmé par le manuel
  (avancer/reculer/pivoter, pas de déplacement absolu comme sur la
  grande carte).
- `worldgen.py` place désormais clé/étage, autel de soin, escalier
  montant (mondes 2..dernier), escalier descendant (mondes 0..avant-
  dernier) et Arioch (toujours au dernier étage généré), avec une
  boucle de régénération tant que la case de sortie de l'étage 0
  (case 99) n'est pas atteignable — fidèle au principe REPEAT...UNTIL
  de l'original (section 5.3). **Point non confirmé** : la position
  exacte choisie par l'original pour l'escalier descendant dans chaque
  étage n'a pas pu être retrouvée (`TERAUTIL.INC` manquant, voir
  section 15) — le port choisit une case atteignable au hasard, ce qui
  respecte l'invariant documenté ("un escalier par étage") sans
  prétendre reproduire l'algorithme exact de sélection.
- Les seuils numériques du combat (44, 35, divisions par 5/6/7) sont
  fidèles à l'original — **ne pas les changer sans que Merci le
  demande explicitement**, ils encodent l'équilibrage testé en 1986.
- `TERAUTIL.INC`/`KBD.INC` ont été partiellement reconstruits par
  désassemblage direct de `TERA.COM` (`data/source code/TERAUTIL.INC`,
  `data/source code/KBD.INC` — voir `docs/TERA_notes_moteur.md` section
  17). Le cœur du combat est reconfirmé, plus une fonction non
  documentée jusque-là (`C_842A`) qui **contredit** l'ancienne
  hypothèse sur la façon dont le bonus d'équipement se combine au jet
  de dégâts (multiplicatif après coup, pas additif avant le jet — et la
  formule exacte varie par action). **Corrigé dans `combat.py`** (sur
  demande explicite de Merci) : `resolve_action()` porte les 9 branches
  confirmées ; `Character.skills` est maintenant indexé par nom de stat
  (pas par nom de verbe) ; `game.py` demande l'objet utilisé pour les
  actions à objet dynamique (`attaque_mentale`, `dissiper`, `invoquer`,
  `psalmodier`) et applique automatiquement l'objet fixe requis pour les
  autres (`lutter`→épée runique, `tirer`→désintégrateur) — sans l'objet
  requis, `tirer` échoue maintenant toujours (bonus "dur"), alors que
  `lutter` reste possible sans objet (bonus "souple").
- **Sprites extraits** en PNG dans `data/sprites_extracted/` — voir la
  section dédiée "Sprites et images (.IMA)" ci-dessous pour le détail
  complet (format, algorithme, où chaque fichier est sorti).

## Données textuelles (`VERBES`/`OBJETS`/`NOMON`/`NOMPERS`/`VALMON`/`MESSAGE*`)

- **Sources** : `data/compiled program/{VERBES,OBJETS,NOMON,NOMPERS,VALMON,MESSAGE,MESSAGE2,MESSAGE3}`
  — fichiers Pascal `FILE OF STRING[n]` (ou d'entiers pour `VALMON`),
  format confirmé section 12 des notes moteur : enregistrements à
  **taille fixe**, chacun = 1 octet de longueur + N octets de contenu
  (encodage CP437) + padding zéro. `VALMON` est différent : 6 entiers 16
  bits signés par enregistrement (12 octets), pas une chaîne.
- **Script d'extraction** : `tools/extract_text_data.py` (aucune
  dépendance externe, marche avec `python3`). Régénère entièrement
  `data/game_data.json` — relancer avec
  `python3 tools/extract_text_data.py` si un fichier source change ou
  si un doute apparaît sur le contenu actuel. Ce script n'existait pas
  avant le 2026-07-26 (le premier `game_data.json` avait été fait à la
  main dans une session antérieure) ; il documente maintenant
  précisément comment chaque champ est dérivé (taille d'enregistrement
  = taille fichier ÷ nombre d'entrées connu, vérifié exact pour chacun).
- **Contenu** (déjà catalogué en entier dans l'Annexe de
  `docs/TERA_notes_moteur.md`) : `VERBES` (56 verbes/mots-clés),
  `OBJETS` (25 objets), `NOMON` (22 noms de monstres), `NOMPERS` (27 PNJ
  recrutables), `VALMON` (statistiques des 22 monstres, → clé
  `monsters` dans `game_data.json`).
- **`MESSAGE`/`MESSAGE2`/`MESSAGE3`** (102/33/33 entrées) : **nouvellement
  extraits en entier** (avant le 2026-07-26, seuls les index 26-33
  — les 3 variantes de texte des 8 verbes de combat — avaient été cités
  ponctuellement, section 12.1). Voir section 12.4 des notes moteur pour
  le détail complet ; points marquants trouvés en lisant l'extraction
  complète : la liste de prix intégrale de la boutique de Kronopolis
  (lettres a-s), 5 noms de lieux d'Amarande jamais documentés avant
  ("Sanctuaire de MAHALORE", "Domaine de Balgence", "Ermitage de la
  Roche aux Goules", "Hameau de la Porte des sables", "Abbaye de
  Mortefontaine" — aucun overlay ne leur est encore associé), et la
  confirmation textuelle du groupe limité à 4 (*"vous êtes déjà 4 !"*).
  Exposés via `data_loader.get_messages(variant=1|2|3)`.

## Sprites et images (`.IMA`)

Détail complet de l'extraction faite le 2026-07-26, pour ne pas avoir à
la refaire à l'aveugle si le besoin revient (rendu avec sprites,
vérification d'un sprite précis, etc.).

**Contexte historique** : `.IMA` n'est pas un format standard de
l'époque — en 1986, il n'existait quasiment aucun format d'image
normalisé pour DOS (PCX débute tout juste, GIF/BMP n'existent pas
encore). Chaque studio inventait sa propre convention. LORICIELS a
choisi deux approches différentes **sous la même extension** dans TERA :
un dump brut de la mémoire vidéo CGA (les 6 fonds d'écran, blocs
contigus, format trivial à charger avec un simple `memcpy`) et un atlas
compact maison avec en-tête par sprite (`CRAPIM.IMA`, entrelacement
octet par octet) — deux formats internes différents, même extension
`.IMA`, aucun standard suivi dans les deux cas.

### Où sont les fichiers

- **Sources** (binaires d'origine, ne pas modifier) :
  `data/compiled program/CRAPIM.IMA` (l'atlas des 82 sprites de jeu) et
  6 fichiers plein écran : `ASTRON.IMA`, `BANNIERE.IMA`, `FACADE.IMA`,
  `KRONO.IMA`, `MAISONJ.IMA`, `MAISONS.IMA`.
- **Script d'extraction** : `tools/extract_sprites.py` (nécessite
  Pillow → interpréteur `python`, pas `python3`, voir plus bas). Relance
  avec `python tools/extract_sprites.py` depuis la racine du projet.
- **Sorties** (regénérées à chaque exécution du script, jetables) :
  - `data/sprites_extracted/crapim/sprite_01.png` à `sprite_82.png` —
    un PNG par sprite de `CRAPIM.IMA`.
  - `data/sprites_extracted/backgrounds/astron.png`,
    `banniere.png`, `facade.png`, `krono.png`, `maisonj.png`,
    `maisons.png` — les 6 images plein écran.
  - `data/sprites_extracted/manifest.json` — dimensions brutes
    (largeur en octets, hauteur en pixels tels que lus dans le
    fichier) et taille finale en pixels pour chacun des 82 sprites,
    plus la liste des fonds extraits.
  - `data/sprites_extracted/crapim_contact_sheet.png` — planche
    contact (grille 10 colonnes) des 82 sprites, pour une revue
    visuelle rapide sans ouvrir 82 fichiers.

### Comment ça marche : `CRAPIM.IMA` (82 sprites)

Format confirmé en portant ligne à ligne la routine de chargement
`OVL_00DB.PAS` (déjà décompilée, voir aussi
`docs/TERA_notes_moteur.md` section 10) :

1. Le fichier fait exactement **23206 octets = `0x5AA6`**, qui est très
   exactement la taille du buffer Pascal (`t5AA6 = ARRAY[1..$5AA6] OF
   BYTE`) dans lequel `OVL_00DB.PAS` lit tout le fichier d'un coup
   (`READ(BP_58, BPA2A2)`). Ce n'est pas une coïncidence : ça confirme
   qu'il n'y a ni en-tête ni padding, tout le fichier est utile.
2. Les 82 sprites (identifiants 1 à 82, `SPR_01`..`SPR_52` en
   hexadécimal) sont stockés **à la suite**, sans taille fixe par
   sprite — il faut lire chaque sprite pour savoir où le suivant
   commence (pas d'index/table des offsets dans le fichier).
3. Pour chaque sprite, dans l'ordre : **1 octet = largeur EN OCTETS**
   (pas en pixels — chaque octet encode 4 pixels CGA de 2 bits), **1
   octet = hauteur en pixels**, puis les données de pixels.
4. Les données de pixels sont réparties en **deux "plans"** qui
   correspondent aux deux bancs mémoire entrelacés du CGA (lignes
   paires dans un plan, lignes impaires dans l'autre — comme
   `$B800`/`$BA00` sur le vrai matériel). **Point important, source du
   bug rencontré (voir plus bas)** : ces deux plans sont entrelacés
   **octet par octet** dans le fichier (octet 1 → plan pair, octet 2 →
   plan impair, octet 3 → plan pair, etc.), **pas** stockés comme deux
   blocs contigus.
5. **Sprites "grands format"** (identifiants > `SPR_32` = 50) : le jeu
   leur applique un facteur d'échelle (×2 ou ×3, horizontal et/ou
   vertical) — mais ce facteur est déjà reflété dans la **quantité de
   données stockées** (le fichier contient directement le bitmap à sa
   taille finale, pas un petit bitmap redessiné plus grand au chargement).
   La cascade de conditions qui détermine l'échelle exacte par
   identifiant est portée telle quelle depuis le Pascal dans
   `_scale_for()` du script.
6. Nombre d'octets par plan pour un sprite donné = `largeur × hauteur ×
   échelle_h × échelle_v ÷ 2`, avec un cas particulier pour les petits
   sprites (`id < SPR_3B` = 59) à hauteur impaire. Ported tel quel
   (voir le commentaire dans `extract_sprites.py` pour la formule
   exacte) — validé par le fait que les 23206 octets du fichier sont
   consommés **exactement**, sprite après sprite, sans reste ni
   dépassement.
7. Décodage final : chaque octet d'un plan encode 4 pixels (2 bits
   chacun, poids fort en premier), une valeur 0-3 par pixel, passée à
   travers une palette CGA 4 couleurs pour obtenir du RGB.
8. **Transparence (ajoutée après coup, retour de Merci "tu peux gérer la
   transparence des PNG ?")** : les 82 PNG de `CRAPIM.IMA` étaient
   d'abord exportés en RGB opaque (y compris l'indice de palette 0/noir)
   — sans conséquence quand un sprite est seul sur fond noir, mais un
   vrai problème dès que plusieurs sprites se chevauchent (les bouquets
   d'arbres `INNER_BEA8`/`INNER_BE5A` placent jusqu'à 5 sprites avec de
   petits décalages) : chaque sprite dessiné par-dessus effaçait un
   rectangle noir opaque sur celui du dessous. Corrigé dans
   `extract_sprites.py::_interlaced_image` (paramètre `transparent_key`) :
   les sprites `CRAPIM` sont maintenant exportés en RGBA avec l'indice 0
   (noir) transparent — convention standard pour ce genre de blit CGA
   2 bits/pixel sans canal alpha matériel, dont le jeu original avait
   nécessairement besoin pour superposer ses sprites sans bloc noir
   visible. Les 6 fonds plein écran restent opaques (`transparent_key=None`),
   ce sont des remplissages plein cadre, pas des sprites composités.
   Vérifié visuellement (deux arbres superposés sur fond rouge vif —
   aucun rectangle noir résiduel).

### Le bug rencontré (et pourquoi le corriger a suffi)

La toute première extraction produisait des images bruitées
(reconnaissables comme "il y a bien des formes" mais aucun sprite
clair). Cause : le script traitait les deux plans comme **deux blocs
contigus** dans le fichier (tous les octets du plan pair, puis tous
ceux du plan impair) au lieu de les lire **entrelacés octet par
octet** comme le fait réellement la boucle Pascal
(`SpriteData[0][i]:=octet; SpriteData[1][i]:=octet_suivant`). Le
nombre total d'octets consommés était identique dans les deux cas
(23206/23206), donc ce contrôle-là ne suffisait pas à détecter
l'erreur — seule l'inspection visuelle (planche contact) l'a révélée.
Après correction de l'entrelacement, les sprites sont nets et
reconnaissables (personnages encapuchonnés, cobra, squelettes, armes,
décors).

### Comment ça marche : les 6 images plein écran

`ASTRON.IMA`, `BANNIERE.IMA`, `FACADE.IMA`, `KRONO.IMA`, `MAISONJ.IMA`,
`MAISONS.IMA` font chacun exactement **16384 octets = `0x4000`**, la
taille d'une page mémoire CGA complète. **Aucune routine de chargement
n'a été retrouvée dans le code décompilé pour ces fichiers** (pas
d'overlay identifié qui les charge, contrairement à `CRAPIM.IMA`) —
traités par hypothèse comme des captures d'écran CGA standard 320×200 :
deux blocs **contigus** (pas entrelacés octet par octet, contrairement
à `CRAPIM.IMA`) de 8000 octets utiles chacun, aux offsets fixes `0x0000`
(lignes paires) et `0x2000` (lignes impaires), avec du padding jusqu'à
8192 octets par bloc. Hypothèse **confirmée visuellement** :
`BANNIERE.IMA` donne l'écran-titre "TERA — LA CITÉ DES CRÂNES" et
`KRONO.IMA` donne la skyline de Kronopolis — les deux nets et corrects.

**Contenu de chacune** (inspection visuelle des PNG extraits, 2026-07-26) :

| Fichier | Contenu |
|---|---|
| `BANNIERE.IMA` | Écran-titre "TERA — LA CITÉ DES CRÂNES", montagnes, arbre, lune |
| `KRONO.IMA` | Skyline de Kronopolis (immeubles futuristes, lune, arbre) |
| `ASTRON.IMA` | Cockpit du vaisseau pour le mini-jeu de combat spatial (section 8 des notes moteur / "Les combats galactiques" du manuel) : vitre de poste de pilotage + tableau de bord |
| `FACADE.IMA` | Façade à colonnade/arcades — probablement l'entrée d'un bâtiment (temple ou boutique de Kronopolis), non identifié précisément |
| `MAISONJ.IMA` | Façade de maison (porte, fenêtre, arbre) |
| `MAISONS.IMA` | Variante de la même scène de maison — **pas identique** à `MAISONJ.IMA` (~27% des pixels diffèrent : `ImageChops.difference` sur les PNG extraits), surtout le motif de briques du mur et la couleur de l'arbre. Signification du "J" vs "S" non confirmée (hypothèse : jour/nuit via palette différente au runtime, ou deux maisons de PNJ différents) — aucune routine de chargement retrouvée pour trancher. |

### Palette de couleurs — hypothèse à vérifier

Le script utilise par défaut la palette CGA "noir / cyan / magenta /
blanc" (la plus courante pour ce genre de jeu DOS de l'époque). Elle
est **confirmée visuellement** par la bannière-titre et la skyline
(couleurs cohérentes avec ce qu'on attend). Mais le code Pascal appelle
`PALETTE(0..3)` à plusieurs endroits (`OVL_0145.PAS`), ce qui suggère
que le jeu utilise **plus de 2 combinaisons de palette CGA** selon le
contexte — donc **certains sprites individuels pourraient avoir des
couleurs incorrectes** avec la palette par défaut. Non vérifié sprite
par sprite ; à garder en tête si un sprite semble avoir des couleurs
bizarres une fois utilisé dans le rendu.

## Principes de travail sur ce projet

- **La fidélité aux mécaniques originales prime** sur l'élégance du
  code tant que Merci n'a pas dit explicitement de moderniser un
  système donné (voir le tableau "garder vs moderniser" dans le
  blueprint).
- Quand un point du moteur original est incertain ou non confirmé,
  le dire explicitement (comme le fait `docs/TERA_notes_moteur.md`)
  plutôt que d'inventer un comportement plausible.
- Merci apprécie des instructions concrètes, séquentielles, sans
  ambiguïté — éviter le flou artistique dans les explications
  techniques.
- Merci est à l'aise avec Python, PowerShell, le développement de
  jeu procédural (voir Oracle Toolbox, un autre de ses projets) —
  pas besoin de réexpliquer les bases.
- S'il faut désassembler à nouveau du code depuis `TERA.COM` (des
  fichiers `.PAS` non couverts, par exemple, ou pour vérifier un
  détail), la méthode qui a fonctionné est documentée dans
  `docs/TERA_notes_moteur.md` section 13 (`objdump -b binary -m i8086`)
  — les adresses des `.PAS` reconstruits correspondent directement
  aux adresses réelles du binaire. `docs/TERA_guide_desassemblage.md`
  détaille la procédure complète pas à pas (reconnaître une fonction
  Pascal compilée, retrouver l'ordre des paramètres, les tables, les
  primitives runtime) avec des exemples réels, et les 6 transformations
  récurrentes utilisées pour porter le Pascal reconstruit en Python.

## Commandes utiles

**Le plus simple : double-clique sur `lancer_tera.bat`** à la racine du
projet — un menu (jouer en graphique/texte, tests, extraction des
sprites/données) qui règle automatiquement `PYTHONPATH` et choisit le
bon interprète (`python`/`python3`) pour chaque option, sans avoir à
taper les commandes ci-dessous à la main.

**Important sur ce poste** : deux interpréteurs Python coexistent.
`python3` (dans le PATH via MSYS64) n'a **pas** `pip`/`pygame`. `python`
pointe vers `E:\Python\Python310\python.exe`, qui a `pip` et où
`pygame` a été installé (`requirements.txt`). Utiliser `python3` pour
les tests/scripts sans dépendance externe, `python` pour tout ce qui
touche à `tera.render`.

```bash
# Lancer les tests de fumée (aucune dépendance externe)
python3 tests/test_smoke.py

# Boucle de jeu textuelle (CLI, aucune dépendance externe)
python3 -m tera.game
# Demarre sur Amarande. "entrer" a l'entree de la Cite des Cranes ou
# d'Alfol, "monter" a la Pyramide (mot secret) pour rejoindre Meduz.

# Rendu Pygame (necessite `python`, pas `python3` -- voir note ci-dessus)
python -m tera.render.app
# Demarre sur Amarande, comme la boucle textuelle. Amarande/Alfol : vue
# "scene" (pas de carte), fleches/WASD = N/S/E/O. Donjons : vue couloir
# par defaut (avancer/reculer/pivoter), [T] pour la carte de secours en
# vue du dessus. Actions (donjons) : [E] entrer, [U] sortir, [O]uvrir,
# [C]ombattre, [I]nvoquer, [M]onter/mediter, [J] descendre, [P]ayer.
# Combat/mots secrets/paiement se saisissent SUR UNE LIGNE DE COMMANDE
# DANS LA FENETRE (pas de console separee, voir CLAUDE.md).
# Echap/Q = quitter, Echap annule aussi une saisie en cours.

# Extraction des sprites/images (necessite `python`, Pillow -- voir requirements.txt)
python tools/extract_sprites.py
# Sortie : data/sprites_extracted/{crapim,backgrounds}/*.png + manifest.json

# Structure du projet
src/tera/
  models.py         # Character, MapCell, World, Monster, SaveGame
  worldgen.py        # generation procedurale : Cite des Cranes (port de OVL_0000),
                      # + Alfol-base/Meduz (generate_alfol_base/generate_meduz)
  amarande.py         # Amarande : montagnes, villes, mots secrets, deplacement (port de OVL_0010/OVL_00FB)
  combat.py          # résolution de combat (port des fonctions désassemblées, C_842A inclus)
  data_loader.py     # charge data/game_data.json (verbes, objets, monstres, PNJ, messages)
  save.py            # sérialisation JSON de SaveGame
  game.py            # boucle de jeu textuelle (CLI) -- Amarande/Alfol/Meduz/Cite des Cranes ;
                      # expose aussi les fonctions reutilisees par render/app.py
                      # (resolve_chest, run_combat, try_stairs_up, handle_amarande_command...),
                      # toutes parametrees par des callbacks ask/log injectables
  render/            # rendu Pygame : corridor.py (donjons, vue par defaut,
                      # sprite de porte SPR_2D), topdown.py (carte de
                      # secours, touche T), scene.py (Amarande : port
                      # fidele des 23 scenes fixes + surface d'Alfol,
                      # modernisee), amarande_scenes.py (table des 23
                      # scenes, portee d'OVL_00FB.PAS), sprites.py
                      # (chargement/cache des PNG extraits, table de
                      # sprites confirmes), chargen.py (ecran de creation
                      # de personnage mode texte fidele), textinput.py
                      # (ligne de commande dans la fenetre), app.py
                      # (boucle + dispatch)
tools/
  extract_sprites.py    # extraction PNG des .IMA (sprites + fonds d'ecran)
  extract_text_data.py  # regenere data/game_data.json depuis les fichiers sources bruts
data/game_data.json  # données réelles extraites des fichiers VERBES/OBJETS/NOMON/NOMPERS/VALMON/MESSAGE*
```
