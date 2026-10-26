# Étapes 6/7 — combat, temps et haut-parleur

Sources : BIN C8790/C8CE3 (menu et nom du combattant en ligne 25),
C95D9 (annonce puis DELAY(1000)), C8B5E (500 ms avant le résultat,
700 ms sans son si touche, 800 ms si manque), C92CE (400+400 ms),
C9B36 (1500 ms victoire sans son), C4503 (440 Hz,600 ms),
C6121/C6179 (balayages), C65B5 (bruit), C6637, C6603, C6976,
C6A45, C6518/C62B7 ; DATA NOMON, VERBES, MESSAGE/2/3, SPRITES.

CONFIRMÉ : huit actions T/L/J/D/A/P/C/F ; confirmation par O/Entrée,
deux essais, puis passage ; C lit dix lettres et en conserve neuf.
Le frontend ne déduit ni touche, ni dégâts, ni condition, ni récompense.
Les instantanés d'événements permettent de montrer les résultats dans leur
ordre, même quand le moteur atteint le prochain lecteur dans le même appel.
Les délais sont des événements de présentation non bloquants côté SDL.

CONFIRMÉ : les tirages sonores existants doivent rester au même endroit.
La synthèse reçoit les fréquences et durées déjà déterminées par le C.
MODERNISABLE : onde carrée PC speaker transmise à un flux SDL mono.
À VÉRIFIER : timbre acoustique exact et durée des boucles sans DELAY,
dépendantes du processeur DOS ; celles-ci seront explicitement normalisées
à un échantillon par écriture de fréquence, sans les annoncer comme ISO.

Comparatif graphique : positions/sprites C9B4E, transparence par octets
C5051. Les silhouettes procédurales complexes doivent rester signalées si
incomplètes ; aucune silhouette approchante ne remplace un monstre inconnu.
Fonte DOS/palette matérielle restent à vérifier. Les tests ne constituent
pas une comparaison de captures DOS exécutées.

## Extension : figures et ambiances (16 octobre)

CONFIRMÉ BIN : C73ED relie trois chaînes (points 1..9, 10..16,
17..24), un segment transverse et huit détails. C76D2 relie 39 points
et ajoute deux segments. C77F0 compose C76D2(32,0,1), effacements et
sprites ; C7B74 compose effacement, C73ED(x,106,1), huit segments de
capuche et S19(28+x DIV 4,96). Le type 17 appelle cette composition
pour x=0 puis 84, puis S1A(49,96). Aucun tirage supplémentaire.

CONFIRMÉ BIN/PAS : C62B7 émet duration-1 notes avec RANDOM20,
éventuellement RANDOM2 puis RANDOM255/256, et DELAY explicite. C6352
enchaîne sept parties et sept pauses de 200 ms ; C6496 et C6518 trois
parties de 59/29/119 notes de 15 ms. TERAMAIN fixe les paramètres des
villes 0, 6, 8 et 9 ; C9B4E ceux du bruit des monstres 3, 6 et 9.
Ces sorties remplacent leurs tirages jetés, avec la même graine finale.
Les traits passent par les primitives SDL existantes ; l'identité pixel
avec l'algorithme de ligne CGA reste à distinguer de la composition.

CONFIRMÉ BIN/PAS : OVL00FB choisit les rapports 1/1, 4/3 ou 3/2 pour
l'ambiance de marche ; OVL0085 utilise C62B7(30,110,50,10,10,8500,11000)
au dépôt radioactif, avec attente de 1000 ms sans son. OVL002E enchaîne
C6496(1,1), rafale 400, attente 120 ms, rafale 600, attente 180 ms,
C6496(9,8) à l'entrée Position=1.

CONFIRMÉ BIN : C6221(base,delay,count) émet, pour n=count..1,
TRUNC(base*(100+80*COS(0.2*n))/100), puis DELAY(delay). Les trois appels
utilisés (rayon, pirates, temples) concordent intégralement avec le calcul
Real48 du COM ; le C utilise double, substitution MODERNISABLE pour ces
paramètres vérifiés. C666B/C6721 parcourent 50..1200 / 1200..50 inclus,
quatre délais de 1 ms par pas, avec fréquences divisées seulement au-dessus
de 600, 400 et 300. OVL00AE conserve cet ordre : étoiles, saut (ou 2000 ms),
explosions, étoiles, annonce des pirates (ou 3000 ms), tirs ; à l'arrivée
1000 ms, saut, explosions/étoiles, énergie puis C6721 si son actif.
Le rayon sonore ne s'émet qu'avec une touche 1..9 ; sans son, attendre 30 ms.
TERAMAIN raccorde aussi C666B/C6721 au spatioport et C6221 aux temples.
La visualisation du rayon est raccordée par le [bloc scènes](spec_scene_completion_c.md).

Validation de cette extension : les 37 suites passent, avec 11 empreintes
de partitions calculées à partir des SOUND/DELAY originaux exécutés, et
comparaison des trois figures sur fonds noir/cyan aux traces originales.
Les vecteurs C sont embarqués dans les tests ; l'audit Python/Unicorn est
optionnel et produit `build/audit/media_binary_trace.json`. L'audit séparé
des façades vérifie 16 cas, sans écart. Le moteur sans SDL passe aussi les
empreintes audio en compilation 32 bits.
L'audit spatial supplémentaire passe 173 scénarios et 120 sélections de
pilote, 1 369 frontières clavier au total, sans écart de règles/RNG.
Cet audit abstrait les effets déterministes, contrôlés séparément par les
empreintes audio, et ne certifie pas un parcours humain complet.

## Comparatif livré

| Élément original | Restitution C/SDL | Statut |
| --- | --- | --- |
| C4D46 : noms, PV/max, PM/max, états, colonnes 1/13/26/37 | Valeurs du combat vivant ou de son instantané, aucun recalcul SDL | CONFIRMÉ BIN |
| C8CE3 : nom du personnage suivi de ` -> ` ; C81D7 : verbe + ` ? ` | Ligne 25, huit commandes, annulation et deuxième essai conservés | CONFIRMÉ BIN/DATA |
| Cri : dix lettres, stockage de neuf | Saisie affichée, mot attendu jamais transmis au renderer | CONFIRMÉ BIN |
| Phrases et objet souligné | MESSAGE/2/3 et VERBES, ID/variante choisis dans le core ; sans effet distinct d'un manque | CONFIRMÉ BIN/DATA |
| C9B4E : sprites des 22 types | Associations et positions relevées ; type 22 ne dessine rien dans ce CASE | CONFIRMÉ pour les sprites repris |
| Types 16/17/19 : compléments procéduraux C76D2/C7B74 | Contours, effacements et sprites intégrés, placeholders retirés ; comparaison aux primitives BIN | CONFIRMÉ pour la composition, rasterisation SDL À VÉRIFIER |
| Types 1/2/3/6/9/21 : coordonnées aléatoires | Points capturés pendant les tirages existants, jamais retirés dans SDL | CONFIRMÉ BIN, tests de graine |
| C7868 : monstre 15 | Image, attente 700+RANDOM255 ms, son 20 ms, test clavier après image, jusqu'à six | CONFIRMÉ BIN ; smoke SDL réel avec interruption de la première image |
| Combat : annonce, résultat, flash, victoire | Chronologie d'instantanés + délais mesurés par horloge SDL | CONFIRMÉ pour les délais relevés |
| C6603/C65B5/C6637/C6976/C6A45/C6518 | Tir, lutte, touche, cri, psalmodie, victoire : score de fréquences/durées | CONFIRMÉ BIN ; graine et nombres de notes testés |
| C67D7/C68AD/C693B | Pas, alertes, grondements de monstres | CONFIRMÉ BIN ; boucle sans DELAY normalisée |
| Explosions spatiales C6D32 | Huit palettes/bordures ; 60 ms sans son, 16 notes de 2 ms avec son ; graine conservée | CONFIRMÉ BIN pour paramètres ; apparence de la bordure MODERNISABLE |
| Chronologie du tir spatial | Échéances/purges, rayon double temporisé, son et attentes raccordés ; nouveau tir après effets | CONFIRMÉ pour les coordonnées/durées ; parcours humain À VÉRIFIER |
| Ambiances, sept lieux, dépôt Alfol et entrée du souterrain | C6352/C6496/C6518/C62B7/C6221/C666B/C6721 raccordés avec paramètres PAS/BIN | CONFIRMÉ pour ces appels ; sons de progression complétés : voir spec_progression_sound_c.md |
| Décollage, pirates, saut de retour et atterrissage | Notes/délais originaux, branches silencieuses et RNG conservés | CONFIRMÉ pour les appels sonores ; acoustique matérielle MODERNISABLE |
| Fonte, palette physique, acoustique du PC speaker | Fonte libre, quatre palettes CGA, onde carrée mono 48 kHz | À VÉRIFIER visuellement/acoustiquement |

Le contrôle `#` puis O reste le contrôle sonore du moteur. Il n'est pas
remplacé par un réglage SDL indépendant. Aucun fichier musical ajouté.
L'absence de périphérique audio conserve les délais en silence.

## Architecture et validation

`engine/src/sound.c` remplace les tirages sonores jetés par les mêmes
tirages et des lots de 64 notes au maximum. Les nouveaux événements sont
par valeur : TONES, WAIT, FLASH et MONSTER. Les événements COMBAT portent
les PV/PM/états, le type, l'éligibilité et l'objet du résultat observé.
Rien de cela n'est sérialisé ; la sauvegarde garde son format précédent.
Le journal logique END reste émis après les tirages de victoire.

`frontend/sdl3/media.c` construit un flux PCM et une chronologie à partir
de ces événements. Une touche pendant un effet ne devient pas une commande
future ; la purge à la frontière de lecture reste celle du core. La fenêtre
continue à se dessiner/se fermer et le plein écran reste disponible.
Le service FRAME du monstre 15 présente les durées déjà calculées, traite
la fermeture et sonde la touche sans la consommer ; il ne rappelle pas le
moteur. Le clavier est purgé avant le lecteur du combat.

Validation native : **37/37 suites CTest**. Les nouveaux tests contrôlent
les nombres de tirages, les fréquences, les durées, les 22 rendus, les huit
confirmations, les instantanés sans mutation, les flashes, un flux audio
SDL dummy et l'horloge virtuelle. Le smoke du paquet autonome exerce aussi
une vraie attente SDL et l'interruption du monstre 15 après trois tirages.
Les deux campagnes complètes gardent leurs digests et compteurs précédents.

Captures : `build/sdl3/battle-*.bmp`. Échantillon du double avertissement
C4503 : `build/sdl3/combat-original-score.wav`. Ces captures et ce fichier
sonore dérivé restent locaux, exclus de Git.

Comparaison différentielle du 16 octobre avec les routines COM exécutées
dans le banc d'audit existant : **946 scénarios, 944 terminés, 6 836
frontières clavier, zéro écart**. Les 770 vecteurs RNG passent ; les fuites
comprennent 1 932 candidats rejetés. Les cas incluent les 22 types et les
sept attaques avec son actif. Rapport local :
`build/audit/interactive_comparison.json`, SHA256 COM
`22fb836aa93f46dc2437e8b9586415d13552bfac35e8ac6647ba6157fd9b1a37`.
Ce banc exécute les instructions de règles/RNG originales avec clavier
injecté ; vidéo/sons/délais DOS y sont abstraits. Il ne certifie donc pas
une comparaison visuelle ou acoustique. Le banc d'audit utilise Python et
Unicorn uniquement en développement ; le jeu et les 37 tests C/SDL restent
autonomes. Trois contrats ciblés passent aussi dans la compilation sans SDL.
