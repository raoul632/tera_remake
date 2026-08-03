# TERA (LORICIELS, octobre 1986) — Notes d'architecture pour remake

Document de synthèse basé sur l'analyse du code source Turbo Pascal décompilé
(overlays `OVL_*.PAS`, `TERA.PAS`, `TERAMAIN.PAS`). `TERAUTIL.INC` et `KBD.INC`
n'ont pas été fournis : le système de combat au tour par tour et les primitives
graphiques bas niveau (`PutSprite`, `DrawMountains`, `DrawSkyscrapers`, `DRAW`,
`PLOT`...) restent donc des boîtes noires dont on ne connaît que l'usage,
pas l'implémentation.

## 1. Plateforme

- PC/compatible, **CGA 320×200×4 couleurs**, mémoire graphique à `$B800:0000`
  (16 Ko, `tScreen`), lignes paires/impaires entrelacées (`$B800`/`$BA00`)
  — d'où les fonctions `Off2Screen`/`Screen2Off` qui jonglent entre écran
  visible et double-buffer.
- Turbo Pascal avec **overlays** (`OVERLAY PROCEDURE`) : chaque module de jeu
  (une ville, un donjon, une séquence) est chargé à la demande par-dessus
  la même zone mémoire, car le jeu complet dépasse la RAM disponible en 640 Ko.
  Détail complet du mécanisme en 1.1.

### 1.1 Le système d'overlay, en détail

**Le principe** (limitation matérielle de l'époque, pas un choix de
design) : DOS ne donne accès qu'à 640 Ko de RAM adressable. `TERA.PAS`
fait ~135 Ko de code compilé une fois tous les modules assemblés — trop
pour tenir en mémoire en même temps que les données de jeu, les
sprites, etc. Turbo Pascal 3 répond à ça en compilant les procédures
marquées `OVERLAY PROCEDURE` **à part**, dans un fichier séparé, et en
ne chargeant qu'**un seul module à la fois** en mémoire, à une adresse
fixe, en écrasant le précédent à chaque fois qu'on change d'écran/de
lieu.

**Ce qui reste résident en permanence** : le code non marqué `OVERLAY`
dans `TERA.PAS`/`TERAMAIN.PAS`, y compris `KBD.INC` et `TERAUTIL.INC`
(inclus juste *avant* la liste des overlays, ligne 266-267 de
`TERA.PAS`) — c'est précisément pourquoi le système de combat a pu être
retrouvé par désassemblage direct de `TERA.COM` (section 13/17) : il
n'est jamais déchargé, contrairement aux `OVL_*`.

**Découverte utile en préparant cette section** : `TERA.PAS` annote
chaque `{$I OVL_XXXX.PAS}` d'un commentaire donnant sa taille compilée
en octets, et un commentaire juste au-dessus de la liste précise
`{loaded at ba0f}` — un seul emplacement mémoire fixe (segment `BA0Fh`)
reçoit tour à tour chacun des 26 modules. **La somme de ces 26 tailles
déclarées fait exactement 84992 octets (`0x14C00`) — la taille exacte
de `TERA.000`, au octet près.** Ça confirme sans ambiguïté que
`TERA.000` est la concaténation brute des 26 modules compilés, dans
l'ordre exact de leur déclaration dans `TERA.PAS`, chacun rechargé
entièrement à chaque fois par-dessus le précédent :

| Overlay | Taille | Rôle |
|---|---|---|
| `OVL_0000` | 4096 | Génération du monde (section 4/5.3) |
| `OVL_0010` | 4608 | Initialisation de constantes (rôle précis incertain) |
| `OVL_0022` | 3072 | Création de personnage (section 6) |
| `OVL_002E` | 6912 | Cité des Crânes, étage d'entrée (monde 0) |
| `OVL_0049` | **8192 (le plus gros)** | Moteur générique des étages 1 à 8 de la Cité des Crânes (section 5.3) |
| `OVL_0069` | 7168 | Temple de Seth (section 5.4) |
| `OVL_0085` | 7936 | Alfol (surface + base), rendu couloir (section 5.2) |
| `OVL_00A4` | 2560 | Meduz |
| `OVL_00AE` | 3584 | Séquence de tir spatial (section 8) |
| `OVL_00BC` | 3840 | Commerce de Kronopolis (section 7) |
| `OVL_00CB` | 1792 | Écran de fin, bonne/mauvaise fin (section 11) |
| `OVL_00D2` | 768 | Écriture de sauvegarde |
| `OVL_00D5` | 1536 | Lecture de sauvegarde |
| `OVL_00DB` | 1792 | Chargement des ressources (section 10) |
| `OVL_00E2` | 2560 | Écran d'aide |
| `OVL_00EC` | 1024 | Rendu du Sanctuaire de MAHALORE |
| `OVL_00F0` | 768 | Rendu du Temple de Seth |
| `OVL_00F3` | 2048 | Rendu de la Cité des Crânes |
| `OVL_00FB` | 6144 | Amarande (grande carte, section 5.1) |
| `OVL_0113` | 512 | Écran d'ouverture |
| `OVL_0115` | 5888 | Démo 1 (écran-titre) |
| `OVL_012C` | 5632 | Démo 2 |
| `OVL_0142` | 768 | Rendu de la Pyramide |
| `OVL_0145` | 768 | Animation vers/depuis Meduz |
| `OVL_0148` | 256 | Rendu du spatioport |
| `OVL_0149` | 768 | (rôle non précisé dans le commentaire source) |

**Conséquence pratique déjà rencontrée** (section 5.4) : les overlays
Turbo Pascal 3 ne pouvant pas s'appeler mutuellement (un seul chargé à
la fois, à la même adresse), le même code de rendu de couloir a dû être
**copié-collé** dans `OVL_0049` et `OVL_0069` plutôt que factorisé — un
détail d'implémentation directement explicable par cette contrainte
mémoire, et qui n'a plus de raison d'être dans le remake (un seul
renderer de couloir paramétré suffit, voir blueprint section 5).
- Sauvegardes sur disquette (lecteur A dédié, va-et-vient disque jeu / disque
  save), avec recherche de fichiers via appels DOS bas niveau (FCB, `INT 21h`)
  plutôt qu'une API de listing de fichiers standard.

## 2. Structure de sauvegarde (`tSavedata`, ~0x27B2 octets)

Point central de tout le jeu — tout l'état persistant y passe :

- **10 personnages** (index 0-9) : équipement, stats (max 11), compétences
  (max 5), PV/PM, nom, état (`'OK'`/`'HS'`/`'EM'`)
- `f_Maps: ARRAY[0..1099, 1..7]` — 11 mondes de 100 cases (grille 10×10),
  7 attributs par case : murs N/S/E/O (index 1-4), objet/coffre (5),
  rencontre (6), 7e attribut encore incertain
- Argent, XP, niveau, jours écoulés, position sur Alfol, difficulté,
  4 tâches principales à accomplir (`f_IsTaskDone[0..3]`), mots secrets,
  indices (`f_HintStr`)

## 3. Boucle de jeu principale (`TERA.PAS`)

```
Lancement (bannière + ressources)
  → Écran titre (boucle démo jusqu'à touche)
    → Nouvelle partie ?
         oui → génération monde (OVL_0000) + création perso (OVL_0022)
         non → chargement sauvegarde (OVL_00D5)
      → BOUCLE DE JEU
           lecture touche → déplacement (OVL_00FB) → rendu du lieu →
           verbes contextuels (case par lieu) → aide/sauvegarde
         (jusqu'à mort du groupe ou 4 tâches terminées)
      → écran de fin (OVL_00CB)
    ↻ rejouer si partie non terminée
```

Point notable pour le remake : la variable `D_AFD3` (index du lieu courant)
sert à la fois de sélecteur d'écran de rendu ET de sélecteur de logique de
verbe, dans deux `CASE` séparés mais synchronisés à la main. À remplacer par
une table de données lieu → (rendu, actions).

## 4. Génération procédurale des donjons (`OVL_0000`)

Chaque monde (grille 10×10) est creusé par une **marche aléatoire
auto-évitante** de 225 pas (`0xE1`) qui perce des couloirs case par case,
en évitant les bornes du monde, et pose des qualités de murs aléatoires
(1/2/3 — mur simple/porte/mur secret probable) à chaque intersection.
Algorithme de creusement de labyrinthe classique, fait main.

## 5. Trois échelles de carte, trois moteurs de rendu

| Échelle | Vue | Stockage des murs |
|---|---|---|
| Donjons (Cité des Crânes, Temple de Seth, base d'Alfol, Meduz) | 1ère personne, couloir | `f_Maps[pos][1..4]` |
| Alfol — surface extérieure | Dessus, sprites déco mod-23 | **Aucun** — simple arithmétique de position (0-99, rebouclage modulo), pas de génération de donjon (voir correction section 5.3/19) |
| Amarande (carte du monde entier) | Dessus, sprites déco mod-23 | Montagnes-lignes (`f_PosHMountains`/`f_PosVMountains`) |

### 5.1 Déplacement grande carte (Amarande, `OVL_00FB`) et Alfol (`OVL_0085`)

- Grille **30×30** pour Amarande (deux hémisphères de 900 cases qui se
  rebouclent l'un dans l'autre aux bords, comme un tore), **10×10** pour
  Alfol. Indexation *column-major* : ±1 = nord/sud, ±30 (ou ±10 sur Alfol)
  = est/ouest.
- Amarande utilise des **chaînes de montagnes en lignes complètes**
  (`f_PosHMountains`, 9 valeurs bloquant N/S ; `f_PosVMountains`, 25 valeurs
  bloquant E/O) plutôt qu'un masque de mur par case — économe en mémoire
  mais ne peut modéliser que des barrières rectilignes intégrales.
- **Décor procédural par modulo** : `CASE Position MOD 23 OF` avec 23 scènes
  prédéfinies de sprites (arbres/rochers/bâtiments). 23 est premier avec
  30 (et avec 10), donc le motif ne se répète jamais visiblement sur une
  ligne/colonne entière — un trick habile pour donner une impression de
  terrain varié avec un minuscule budget mémoire. Le même jeu de fonctions
  de placement de sprites est dupliqué presque à l'identique entre
  `OVL_00FB` et `OVL_0085` (les overlays ne partagent pas facilement de
  code entre eux).
- Silhouette de montagne dessinée en vectoriel (`DRAW`, pas de sprites)
  quand on est adjacent à une chaîne, avec ciel étoilé aléatoire.

### 5.2 Rendu couloir 1ère personne (donjons + base d'Alfol, `INNER_CAFC` dans `OVL_0085`)

Moteur façon Wizardry/Bard's Tale : à chaque pas, calcule le **delta entre
position actuelle et précédente** pour déduire la direction de déplacement,
puis mappe les indices absolus de mur (`f_Maps[pos][1..4]`) sur les côtés
relatifs (gauche/droite/face) selon cette direction — un mur "face" dessine
aussi une porte (`SPR_2D`) si le couloir continue. Rendu purement vectoriel
(`DRAW`), pas de sprites de mur.

**La technique exacte du "pseudo-3D"** (confirmée en lisant les routines
de dessin dans `OVL_0049.PAS`, section 5.3) : **ce n'est ni du
raycasting, ni un moteur 3D avec calcul de projection** — bien trop
coûteux pour un CGA de 1986. C'est une bibliothèque de **formes
vectorielles dessinées à la main, à coordonnées fixes**, une par
combinaison (côté gauche/droit × profondeur 1/2/3 × mur plein/porte
ouverte) :

```pascal
{ mur lateral gauche, profondeur 1 (le plus proche), variante "mur plein" }
DrawRect($19,$88,$23,$62,1);
DRAW($19,$88,$19,$8B,1);
DRAW($19,$8B,$23,$88,0);
{ meme cote/profondeur, variante "porte ouverte" (le couloir continue) }
DRAW($1A,$88,$22,$88,0);
DRAW($19,$8B,$23,$88,1);
FillShape($1b,$82,3,1);
```

`INNER_BDBB`/`INNER_C02C` (murs latéraux gauche/droite) ont chacun
**3 jeux de coordonnées figées** (un par profondeur 1/2/3, choisi par
paramètre), toutes des valeurs numériques codées en dur — pas de calcul
géométrique, juste une petite table de "vues" pré-dessinées qu'on
sélectionne selon la profondeur. Seuls le mur de face (`INNER_C29D`) et
la porte texturée (`__DrawPatternedDoor`) font un minimum de calcul, et
seulement une **mise à l'échelle linéaire simple** (multiplication par
l'indice de profondeur, pas de division perspective) :

```pascal
DrawRect(160 - 12 * _ratio_w, _ratio_h, 160 + 12 * _ratio_w, _ratio_h - 32 * _ratio_w, 1);
```

Le moteur avance case par case dans la direction du regard et appelle
ces routines avec l'indice de profondeur croissant (1, 2, 3) pour les
côtés gauche/droite tant que le couloir continue, en s'arrêtant dès
qu'un mur bloque (dessin du mur de face) ou que la distance de rendu
max est atteinte (le fond reste noir). Concrètement : un flip-book de
2-3 vues perspective dessinées à la main par les développeurs plutôt
qu'un calcul de perspective réel — exactement le genre d'astuce qu'on
attend d'un jeu CGA de cette époque. Le renderer du remake
(`src/tera/render/corridor.py`) reproduit le *principe* (plans qui
rétrécissent avec la profondeur) mais avec une **mise à l'échelle
géométrique calculée** (`DEPTH_SHRINK = 0.62` par plan) plutôt que des
coordonnées figées à la main — cohérent avec le choix du blueprint de
moderniser le rendu plutôt que de le reproduire trait pour trait.

### 5.3 La Cité des Crânes : un donjon de 9 étages, pas deux zones fixes

En vérifiant directement `OVL_0000` (la génération du monde), il s'avère
que la profondeur réelle est bien plus grande que ce qu'on pensait.

**Génération en deux passes** :
1. Les **11 "mondes"** (`WorldId` 0 à 10) sont tous creusés par la même
   marche aléatoire (`INNER_BBD3`) — y compris Alfol et sa base, qui
   utilisent donc le même algorithme de labyrinthe que les donjons.
2. `INNER_BD96` fait la vraie passe de contenu (monstres, coffres,
   escaliers, indices), en parcourant les cases de 899 à 1 :
   - **Progression de difficulté explicite** : cases < 200 (mondes 0-1)
     tirent des monstres faciles (`RANDOM(11)+1`), cases > 200
     (mondes 2+) tirent dans un pool plus corsé (`RANDOM(13)+8`).
   - **Un escalier montant est placé au début des mondes 2 à 7**
     (cases 200,300,400,500,600,700) — la preuve que ces mondes
     s'enchaînent en **un seul donjon continu de plusieurs étages**,
     pas des zones indépendantes.
   - **Une clé garantie par étage** (mondes 0-8), numérotée
     séquentiellement.
   - **Un autel de soin garanti par étage** (la case marquée 103).
   - **Arioch** est placé à l'endroit où un escalier descendant *aurait
     dû* être généré, mais uniquement pour les cases > 700 — **Arioch se
     trouve donc toujours au monde 7** (cases 700-799), jamais ailleurs.
   - Une case de **sortie finale** (marqueur 110) est cherchée dans le
     monde 8 (cases 824-899).

**Auto-validation de la génération** : toute la création du monde est
recommencée en boucle (`REPEAT...UNTIL BPFE3B = 11`) tant que 11
conditions structurelles précises ne sont pas toutes remplies (un
escalier par étage 0-6, Arioch bien positionné en monde 7, une sortie
trouvée en monde 8, et les mondes 9/10 structurellement valides) — **le
jeu garantit qu'une partie générée est toujours complétable**, quelle
que soit la seed aléatoire.

**Mondes 9 et 10 = Alfol** (surface et base intérieure), avec un
nettoyage spécifique des qualités de mur (valeurs > 1 ramenées à 1),
cohérent avec le rendu plus simple observé dans `OVL_0085`.

**Structure finale** :

| Monde (`WorldId`) | Contenu |
|---|---|
| 0 | Premier étage, entrée du donjon (gérée par `OVL_002E`) |
| 1-6 | Étages de progression, un escalier montant/descendant chacun (gérés par `OVL_0049`, générique) |
| 7 | **Arioch, toujours ici** |
| 8 | **Temple de Seth** (lieu séparé, pas un étage de la Cité des Crânes — voir section 5.4) |
| 9 | **Alfol — base intérieure** |
| 10 | **Meduz** |
| 11 | Pas un vrai index `f_Maps` — valeur sentinelle signifiant "sur la carte d'Amarande, hors donjon" |

**Correction (section 19)** : cette ligne disait auparavant "9 = Alfol
surface, 10 = Alfol base" — inversé/faux sur les deux points, corrigé
après lecture directe de `OVL_0085.PAS` (`WorldId := 9` explicite pour
la base d'Alfol, confirmé aussi par `OVL_0010.PAS` qui place les PNJ 7
et 9 avec `INNER_BA2F(9)`, et l'un d'eux — PNJ 7 — est bien retrouvé
dans la base d'Alfol par `OVL_0085`) et `OVL_00A4.PAS` (`WorldId := 10`
explicite pour Meduz). **La surface d'Alfol n'est pas un monde numéroté
du tout** : `OVL_0085.PAS` la gère par simple arithmétique de position
(0-99, rebouclage modulo simple) sans jamais lire `Sav.f_Maps` pour les
murs — pas de génération de donjon pour elle, juste un décor procédural
modulo-23 comme Amarande. Voir section 19 pour le détail complet.

**Correction** : une case de sortie (marqueur `110`) est bien cherchée
dans les cases 824-899 lors de la validation de génération — mais ce
n'est pas "la sortie de la Cité des Crânes" comme supposé plus tôt,
plutôt une coïncidence de plage de cases avec le Temple de Seth qui,
lui, est entièrement séparé (voir ci-dessous). Le monde 8 est donc
généré comme les autres (marche aléatoire + contenu procédural) mais
occupé par un lieu totalement différent, accessible directement depuis
la carte d'Amarande plutôt que par les escaliers du donjon.

Ça confirme au passage pourquoi `OVL_0049` est le plus gros overlay du
jeu : ce n'est pas juste "l'étage 2" de la Cité des Crânes, c'est le
**moteur générique qui gère tous les étages 1 à 8**, avec l'arithmétique
`WorldId := WorldId ± 1` et `Sav.f_Position := 100 * WorldId` qu'on avait
repérée pour les escaliers.

**Rendu du couloir** : plus élaboré que celui d'Alfol — `INNER_BFCB` et
`INNER_C211` dessinent plusieurs segments de mur qui s'enfoncent en
profondeur (plusieurs "plans" reculant vers un point de fuite), une vraie
perspective de couloir à la Akalabeth/Ultima I, pas un simple mur unique
par écran.

**Coffres et pièges** : `f_Maps[pos][5]` encode le contenu d'une case
"coffre" : `0` = vide, `1-25` = objet de la table `OBJETS`, `26-89` = or
(`valeur - 20` pièces), `90-98` = clé n°(valeur-90), `99` = carte du
niveau, `100`/`101` = escalier montant/descendant, `103` = autel de soin,
`0xFF` = pas de coffre du tout. Ouvrir un coffre non vide teste le
désamorçage de piège :
```
SI (Force_Agilité + Habileté + bonus_compétence) * (1 + RANDOM(4)) > 11 + Difficulté
ALORS piège désamorcé
SINON un des 4 effets aléatoires : gaz (assomme tout le groupe présent),
      explosion (dégâts à tous), cyanure (assomme 1 perso), lame (dégâts à 1 perso)
```

**Autel de soin** (case marquée `103`) : `'I'nvoquer` y teste la
**Vitalité** de chaque personnage présent (`C_8315(perso,6,6)`, confirmant
que "invoquer" utilise bien la stat Vitalité) et restaure PV/PM en cas de
réussite — un point de soin gratuit en plein donjon.

**Indices qui s'approfondissent** : `'L'ire` sur une inscription (case
> 103) affiche un texte de `Sav.f_HintStr`, dont le contenu exact dépend
de la profondeur (`WorldId`) — formule différente pour l'étage 1, les
étages 2-4, et 5+, cohérent avec des indices de plus en plus précis à
mesure qu'on s'approche d'Arioch.

**Fin du donjon (tâche 3 - tuer Arioch)** : à chaque victoire de combat,
`IF Sav.f_Position = Sav.f_PosArioch THEN Sav.f_IsTaskDone[2] := TRUE` —
la tâche "tuer Arioch" se valide simplement en remportant le combat à la
case précise où le jeu a placé Arioch à la génération du monde (toujours
au monde 7 désormais, comme établi ci-dessus).

**Sortie** : il faut remonter jusqu'à la case `99` du monde 0 précisément
pour ressortir vers Amarande (`UNTIL Sav.f_Position = 99 OR mort du groupe`).

### 5.4 Le Temple de Seth (`OVL_0069`, monde 8) — l'Autel des Ténèbres et la tâche 4

Entrée fixe et indépendante du donjon principal :
```
Sav.f_Position := 800; WorldId := 8; D_9B72 := 799
```
Le rendu utilise **trois procédures figées** (`INNER_BAC0`/`INNER_BB8B`/
`INNER_BC92`, une par valeur de `f_Maps[pos][7]`) plutôt que la marche
aléatoire habituelle en couloir simple — trois "gabarits" de salle de
tailles différentes, donnant une impression de temple architecturé plutôt
qu'un labyrinthe uniforme, bien que la génération sous-jacente reste la
même marche aléatoire que partout ailleurs.

**Fonctions de rendu partagées avec `OVL_0049`** : `INNER_BD7B`/
`INNER_BFEC`/`INNER_C25D`/`__DrawPatternedDoor` ici sont **identiques,
constante hexadécimale par constante**, à `INNER_BDBB`/`INNER_C02C`/
`INNER_C29D`/`__DrawPatternedDoor` dans `OVL_0049` (même nom de fonction
pour cette dernière !). Ce n'est pas une coïncidence de conception : les
overlays Turbo Pascal 3 ne pouvant pas s'appeler mutuellement, le
développeur a dû **copier-coller le même module de rendu de couloir**
dans chaque fichier qui en avait besoin. Pour le remake, c'est une
excellente nouvelle : un seul composant de rendu de couloir paramétré
(par `ratio_h`/`ratio_w`, sprite de porte selon l'échelle) suffit à
couvrir tous les donjons du jeu.

**L'Autel des Ténèbres — tâche 4 (tuer le "copain d'Arioch")** : sur la
case spéciale marquée `110` (propre au Temple, sans rapport avec le
marqueur `110` généré ailleurs), lire l'inscription affiche "AUTEL DES
TENEBRES" ; `'I'nvoquer` y demande un nom au clavier, comparé à
`Sav.f_SecretWords[4]` :
```
SI nom saisi = Sav.f_SecretWords[4] ALORS
    SpawnMonster(21)   { toujours le Prince-démon, le monstre le plus
                         puissant de la table NOMON/VALMON }
    SI victoire ALORS Sav.f_IsTaskDone[3] := TRUE   { tâche 4 validée }
```
Le "copain d'Arioch" n'est donc pas aléatoire dans son identité de
monstre : c'est toujours le Prince-démon, seul son *nom* (le mot à
prononcer pour le convoquer) est généré aléatoirement en début de partie.

**Piège de coffre** : formule identique à la Cité des Crânes (désamorçage
via Agilité+Habileté+compétence vs seuil `11 + Difficulté`, sinon un des
4 effets aléatoires gaz/explosion/cyanure/lame) — encore une preuve du
même module dupliqué partout.

**Sortie** : `WorldId := 11; Sav.f_Position := BackupPosition` — confirme
que `11` est bien la valeur sentinelle "hors donjon, sur Amarande", et
que la position sur la carte du monde est sauvegardée avant d'entrer
dans un lieu spécial pour permettre d'y revenir exactement.

## 6. Création de personnage (`OVL_0022`)

- 3 statistiques primaires tirées aléatoirement (Force/Intelligence/Chance,
  total = 18), chacune répartie par le joueur entre deux stats secondaires :
  - Force → Endurance / Agilité
  - Intelligence → Volonté / Charme
  - Chance → Vitalité / Habileté
- Bonus de genre : +2 Force (H) ou +2 Charme (F)
- 6 points bonus à répartir librement sur 8 stats via des lettres
  (F/I/N/E/V/H/A/C/T)
- Argent de départ minimum 500, position de départ `1000`. **Correction**
  (incohérence relevée en relisant cette section après coup) : `1000 =
  100 * WorldId` place bien ça au monde **10** (Alfol — base intérieure,
  cf. tableau section 5.3), pas 11 — le 11 est la valeur sentinelle
  "hors donjon, sur Amarande" (section 5.3/5.4), pas un vrai lieu où
  démarrer. Le jeu démarre donc dans le couloir de la base d'Alfol, une
  vue 1ère personne similaire à un donjon mais un lieu différent de la
  Cité des Crânes. **Ni Alfol ni Amarande ne sont générés par
  `worldgen.py` à ce jour** — `game.py`/`render/app.py` démarrent
  directement dans un donjon Cité des Crânes généré, une simplification
  de portée assumée et documentée (voir `CLAUDE.md`), pas une
  reproduction fidèle du point de départ réel.

## 7. Commerce (Kronopolis, `OVL_00BC`)

- Achat (`__Buying`) : 19 objets (lettres A-S), prix de base modulé par la
  stat Charme (index 8) du perso qui négocie : `prix * 30 / (30 + charme/5)`.
  Objets spéciaux (énergie, cartes, clés) traités hors de l'inventaire
  classique.
- Vente (`__Selling`) : prix de rachat fixe par objet, majoré par le Charme.
- **Hôpital** : soin gratuit ("pris en charge par la SECU" — clin d'œil à la
  sécurité sociale française), mais coûte du temps de jeu (1 semaine par
  point de PV à soigner) plutôt que de l'argent.

## 8. Séquence spatiale — mini-jeu de tir (`OVL_00AE`)

Un shoot 'em up en grille 3×3 :
- Un "alien" apparaît à une position aléatoire dans une grille 3×3
  (`_alienX/_alienY ∈ [-1,0,1]`), puis **s'approche en 5 étapes de zoom**
  (sprites `SPR_1F` → `SPR_1E` → `SPR_1D` → `SPR_43`, de plus en plus gros/proches)
- Le joueur doit taper la touche du pavé numérique (1-9) correspondant à la
  position de l'alien **avant qu'il n'arrive au contact** (dernière étape)
- Raté → perte d'énergie (`Sav.f_Energy -= 5`) + explosion ; trop de pertes
  d'énergie (< 5) = `HALT` (fin de partie brutale !)
- Réussite en vraie partie → +3 XP par tir
- Ravitaillement en énergie payant (50 pièces/unité) avant le combat

## 9. Sauvegarde/chargement

- Système de "scénario" à 3 lettres choisies par le joueur (pas un simple
  slot numéroté) : fichier `SCENARIO.xxx` sur le disque de sauvegarde
- Liste des sauvegardes existantes via recherche de fichiers FCB DOS bas
  niveau (`INT 21h AH=11h/12h`) — API historique pré-`FindFirst`/`FindNext`
- Prompts explicites d'échange de disquette (jeu ↔ sauvegarde) typiques
  du mono-lecteur A: de l'époque

## 10. Sprites (`OVL_00DB`, format `crapim.ima`)

- Blob unique contenant tous les sprites (jusqu'à 82), chaque sprite préfixé
  par largeur/hauteur, données en deux plans (pair/impair) correspondant à
  l'entrelacement CGA (`$B800`/`$BA00`)
- **Mise à l'échelle au chargement** : certains sprites (au-delà de `SPR_32`)
  sont automatiquement dessinés en 2x/3x horizontal et/ou vertical — un seul
  art source réutilisé pour les variantes "big" (cf. commentaires `[big]` /
  `[close]`/`[far]` dans `TERA.PAS`), économie mémoire supplémentaire

**Extraction confirmée** (`tools/extract_sprites.py`) : le format ci-dessus
a été porté ligne à ligne depuis `OVL_00DB.PAS` et validé — les 23206
octets de `CRAPIM.IMA` (= `0x5AA6`, taille exacte du buffer `BPA2A2`) sont
consommés exactement par les 82 sprites, sans reste ni dépassement, et
les images obtenues sont clairement reconnaissables (personnages, cobra,
squelettes, armes, décors). Point d'attention corrigé au passage : les
deux plans pair/impair sont entrelacés **octet par octet** dans le
fichier (`SpriteData[0][i]:=octet; SpriteData[1][i]:=octet_suivant`), pas
stockés en deux blocs contigus — une première tentative d'extraction
avec cette hypothèse erronée produisait des images bruitées.

Les 6 autres fichiers `.IMA` (`ASTRON`/`BANNIERE`/`FACADE`/`KRONO`/
`MAISONJ`/`MAISONS`) font tous exactement 16384 octets (`0x4000`) — une
page mémoire CGA complète. Aucune routine de chargement n'a été retrouvée
pour ceux-ci (pas d'overlay identifié qui les charge), mais traités comme
des captures plein écran 320×200 standard (deux blocs contigus de 8000
octets utiles à 0x0000/0x2000, pas d'entrelacement octet par octet comme
CRAPIM) — hypothèse confirmée visuellement : `BANNIERE.IMA` donne
exactement l'écran-titre "TERA — LA CITÉ DES CRÂNES", `KRONO.IMA` donne
la skyline de Kronopolis vue dans `OVL_00BC`.

**Palette** : CGA 4 couleurs, palette "noir/cyan/magenta/blanc" (la plus
courante pour ce type de jeu DOS) — confirmée visuellement par la
bannière-titre et la skyline, mais le jeu appelle `PALETTE(0..3)` à
plusieurs endroits (`OVL_0145.PAS`), donc **certains sprites/écrans
utilisent peut-être une palette différente** (les 2 autres combinaisons
CGA standard, ou un système de palette étendu propre au jeu) — non
vérifié sprite par sprite.

Sorties : `data/sprites_extracted/crapim/sprite_NN.png` (82 fichiers),
`data/sprites_extracted/backgrounds/*.png` (6 fichiers),
`data/sprites_extracted/manifest.json` (dimensions brutes/finales par
sprite), `data/sprites_extracted/crapim_contact_sheet.png` (planche
contact pour revue rapide).

## 11. Fin de partie (`OVL_00CB`)

- Défaite : texte de "condoléances", butin transmis au prochain aventurier
- Victoire : texte de félicitations + **score calculé** :
  `score = (XP / (jours - 4000)) × (1 + difficulté) × (1 + (difficulté+3)/4) × 120`
  — favorise vitesse (peu de jours), XP élevé et difficulté choisie en
  début de partie

## 12. Combat — confirmé depuis les fichiers de données du jeu

Contrairement à ce qu'on croyait, il n'a pas fallu désassembler `TERA.COM` :
les fichiers de données (`VERBES`, `MESSAGE`, `MESSAGE2`, `MESSAGE3`,
`VALMON`, `NOMON`, `NOMPERS`, `OBJETS`) sont de simples fichiers Pascal
typés (`FILE OF STRING[n]` ou `FILE OF t108`), donc trivialement lisibles
en Python (chaîne = 1 octet de longueur + N octets de contenu, encodage
CP437). Ça confirme presque tout le système de combat.

### 12.1 Les 7 actions de combat (`VERBES`, indices 26-32)

`tirer / lutter / jeter un sort / dissiper / invoquer / attaque mentale /
psalmodier` — exactement l'ordre déduit des commentaires du blog CRPG
Addict. Chaque action a **trois variantes de texte de résultat**, tirées
en alternance de `MESSAGE`, `MESSAGE2` et `MESSAGE3` (même index 26-33
dans les trois fichiers) — un système de variation de texte pour éviter la
répétition à chaque tour de combat :

| # | Action | `MESSAGE` | `MESSAGE2` | `MESSAGE3` |
|---|---|---|---|---|
| 26 | tirer | tire | vise | ajuste son tir |
| 27 | lutter | frappe | assène un coup | frappe deux coups |
| 28 | jeter un sort | fait appel à sa magie | tente de matérialiser la foudre | jette un sort |
| 29 | dissiper | entre en transe | invoque les anciens dieux | fait appel aux éléments |
| 30 | invoquer | invoque les anciens dieux | est exaucé(e).. | *(vide)* |
| 31 | attaque mentale | se concentre | controle par hypnose | projette un flux mental |
| 32 | psalmodier | psalmodie une incantation | entonne le chant incantatoire | prononce l'incantation |
| 33 | crier | crie le mot redoutable | *(inconnu)* | éructe le mot terrible |

Menu de haut niveau lors d'une rencontre (chaînes trouvées en dur dans
`TERA.COM`, donc du code non-overlay) : **Negocier / Abandonner / Combattre
/ Invoquer** — on choisit d'abord une de ces 4 options, et `Combattre`
ouvre le sous-menu des 7 actions ci-dessus.

### 12.2 Table des monstres (`VALMON`, 22 monstres × 6 valeurs)

```
#  nom (NOMON)                        HP  ATK PTS_MAGIE? ?    ?   XP/OR?
1  un cobra                            6   1   20         0    2   7
2  un naja royal                      10   2   20         0    1   6
...
13 un cyborg-laser                    14   7    6         1    1   8
16 des ESPRITS incarnés               28  18   11         3    1  10
21 un prince-démon                    32  25   11         5    1  15
22 des cristaux télépathes             8  10    6         6    5  24
```

Colonnes 1-2 très probablement **HP** et **Attaque**, en progression
cohérente avec l'ordre des monstres (du cobra au prince-démon). La colonne
4 est à **0 pour les monstres 1-12** (petites créatures d'ambiance : cobra,
caïman, mygales, insectes...) et **non-nulle et distincte pour les
monstres 13-22** (les boss narratifs : cyborg-laser, gardes de Merenen,
esprits, prince-démon...). Ça correspond exactement à la remarque du
posteur du forum : *"les points faibles des big bosses (à quels
armes/objets ils sont vulnérables)"* — cette colonne est vraisemblablement
un **identifiant de faiblesse élémentaire/matérielle** que le joueur doit
découvrir en parlant aux PNJ avant d'affronter le monstre.

### 12.3 Autres confirmations croisées

- **Séduire un PNJ du même genre coûte 250 ducats** : confirmé texto par
  `MESSAGE[96]` = *"merci pour les 250 ducats.."*.
- **Prix de la boutique de Kronopolis** (`MESSAGE[72-90]`) correspondent
  au chiffre près à ceux codés en dur dans `OVL_00BC` (revolver 100,
  désintégrateur 500, armure spatiale 1000, épée 25...).
- `ARIOCH` apparaît en toutes lettres dans `TERA.COM` (code non-overlay) —
  c'est donc un nom fixe, pas généré aléatoirement comme les 4 mots secrets
  de quête ; seul le "copain d'Arioch" (tâche 4) a un nom aléatoire.
- 27 PNJ nommés dans `NOMPERS` (dont Thelia, Ymir, Torn, Zaar... cités dans
  le blog), 25 objets dans `OBJETS`, 22 types de monstres dans `NOMON`.

### 12.4 `MESSAGE`/`MESSAGE2`/`MESSAGE3` — extraction complète (102/33/33 entrées)

Ces trois fichiers n'étaient pas encore dans `data/game_data.json`
(seuls les index 26-33 avaient été cités ponctuellement, section 12.1).
Extraits en entier maintenant (`tools/extract_text_data.py`, format
confirmé : mêmes enregistrements à taille fixe 1 octet de longueur + N
octets CP437 que `VERBES`/`OBJETS`/`NOMON`/`NOMPERS`, taille
d'enregistrement 40 octets pour les trois, tailles de fichier 4080/1320/
1320 octets = exactement 102/33/33 entrées, aucun octet perdu). Nouvelles
informations qui en ressortent, au-delà des 8 verbes de combat déjà
documentés :

- **Prix complet de la boutique de Kronopolis** (`MESSAGE[72-90]`, lettres
  a-s) : revolver 100, désintégrateur 500, armure spatiale 1000,
  conteneur d'énergie 50, carte des canaux de MEDUZ 100, plan de la base
  d'ALFOL 100, épée 25, hache 50, arc 75, élixir 90, contrepoison 40,
  clé n°9 25, cuir 90, cuirasse 300, cristal 200, talisman 250, anneau
  300, pierre 350, amulette 750.
- **Noms de lieux non documentés jusqu'ici** (`MESSAGE[53-57]`) : "Sanctuaire
  de MAHALORE", "Domaine de Balgence", "Ermitage de la Roche aux Goules",
  "Hameau de la Porte des sables", "Abbaye de Mortefontaine" — des lieux
  d'Amarande visitables en plus de Kronopolis, jamais mentionnés dans les
  sections précédentes de ces notes (aucun overlay ne leur a encore été
  associé avec certitude).
- **Groupe limité à 4, confirmé texto** (`MESSAGE[98]`) : *"vous êtes déjà
  4 !"* — recoupe exactement ce que dit le manuel officiel (section 18).
- Autres éléments utiles : `MESSAGE[60-63]` (pyramide, "chute dans le
  temps..", entrée du Temple de Seth), `MESSAGE[71]` ("invoquer le
  seigneur des ombres" — probablement Arioch ou son allié), `MESSAGE[99-
  102]` (atterrissage/ALFOL, "* AMARANDE *", pilotage, temps de trajet).

`data_loader.get_messages(variant=1|2|3)` expose maintenant les trois
listes complètes.

## 13. Combat — formule de jet confirmée par désassemblage de `TERA.COM`

Contrairement à ce qu'on pensait, le système de combat n'est **pas** dans
un fichier `.INC` manquant : il vit directement dans le programme
principal `TERA.COM` (pas dans un overlay), sous forme de fonctions
identifiées par leur adresse mémoire — exactement les mêmes adresses que
les noms `C_XXXX` déjà présents dans les fichiers `.PAS` reconstruits.
Désassemblé avec `objdump -b binary -m i8086`, en suivant `C_AFE8` (le
distributeur de commandes principal, appelé à chaque tour depuis
`TERA.PAS`) jusqu'à la branche `Combattre`.

**Fonction de calcul de puissance d'attaque** (`0x8315`, appelée une fois
par action de combat, avec en paramètres le personnage et le type
d'action 1-9) :

```
puissance := Sav.f_Stats[perso][stat_liee_a_l_action]
           + C_8215(perso, competence)   { bonus/malus de competence }
           + Sav.f_Skills[perso][7]      { bonus fixe }
           + C_8391(perso, objet_requis) { verifie l'equipement necessaire }

jet := puissance * (RANDOM(15) + 3)      { facteur aleatoire x3 a x17 }

SI jet > 44 ALORS degats := jet - 35
SINON echec (aucun effet)
```

Sous-fonctions entièrement résolues :

- **`C_8215(perso, competence)`** : `SI Sav.f_Skills[perso][competence] = 0
  ALORS -2 SINON Sav.f_Skills[perso][competence]` — pénalise un
  personnage qui n'a pas appris la compétence, sinon utilise son niveau
  comme bonus.
- **`C_82E5(roll)`** : `SI roll > 44 ALORS roll - 35 SINON 0` — le seuil de
  réussite/échec, et l'échelle des dégâts en cas de réussite.
- **`C_8391(perso, item)`** : boucle sur jusqu'à 3 personnages du groupe
  (`perso`, `perso-1`, `perso-2`, via `C_8270` qui scanne
  `Sav.f_Equipment[perso][0..9]`) pour vérifier si l'un d'eux possède
  l'objet requis pour l'action (ex : une arme de tir pour "tirer" —
  cohérent avec le message `"n'a pas d'arme de tir"` trouvé dans les
  chaînes de `TERA.COM`). Retourne un **bonus dégressif** : 3 si le
  personnage lui-même a l'objet, 2 si c'est le personnage suivant dans
  l'ordre du groupe, 1 si c'est le second, 0 si personne ne l'a. Mémorise
  aussi (`D_AFCC`) lequel des trois l'a fourni. **Ça explique une
  bizarrerie notée par des joueurs de l'époque (blog CRPG Addict) : des
  PNJ recrutés qui "ratent tout le temps" au combat — s'ils n'ont pas
  l'objet requis pour l'action choisie, ce terme tombe à 0 et fait
  chuter `puissance` sous le seuil de réussite presque systématiquement.**

**Association action → stat** (déduite des index de `Sav.f_Stats`, où
1=Force, 2=Intelligence, 3=Chance, 4=Endurance, 5=Volonté, 6=Vitalité,
7=Agilité, 8=Charme, 9=Habileté — cf. tableau des classes dans `TERA.PAS`) :

| Action (index interne) | Stat utilisée |
|---|---|
| 1 | Force |
| 2 | Habileté |
| 3 | Volonté |
| 4 | Chance |
| 5 | Vitalité |
| 6 | Chance + Vitalité (composé) |
| 7 | Endurance + Agilité (composé) |
| 8 | Intelligence + Volonté (composé) |
| 9 | Intelligence |

**Ce qui reste à confirmer précisément** : le calcul exact de résistance
du monstre a été localisé et percé (voir section suivante).

## 14. VALMON — toutes les colonnes identifiées

Après avoir tracé la boucle de combat complète (`0x95d9`, spawn + initiative
+ butin) et une fonction dédiée à la résistance des monstres (`0x8d20`,
localisée en filtrant les vraies références indexées `[di+disp16]` à
chaque colonne — les recherches naïves d'octets confondaient certaines
adresses avec des variables sans rapport comme `QuitFlag`), les 6 colonnes
de `VALMON` sont maintenant toutes expliquées :

| Colonne | Rôle confirmé |
|---|---|
| 1 | PV de base du monstre |
| 2 | Résistance/défense (type A) |
| 3 | Résistance/défense (type B) |
| 4 | **Faiblesse : ID de l'objet auquel le monstre est vulnérable (0 = aucune faiblesse particulière)** |
| 5 | Initiative/vitesse — comparée à l'Agilité moyenne du groupe pour décider qui attaque en premier |
| 6 | Variance aléatoire ajoutée aux PV de base à l'invocation |

**Mécanique de résistance** (fonction à `0x8d20`) :
```
SI VALMON[type][4] = 0 ALORS
    { monstre générique, pas de faiblesse déclarée }
    resistance := VALMON[type][2 ou 3] DIV (5, 6 ou 7 selon le contexte)
SINON
    { VALMON[type][4] est l'ID d'un objet précis : sa faiblesse }
    SI objet_utilisé = VALMON[type][4] ALORS
        resistance appliquée avec un diviseur plus favorable (dégâts pleinement efficaces)
    SINON
        résistance normale (diviseur moins favorable)
```
Ça confirme noir sur blanc la remarque du posteur du forum abandonware :
certains gros monstres ont une vulnérabilité précise à un objet/arme
particulier, à découvrir en interrogeant les PNJ avant l'affrontement.
Le détail exact des divers embranchements (quelles valeurs précises de
`VALMON[type][4]` correspondent à quels diviseurs/bonus) demanderait
encore un peu de travail pour être cartographié entièrement, mais le
mécanisme général est désormais clair et suffisant pour un remake.

**Méthode utilisée** : `TERA.COM` est un exécutable DOS `.COM` — une copie
brute de mémoire sans en-tête ni symboles, chargée telle quelle à
l'adresse `0100h`. Désassemblé avec :
```
objdump -D -b binary -m i8086 -Maddr16,data16 --adjust-vma=0x100 TERA.COM
```
Les adresses obtenues correspondent exactement aux noms `C_XXXX`/`D_XXXX`
déjà présents dans les `.PAS` reconstruits il y a 15 ans (la personne qui
a fait ce travail a nommé chaque variable/procédure d'après son adresse
mémoire réelle faute de connaître son nom d'origine) — ce qui permet de
recouper directement le désassemblage avec le code source déjà lu.

## 15. Ce qui manque pour une compréhension complète

`TERAUTIL.INC` et `KBD.INC` ne sont pas dans le lot fourni. Ils contiennent
vraisemblablement :
- le système de combat au tour par tour (`SpawnMonster`, `CombatVictoryFlag`,
  et sans doute la résolution des attaques utilisant `MonsterStats`/`t108`)
- les primitives graphiques bas niveau (`PutSprite`, `DrawMountains`,
  `DrawSkyscrapers`, gestion clavier)

Si tu remets la main dessus un jour (même forum probablement), ce sera la
pièce manquante la plus importante.

## 16. Pistes de conception pour le remake

| Sujet | Choix 1986 | Suggestion remake |
|---|---|---|
| Murs de carte | 2 systèmes différents (bitmask par case vs. chaînes de montagnes) | Un seul système de mur par case partout |
| Rendu lieu vs. verbes | 2 `CASE` synchronisés à la main sur le même index | Table de données lieu → (rendu, actions contextuelles) |
| Décor de grande carte | Modulo-23 sur un jeu de scènes fixes | Bruit procédural par seed / table de biomes (cohérent avec Oracle Toolbox) |
| Sauvegarde | Disquette, code 3 lettres, FCB DOS | Fichier JSON/YAML nommé, plusieurs slots |
| Sprites | Atlas brut CGA 2 couleurs, échelle codée en dur au chargement | Sprites vectoriels ou bitmap modernes, échelle via moteur (Pygame/Godot) |
| Combat | Inconnu (fichier manquant) | À concevoir depuis les traces indirectes (`MonsterStats: t108` = 22 monstres × 6 valeurs, `VALMON`) |

## 17. `TERAUTIL.INC`/`KBD.INC` — reconstruction par désassemblage direct

Session complémentaire à la section 13-14, avec le binaire compilé
(`data/compiled program/TERA.COM`) et la méthode déjà documentée
(`objdump -D -b binary -m i8086 -Maddr16,data16 --adjust-vma=0x100
TERA.COM`), mais cette fois en partant du **graphe d'appels** (qui appelle
qui, et avec quelle signature) plutôt que d'adresses isolées. Reconstruction
écrite dans `data/source code/TERAUTIL.INC` et `data/source code/KBD.INC`,
avec un niveau de confiance explicite par fonction (CONFIRMÉ / PARTIEL /
NON RÉSOLU) — voir l'en-tête de ces fichiers.

**Reconfirmation intégrale** de `C_8215`, `C_8270`, `C_82E5`, `C_8315`,
`C_8391` (section 13) : le désassemblage relu ligne par ligne correspond
exactement à ce qui était déjà documenté, aucune correction nécessaire.

**Découverte non documentée jusqu'ici : `C_842A(action_type, perso)`**,
appelée par le menu de combat (`C_8790`, ci-dessous) pour chacune des 9
actions. C'est elle qui combine le jet de base (`C_8315`, seuil 44/35 déjà
appliqué) avec le bonus d'équipement (`C_8391`) — **et pas de la façon
décrite en section 13**. Le désassemblage de `C_8315` ne contient **aucun
appel** à `C_8391`/`C_8270` : le bonus d'équipement n'est pas ajouté à la
« puissance » avant le jet aléatoire. Il est appliqué **après**, par
`C_842A`, avec une formule codée en dur qui **varie selon l'action**.

**Les 9 branches ont toutes été relues ligne par ligne** (mise à jour
par rapport à la version précédente de cette section, qui n'en avait
confirmé que 3) :

```
action 1 (Force, objet requis fixe: 15 épée runique) :
    C_842A := C_8315(1,1,perso) * (1 + C_8391(15,perso))
    { bonus "souple" : le jet de base compte meme sans objet }

action 2 (Habileté, objet requis fixe: 18 désintégrateur) :
    C_842A := (C_8315(9,9,perso) * C_8391(18,perso) * 3) DIV 2
    { bonus "dur" : sans objet (bonus=0), résultat TOUJOURS 0,
      quel que soit le jet de base — c'est là qu'il faut chercher la
      vraie cause du phénomène "PNJ recrutés qui ratent tout le temps"
      déjà noté en section 13, pas dans une simple addition }

action 3 (Volonté, objet requis dynamique: $75ED) :
    C_842A := C_8315(5,5,perso) * (1 + C_8391(D_75ED,perso))
    { formule "souple", objet non fixe }

action 4 (Chance, objet requis dynamique: $75EA) :
    C_842A := C_8315(3,3,perso) * (1 + C_8391(D_75EA,perso))
    { formule "souple" }

action 5 (Vitalité, objet requis dynamique: $75EC) :
    C_842A := C_8315(6,6,perso) * (1 + C_8391(D_75EC,perso))
    { formule "souple" }

action 6 (Chance+Vitalité composé, objet requis dynamique: $75EB) :
    { NE PASSE PAS PAR C_8315 -- réimplémentation en ligne avec DEUX
      bonus de compétence (compétences 6 ET 3, additionnés), même
      facteur RANDOM(15)+3, DIVISÉ PAR 2 avant le seuil 44/35 }
    jet := ((Stats[3]+Stats[6] + C_8215(perso,6) + C_8215(perso,3))
            * (RANDOM(15)+3)) DIV 2
    C_842A := C_82E5(jet) * (1 + C_8391(D_75EB,perso))

action 7 (Endurance+Agilité composé, objet requis fixe: 21 armure spatiale) :
    { NE PASSE PAS PAR C_8315 -- la somme des stats est divisée par 2
      AVANT le facteur aléatoire, aucun bonus de compétence. Plancher
      d'équipement de 2 (pas 1), ET division par 2 finale
      supplémentaire }
    jet := ((Stats[4]+Stats[7]) DIV 2) * (RANDOM(15)+3)
    C_842A := (C_82E5(jet) * (2 + C_8391(21,perso))) DIV 2

action 8 (Intelligence+Volonté composé, AUCUN objet requis) :
    { NE PASSE PAS PAR C_8315 -- un seul bonus de compétence (5),
      aucune vérification d'équipement du tout : cohérent avec une
      action mentale sans besoin d'objet physique }
    jet := (Stats[2]+Stats[5] + C_8215(perso,5)) * (RANDOM(15)+3)
    C_842A := C_82E5(jet)

action 9 (Intelligence, AUCUN objet requis) :
    C_842A := C_8315(2,2,perso)
    { la plus simple des 9 }
```

Ce tableau **confirme intégralement** l'ancienne table "Association
action → stat" de cette même section (les 9 indices, simples et
composés, correspondent exactement), mais **corrige** l'hypothèse sur le
rôle de l'équipement : selon l'action, il est tantôt un bonus optionnel
multiplicatif souple (`1+bonus`), tantôt une condition dure qui annule
tout sans objet (action 2), tantôt totalement absent (actions 8 et 9).
Les objets requis eux-mêmes ne sont pas tous des constantes fixes :
quatre actions (3, 4, 5, 6) lisent l'objet requis depuis une variable
globale distincte (`$75EA`-`$75ED`), très probablement « l'objet
actuellement sélectionné par le joueur pour cette action » — rôle exact
non confirmé, mais clairement pas une constante en dur comme pour les
actions 1/2/7.

**`combat.py` corrigé** (sur demande explicite de Merci, après avoir vu
l'effet en jeu) : `resolve_action()` porte maintenant les 9 branches de
`C_842A` telles que confirmées ci-dessus, avec le bon style de
combinaison par action (souple/dur/absent) et le bon objet requis
(fixe ou fourni par l'appelant). `Character.skills` est maintenant
indexé par **nom de stat** (`force`, `habilete`, ...) et non par nom de
verbe, cohérent avec le fait que chaque appel confirmé de `C_8315`
passe le même indice pour la stat et la "compétence". `action_type` 7
(Endurance+Agilité, objet fixe armure spatiale) reste implémenté dans
`resolve_action()` mais n'est rattaché à aucun verbe dans
`ACTION_TYPES`, faute de savoir avec certitude quel verbe le déclenche
dans l'original.

**`C_8790`** : menu clavier des actions de combat — lit une touche
(`C_4277`), la normalise, et appelle `C_842A(perso, action_type)` pour
l'action correspondante. Table touche→action confirmée seulement
partiellement (3-4 entrées relevées sur ~7-9).

**`C_AFE8`** (distributeur de commandes principal, déjà identifié en
section 13) : reconfirmé — lit une touche, compare au caractère de
chaque verbe (ex. `'I'` = Invoquer). Rien à corriger.

**Table `VALMON` — adresse confirmée** : base `$A745`, 22 entrées de 12
octets (6 mots) chacune, colonnes aux offsets `+0` (PV), `+2`
(résistance A), `+4` (résistance B), `+6` (faiblesse), `+8`
(initiative), `+10` (variance de PV) — confirme l'ordre déjà déduit en
section 14, avec les adresses exactes cette fois.

**PV à l'invocation (dans la grande boucle de combat `0x95D9`)** :
formule retrouvée et confirmée ligne par ligne — `HP := VALMON[type][1]
+ RANDOM(VALMON[type][6])` — identique à `spawn_monster_hp()` dans
`combat.py`, aucune correction nécessaire.

**Fonction de résistance (`0x8D20`) — toujours pas entièrement
résolue.** Localisée et structurellement comprise : elle teste d'abord
`VALMON[type][4]` (faiblesse déclarée) ; si nulle, une cascade de
divisions (`/5`, `/7`, `/9` observés) s'applique sur la colonne
résistance B ; si non nulle, une cascade similaire s'applique sur la
résistance A avec un diviseur `/6` dans les cas observés. **Mais** :
l'hypothèse initiale (section 14) selon laquelle les constantes
comparées dans cette cascade seraient l'ID de faiblesse elle-même s'est
révélée fausse — les valeurs observées dans le désassemblage (12, 20,
10, 1, 3, 4, 16...) ne correspondent pas aux faiblesses réellement
utilisées dans `VALMON` (seulement 1, 3, 4, 5, 6, cf.
`data/game_data.json`). Ces constantes décrivent plus probablement
l'action ou l'objet utilisé par le joueur ce tour-là, mais ceci **n'est
pas confirmé**.

Blocage identifié : le code de cette cascade adresse ses variables via
un pointeur (`BX := [BP-2]`) jamais assigné dans la fonction elle-même —
signe d'une fonction Turbo Pascal **imbriquée** dans une autre, où
`[BP-2]` est le « static link » transmis automatiquement par le
compilateur (mécanisme confirmé par ailleurs : le prologue de `C_842A`
fait exactement `push [BP-2]; mov BP,AX` pour propager ce pointeur d'une
portée à l'autre). Le résoudre précisément demanderait de retracer la
fonction englobante non identifiée qui initialise ce pointeur — pas fait
ce round, effort jugé disproportionné par rapport à la confiance qu'on
pourrait en tirer dans le temps disponible. Volontairement laissé en
`TODO` dans `TERAUTIL.INC` plutôt que d'inventer une formule plausible.
`combat.py` garde donc son approximation actuelle (`resist_a // 5` si
faiblesse exploitée sinon `resist_b // 7`) en attendant.

**`KBD.INC`** : reconstruction très partielle par choix — le blueprint
(section 5-6) prévoit déjà de moderniser entièrement la gestion des
entrées pour le remake, donc reproduire le détail exact des scancodes
DOS/BIOS n'apporte rien à la fidélité des *mécaniques* de jeu,
contrairement au combat. Seul `C_4277` (lecture de touche bas niveau,
utilisée par `C_8790` et `C_AFE8`) a été identifié avec certitude ; son
corps interne (4 sous-appels imbriqués) n'a pas été décodé en détail.

**Primitives graphiques/son repérées en passant, non décompilées** (pour
référence future si besoin) : `0x10DA` = `RANDOM(n)` (confirmé par tous
les sites d'appel), `0x213` = probablement `SOUND`/`DELAY`, `0x11CC` /
`0x1022` / `0x123D` = primitives d'affichage de texte, `0x2558` /
`0x295D` = positionnement curseur/attribut. Aucune de ces primitives
n'a été analysée en détail : cohérent avec le choix du blueprint de
moderniser entièrement le rendu plutôt que de le reproduire pixel pour
pixel.

## 18. Le manuel officiel (`manuel/Tera.pdf`) — confirmations et corrections

Merci a fourni le manuel papier original (scanné, 8 pages, sans couche
texte — extrait en images avec PyMuPDF et lu page par page). C'est une
source de **bien meilleure confiance** que le reverse engineering pour
tout ce qui concerne les mécaniques telles que le joueur les vivait :
c'est la documentation officielle, pas une reconstruction. Points
nouveaux ou qui corrigent des hypothèses précédentes :

- **Lettres de répartition des points de création confirmées** (p.3) :
  `I`(ntelligence), `V`(olonté), `C`(harme), `E`(ndurance), `T`(vitalité),
  `N`(chance), `H`(abileté), `A`(gilité), `F`(orce). Résout l'ambiguïté
  de la lettre `N` dans la section 6 (c'est bien "chance", pas une autre
  stat).
- **Correction de la formule de désamorçage de piège** (p.4) : *"L'agilité
  détermine le niveau des réflexes [...] Ces deux qualités [habileté et
  agilité] déterminent l'adresse du personnage pour désamorcer un
  mécanisme d'ouverture piégé."* — donc **Agilité + Habileté**, sans la
  Force. L'ancienne formule de la section 5.3 (`Force_Agilité+Habileté`)
  incluait la Force à tort, probablement une lecture trop littérale d'un
  nom de variable Pascal composé. Corrigé dans `game.py`.
- **Confirmation** : l'agilité détermine l'initiative en cas de mauvaise
  rencontre (p.4, *"indique qui agit le premier lors d'une mauvaise
  rencontre"*) — cohérent avec `initiative_check()` déjà en place.
- **Schéma de déplacement confirmé, et il diffère selon le contexte**
  (p.5, "Les déplacements") :
  - **Dans les labyrinthes** (vue couloir) : haut = avancer, bas =
    retourner dans la case précédente (la vue s'inverse), gauche/droite
    = **pivoter la perspective à angle droit sans se déplacer**. Ce
    n'est PAS un déplacement absolu — c'est relatif à la direction du
    regard.
  - **Hors des labyrinthes** (grande carte, Amarande/Alfol) : haut/bas/
    gauche/droite = Nord/Sud/Est/Ouest directement.
  - Avant cette lecture du manuel, `src/tera/render/app.py` utilisait le
    schéma "hors labyrinthe" (absolu) pour la vue couloir aussi — corrigé
    pour utiliser le schéma relatif (avancer/reculer/pivoter), qui est
    le seul confirmé pour la Cité des Crânes.
- **Groupe limité à 4 personnages actifs** (p.6), même si 10 personnages
  au total peuvent être créés/recrutés (section 2 des notes moteur) —
  pas encore imposé dans le code, à garder en tête si le recrutement de
  PNJ est implémenté un jour.
- **Menu de mauvaise rencontre confirmé** (p.6) : *"la négociation ne
  sert à rien, il ne reste qu'à combattre ou fuir"* — la rencontre se
  déclenche en tombant sur le monstre, ce n'est pas une action que le
  joueur choisit de son plein gré à un moment arbitraire. Avant cette
  lecture, `game.py` laissait le joueur se déplacer librement à travers
  une case occupée par un monstre sans jamais déclencher de combat ;
  corrigé (`handle_encounter()`, appelée automatiquement à l'entrée
  d'une case avec `encounter is not None`).
- **`EM` = "empoisonné"** (p.7), confirme et précise le sigle déjà
  présent dans `tSavedata` (section 2) dont le sens exact n'était pas
  détaillé.
- **Table des 9 compagnons recrutables** (p.6) avec qualité dominante et
  talent : mage/intelligence/magie, guerrier/force/lutte,
  sorcier/vitalité/incantation, marchande/charme/négociation,
  prêtre/chance/exorcisme, vestale/volonté/pouvoir PSY,
  pilote/endurance/astronaute, aventurier/agilité/système D,
  cyborg/habileté/tir. Pas encore exploité dans le code (pas de système
  de recrutement de PNJ implémenté), mais utile le jour où `game.py`
  gérera plus que le personnage principal.
- **Liste des commandes confirmée** (p.8), cohérente avec `VERBES` déjà
  extrait — rien de nouveau ici, bonne validation croisée.

## 19. Amarande, Alfol, Meduz, la Pyramide — structure et les 4 tâches enfin clarifiées

Contrairement au système de combat, ces lieux ont leur `.PAS` décompilé
disponible depuis le début (`OVL_00FB`, `OVL_0085`, `OVL_00A4`,
`OVL_0142`) — cette section vient d'une lecture attentive de ce code
déjà présent, pas d'un nouveau désassemblage.

### 19.1 Amarande (`OVL_00FB`)

- Grille **30×30 = 900 cases par hémisphère**, deux hémisphères
  rebouclés en tore l'un dans l'autre (position `1000`-`1899`,
  rebouclage `±900` aux bornes) — confirme et précise section 5.1.
- Indexation column-major confirmée avec les touches réelles : `8`/`2`
  (pavé numérique nord/sud) déplacent de `±1` ; `6`/`4` (est/ouest)
  déplacent de `±30`.
- Déplacement bloqué par les chaînes de montagnes :
  `Sav.f_PosHMountains[1..9]` bloquent les `±1` (N/S),
  `Sav.f_PosVMountains[1..25]` bloquent les `±30` (E/O) — confirme les
  tailles de tableaux déjà supposées section 5.1.
- **20 villes** : `Sav.f_LocTowns[0..19]`. Les index 0-9 semblent gérés
  directement dans cet overlay (les scènes de décor modulo-23 ne se
  dessinent que si on n'est PAS sur une de ces cases, ou si l'index de
  ville est `>9`) ; les index `>9` passent par `C_7309`, un
  dispatcher d'entrée en ville séparé, non décodé ici.
- **9 "lieux extra"** : `Sav.f_LocExtra[1..9]`. Déclenchent un sprite de
  rencontre aléatoire (1 parmi 3) quand on tombe dessus, **sauf les
  index 3, 5, 8** — vraisemblablement les destinations fixes du
  scénario (Cité des Crânes, Pyramide, Temple de Seth...), qui n'ont pas
  besoin de rencontre aléatoire puisque ce sont des destinations
  volontaires. Correspondance exacte index↔lieu non établie.
- Décor procédural par `Position MOD 23` déjà documenté section 5.1,
  confirmé avec les valeurs de cas réelles cette fois (détail de rendu,
  hors de portée pour le remake qui modernise cette partie).

### 19.2 Alfol (`OVL_0085`)

- **Surface** : grille 10×10 (position 0-99), même convention de touches
  N/S/E/O que partout, rebouclage aux bords. La position `0` est un
  écran spécial (proche du spatioport). Une position précise
  (`Sav._75D0`) est l'entrée de la base intérieure — taper `E`(ntrer)
  là, après confirmation, y fait entrer (`INNER_CCBE`).
- **Base intérieure** (`INNER_CCBE`/`INNER_CAFC`) : véritable donjon
  couloir, `WorldId := 9`, cases 900-899, **même modèle de mur par case**
  (`Sav.f_Maps[pos][1..4]`) que la Cité des Crânes — confirme que ce
  n'est pas juste un écran spécial mais un vrai labyrinthe.
  - Marqueur `Sav.f_Maps[pos][5] = 1` : **"BUREAU DES MÉDIAS"** — propose
    d'investir 1000 ducats ; accepté et payé → **tâche 2 accomplie**
    (`Sav.f_IsTaskDone[1] := TRUE`).
  - Marqueur `[5] = 2` : dépôt de produits radioactifs — pas une tâche,
    un piège d'ambiance qui retire 1 PV à chaque personnage présent.
  - `Sav.f_Maps[pos][6] <> 0` : rencontre (monstres 12-14).

### 19.3 Meduz (`OVL_00A4`) — accessible via la Pyramide

- Lui aussi un vrai donjon couloir (mur par case), `WorldId := 10`,
  cases 1000-999. **Point non résolu** : ceci semble contredire
  l'attribution "monde 10 = Alfol, base intérieure" établie section 5.3
  depuis l'analyse de `OVL_0000` — à moins que les deux lieux ne
  réutilisent la même tranche de `f_Maps` à des moments différents de
  la partie (Meduz regénéré à la volée à chaque visite, cohérent avec
  l'idée d'un "monde parallèle étrange" instable évoquée par le manuel).
  Pas tranché, à revérifier si besoin en repassant par `OVL_0000`.
- Accès : depuis la Pyramide (19.4), commande `M`(onter), mot demandé
  comparé à `Sav.f_SecretWords[2]` — bonne réponse → "chute dans le
  temps.." → transport vers Meduz avec animation de zoom (`OVL_0145`)
  à l'aller et au retour.
- Marqueur `[5] = 2` : "méditer" (touche `M`) — la première fois,
  **+1 Volonté et +1 Intelligence pour tout le groupe** (flag
  `Sav._9418`, ne se répète pas).
- Marqueur `[5] = 3` : "invoquer" (touche `I`) — nom demandé, comparé à
  `Sav.f_SecretWords[3]` ; bonne réponse ET méditation déjà faite →
  **tâche 1 accomplie** (`Sav.f_IsTaskDone[0] := TRUE`).
- Sortir de Meduz fait avancer `Sav.f_Days` de 7 (une semaine "perdue").

### 19.4 La Pyramide — portail vers Meduz (`TERA.PAS`, dispatch lieu = 3)

Lieu directement accessible depuis Amarande, pas un donjon en soi :
juste la commande `M`(onter) qui, avec le bon mot secret
(`Sav.f_SecretWords[2]`), ouvre le passage vers Meduz (19.3).

### 19.5 Les 4 tâches — déclencheurs désormais tous confirmés

| Tâche (index) | Lieu | Déclencheur confirmé |
|---|---|---|
| 1 (`tasks_done[0]`) | Meduz | Méditer (`[5]=2`) puis invoquer (`[5]=3`) le nom `Sav.f_SecretWords[3]` |
| 2 (`tasks_done[1]`) | Base d'Alfol | Payer 1000 ducats au "Bureau des Médias" (`[5]=1`) |
| 3 (`tasks_done[2]`) | Cité des Crânes, monde 7 | Vaincre Arioch (déjà confirmé section 5.3) |
| 4 (`tasks_done[3]`) | Temple de Seth, Autel des Ténèbres | Invoquer et vaincre le "copain d'Arioch" avec `Sav.f_SecretWords[4]` (déjà confirmé section 5.4) |

### 19.6 Rôle des 4 mots secrets (`Sav.f_SecretWords`, indexation 1-based confirmée)

| Indice | Rôle confirmé |
|---|---|
| `[1]` | Cité dans un indice (`Sav.f_HintStr[$21]`) : *"pour LE vaincre : crier `<mot>`"* — probablement lié à l'action de combat "crier" (`action_type` 9) contre un adversaire précis ; cible exacte non confirmée |
| `[2]` | Mot de passe pour monter à la Pyramide et accéder à Meduz |
| `[3]` | Nom à invoquer sur Meduz pour la tâche 1 |
| `[4]` | Nom du "copain d'Arioch" à invoquer sur l'Autel des Ténèbres pour la tâche 4 |

Ces découvertes ferment un des plus gros trous documentés du projet :
on sait maintenant précisément **quoi** coder pour rendre les 4 tâches
jouables, pas seulement qu'elles existent quelque part.

## 20. Initialisation de partie (`OVL_0010`) — montagnes, villes, PNJ, mots secrets

`OVL_0010.PAS` (commentaire `{INIT CONSTANTS?}` dans `TERA.PAS`) s'avère
être la routine complète d'initialisation d'une nouvelle partie —
beaucoup plus riche que son nom ne le laissait supposer. Lu en entier
cette session (aucun désassemblage nécessaire, le `.PAS` était déjà là).

### 20.1 Les 9 compétences = les 9 stats, confirmation définitive

`Sav.f_SkillName[1..9]` nomme les compétences, et elles correspondent
**exactement, dans le même ordre**, aux 9 stats du personnage (section
13/17) :

| Indice | Stat | Compétence nommée | Talent du PNJ correspondant (manuel) |
|---|---|---|---|
| 1 | Force | lutte | guerrier |
| 2 | Intelligence | magie | mage |
| 3 | Chance | exorcisme | prêtre |
| 4 | Endurance | astronaute | pilote |
| 5 | Volonté | pouvoir PSY | vestale |
| 6 | Vitalité | incantation | sorcier |
| 7 | Agilité | système D | aventurier |
| 8 | Charme | négociation | marchande |
| 9 | Habileté | tir | cyborg |

Ça confirme définitivement que `Character.skills` doit être indexé par
nom de stat (déjà corrigé dans `combat.py` section 17) — ce n'est pas
une supposition, c'est écrit noir sur blanc dans le générateur de
partie. Ça résout aussi le mystère de l'`action_type` 7 (Endurance+
Agilité) sans verbe connu (section 17) : ce n'est pas un verbe de
combat, c'est le talent passif **"astronaute"/"système D"** du
pilote/aventurier — probablement utilisé pour le pilotage d'astronef
(section 8, manuel) plutôt que pour une action de combat classique.

### 20.2 Génération des 9 compagnons potentiels

- Chaque PNJ (index 1-9) a son nom tiré aléatoirement (H/F) dans
  `NOMPERS`, un état initial `'OK'`.
- Toutes les stats sont `2 + RANDOM(6)` (2 à 7) **sauf** leur stat de
  spécialité (`Stats[i][i] := 9`) et leur compétence de spécialité
  (`Skills[i][i] := 3`) — chaque PNJ est donc mécaniquement très typé,
  comme le dit le manuel (page 6, "chacun possède un talent
  particulier").
- Cinq PNJ ont une **spécialité secondaire** forcée à 7 + compétence 1 :
  prêtre (Volonté), sorcier (Intelligence), marchande (Volonté),
  aventurier (Habileté), pilote (Habileté).
- Équipement de départ fixe pour 4 d'entre eux (indices OBJETS) :
  PNJ1 = épée+cuirasse ; PNJ4 et PNJ7 (pilote/aventurier) = cuir+revolver ;
  PNJ9 (cyborg) = armure spatiale+désintégrateur — cohérent avec l'objet
  fixe de l'action 2 (`tirer`→désintégrateur, section 17).
- Seul le personnage créé par le joueur (index 0) commence dans le
  groupe (`Sav.f_IsInParty[0] := TRUE`) — confirme le manuel ("le
  personnage créé par le joueur est seul").

### 20.3 Placement des PNJ et indices (`Sav.f_HintStr`)

Chaque PNJ recrutable est placé à un endroit tiré au sort parmi 2
options, et un indice textuel correspondant est généré pour que le
joueur puisse le retrouver :

- PNJ 1, 2, 6 : quelque part **dans la Cité des Crânes** (étage 0-1,
  via `INNER_BA2F` qui cherche une case valide sans monstre) — indice
  "sous"/"dans la cité des crânes".
- PNJ 8 : à Balgence **ou** à la Porte des sables (2 des 5 lieux nommés
  trouvés dans `MESSAGE`, section 12.4).
- PNJ 5 : dans le Temple de Seth (position fixe `800`) **ou** à la
  Roche aux Goules.
- PNJ 3 : à Mortefontaine **ou** à Mahalore.
- PNJ 4 : toujours à `LocTowns[1]`.
- PNJ 7 et 9 : quelque part dans la **base d'Alfol** (`INNER_BA2F(9)`,
  confirme monde 9 = Alfol base, voir correction section 5.3).

### 20.4 Génération d'Amarande — montagnes et villes (algorithme exact)

- **`PosHMountains[1..9]`** (bloquent N/S) : **une seule chaîne
  contiguë de 9 cases** dans la même colonne — un point de départ
  aléatoire (`1000 + RANDOM(255)`), puis chaque case suivante = la
  précédente `+ 30` (colonne suivante... non, `+30` = même colonne
  ligne suivante vu l'indexation column-major, donc bien une chaîne
  verticale continue de 9 cases).
- **`PosVMountains[1..25]`** (bloquent E/O) : **5 chaînes contiguës de 5
  cases chacune**, chacune démarrant à un point aléatoire
  (`1000 + 3×RANDOM(255)`), les 4 cases suivantes = `+1` (case
  suivante dans la même ligne).
- **20 villes** (`LocTowns[0..19]`) : positions aléatoires
  `1000 + 4×RANDOM(225)`, retirage tant qu'il y a doublon.
- Deux positions spéciales `Sav._75DE`/`Sav._75E0` générées aussi ici,
  rôle exact non tranché (candidates : Kronopolis et un second point
  de repère sur la carte).

### 20.5 Les 4 mots secrets — algorithme de génération exact

Chaque mot secret est la concaténation de 3 syllabes tirées dans des
listes fixes (fantaisie à consonance mystique) :

```pascal
Syllabes1 := ['EL','A','HA'];                              { 1er bloc, 1-3 }
Syllabes2 := ['KHY','MA','KHA','RAH','THY','TOH'];          { 2e bloc, 4-9 }
Syllabes3 := ['HAM','HAAR','LORK','RMA','LU'];              { 3e bloc, 10-14 }
FOR i := 1 TO 4
DO SecretWords[i] := Syllabes1[RANDOM(3)] + Syllabes2[RANDOM(6)] + Syllabes3[RANDOM(5)];
```

Entièrement reproductible — aucune donnée externe nécessaire, juste ces
3 listes et `RANDOM`.

### 20.6 Marqueurs de la base d'Alfol (monde 9) — génération confirmée

```pascal
FOR pos := 900 TO 999
DO IF (pos <> LocChars[7]) AND (pos <> LocChars[9]) AND (Maps[pos][5] <> 1)
THEN IF RANDOM(14 - Difficulty) = 0
THEN Maps[pos][5] := 2;  { depot radioactif }

FOR pos := 900 TO 999
DO IF (...) AND (Maps[pos][5] <> 1)
THEN IF (RANDOM(12 - Difficulty) = 0) AND (Maps[pos][5] <> 2)
THEN Maps[pos][6] := RANDOM(3) + 12;  { monstre, id 12-14 }
```

**Point non résolu** : ce passage place le marqueur `2` (radioactif,
piège d'ambiance) et les rencontres, mais **pas** le marqueur `1`
("BUREAU DES MÉDIAS", section 19.2) — sa position exacte de placement
n'a pas été retrouvée dans ce fichier ; probablement dans la passe de
contenu générique `INNER_BD96` de `OVL_0000` (non vérifiée pour les
mondes 9/10 spécifiquement, section 5.3 ne couvrant que 0-8).

## 21. Les 23 scènes fixes d'Amarande (`Sav.f_Position MOD 23`) — table complète

Suite à une capture d'écran du vrai `TERA.COM` fournie par Merci (scène
nocturne avec arbres, silhouette rocheuse et ligne d'horizon magenta), la
fin d'`OVL_00FB.PAS` (procédure `INNER_C377`) a été relue en détail : le
"décor procédural par `Position MOD 23`" déjà mentionné section 5.1/19.1
n'est **pas** un générateur aléatoire mais une **table de 23 scènes
fixes**, chacune une combinaison exacte d'appels `PutSprite_off`/
`FillRect_off` à des coordonnées codées en dur. Portée telle quelle dans
`src/tera/render/amarande_scenes.py` (table `SCENES`) et interprétée par
`render/scene.py::draw_amarande_scene` — plus une modernisation
approximative comme le laissait entendre section 5.1/19.1.

### 21.1 Identification des sprites — correction importante

En recoupant l'usage réel dans `OVL_00FB.PAS` avec une inspection
pixel-par-pixel des PNG extraits, plusieurs identifications d'une session
précédente se sont révélées **fausses** : `SPR_44`/`SPR_45` avaient été
étiquetés "rocher" par simple inspection visuelle rapide (une forme
vaguement massive) ; en réalité ce sont des **arbres** (tronc + frondaison
confirmés au zoom). Table corrigée (voir aussi `render/sprites.py`) :

| Sprite | Rôle confirmé | Sprite | Rôle confirmé |
|---|---|---|---|
| `SPR_01` | grand arbre orné | `SPR_2C`/`SPR_3A` | rocher/monticule bas |
| `SPR_14` | petit caillou (sol) | `SPR_35`/`SPR_36` | touffe d'herbe (sol) |
| `SPR_20`/`SPR_21` | petit arbre | `SPR_39` | silhouette rocheuse large |
| `SPR_22` | arbre moyen | `SPR_3B` | tige/plante haute (magenta) |
| `SPR_27`/`SPR_28`/`SPR_29` | créature/carcasse au sol (mâchoire visible sur `SPR_29`) | `SPR_44`/`SPR_45` | arbre (corrigé, anciennement "rocher") |
| `SPR_2A`/`SPR_2B` | pic rocheux/montagne | `SPR_47` | grand arbre "signature" (branches noueuses magenta) |
| | | `SPR_4F`/`SPR_50` | colline basse et large |
| | | `SPR_52` | pic enneigé (placé haut dans le ciel) |

### 21.2 Ligne d'horizon (`INNER_BD0C`)

Élément séparé des sprites : 16 étoiles aléatoires (`PLOT` blanc, bande
haute de l'écran) + une silhouette d'horizon en **polyligne magenta**
(couleur CGA 2), tirée aléatoirement parmi deux profils (`INNER_BA2F`/
`INNER_BBA3`), chacun en réalité **deux polylignes disjointes** (pas une
seule chaîne continue — la 2e commence avant la fin de la 1ère en x).
Hauteur aléatoire (`RANDOM(20)`) à chaque affichage. Un `Circle`/
`FillShape` conditionnel (`IF D_A679='6'`, donc seulement après un
déplacement vers l'est) n'a pas été porté — effet visuel mineur et
incertain (peut-être un soleil/une lune), hors de portée.

### 21.3 Ce qui reste non confirmé

- **`DrawMountains`** (silhouette "proche" affichée en plus quand une
  vraie chaîne de montagnes est adjacente) est une primitive résidente
  jamais retrouvée (section 15) — `scene.py` la remplace par une
  silhouette vectorielle approximative, inchangée par ce port.
- **Ancre exacte de `PutSprite_off(x,y,sprite)`** (coin haut-gauche
  supposé, non confirmé par désassemblage de la primitive elle-même).
- La **surface d'Alfol** (`OVL_0085.PAS`) n'a pas été dépouillée au même
  niveau de détail — reste sur l'ancienne modernisation (décor
  pseudo-aléatoire seedé par position) en attendant une passe similaire.

## Annexe -- listes completes extraites des fichiers de donnees

### Verbes (VERBES, 56 entrees)

1. abandonner
2. boire
3. combattre
4. descendre
5. entrer
6. fuir
7. halte
8. lire
9. monter
10. négocier
11. ouvrir
12. prendre
13. quitter et sauver
14. séduire
15. utiliser
16. dormir
17. manger
18. in(tro)spection
19. sonder
20. consulter la carte
21. interroger
22. acheter
23. vendre
24. donner
25. solliciter
26. tirer
27. lutter
28. jeter un sort
29. dissiper
30. invoquer
31. attaque mentale
32. psalmodier
33. crier :
34.  ducats
35.  clé n°
36.  et touche !
37.  et manque..
38. sans effet..
39. arrèter la partie
40. % evolution
41. arbitrary unit
42.  prend l'objet
43. abandonné..
44. qui donne (A-D) ? 
45. quoi (0-9) ? 
46. çà va bien oui ?!
47. remettre le son
48. couper le son
49. bon, je viens
50. embarquer
51.  aux commandes
52. de KRONOPOLIS
53. c'est bon !
54. tu trouves 
55. il contient un(e) 
56.  c'est fait

### Objets (OBJETS, 25 entrees)

1. cristal
2. diamant
3. gemme de feu
4. talisman
5. talisman de xur
6. orbe du temps
7. anneau
8. anneau d'argent
9. anneau de mort
10. pierre
11. pierre de lune
12. pierre-étoile
13. épée
14. hache
15. épée runique
16. arc
17. revolver
18. désintégrateur
19. cuir
20. cuirasse
21. armure spatiale
22. amulette
23. élixir
24. contrepoison
25. couteau

### Monstres (NOMON, 22 entrees)

1. un cobra
2. un naja royal
3. de monstrueux cafards
4. un caiman
5. un crocodile géant
6. des mygales
7. une tarentule
8. deux guerriers de Kwalish
9. des insectes venimeux
10. un seigneur du CHAOS
11. des guerriers de Kwalish
12. un homme de Merenen
13. un cyborg-laser
14. les gardes de Merenen
15. une chauve-souris vampire
16. des ESPRITS incarnés
17. la guilde des nécromanciens !
18. une créature étrange
19. un ESPRIT vampiresque
20. un demi-dieu déchu
21. un prince-démon
22. des cristaux télépathes

### PNJ recrutables (NOMPERS, 27 noms -- 9 personnages x 3 tableaux)

1. CROM
2. KALEB
3. SADOZ
4. de KERLAS
5. YMIR
6. SUN RAA
7. COBURN
8. THELIA
9. X007
10. WHARL
11. HIM
12. TAAR
13. van DUNOR
14. KIRA
15. L'METH
16. Lt RAMBOW
17. AKILONIA
18. MEGA
19. ELRIC
20. TI'SIEN
21. ZAAR
22. von KLUTZ
23. SONJA
24. TORN
25. MATH MAX
26. SHANGALI
27. WULL
