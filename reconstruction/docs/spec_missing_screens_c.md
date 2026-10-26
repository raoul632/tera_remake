# Écrans et séquences restants

Sources : OVL_00E2.PAS (aide), OVL_00CB.PAS (fins), OVL_0069.PAS
(inscriptions de Seth), OVL_0145.PAS (tunnels) ; TERA.COM C5495 et
CA709 (cartes et Halte). Ces fichiers historiques restent en lecture seule.

CONFIRMÉ : aide sur ?/+, grille des commandes et traits ; carte des mondes
0..10 de 10 × 10 cases avec couleurs des murs, carte locale Amarande de
10 × 30 positions, carte Alfol, contrôle de possession ; inscriptions 100/101 ;
fenêtre bleue de fin (80 colonnes), texte et score fournis par le moteur ;
sept tunnels, 24 étoiles par tunnel, cercles et effacements, palette et fond,
ordre et bornes de tous les tirages. Les mélodies finales remplacent leurs
tirages déjà présents, sans nouvelle consommation aléatoire.

La première version utilisait 16 ms par image et une police Latin-1 de
remplacement. Elle est remplacée par le [profil IBM PC/XT](spec_visual_xt_c.md) :
tables BIOS/CGA CP437 complètes, proportions du texte à 80 colonnes,
raster entier GRAPH et coût variable des cercles à 4,77 MHz.

Contrats : moteur sans SDL, événements par valeur, aucune règle recalculée
dans le rendu, pas de nouvelle dépendance aux COM/PAS à l'exécution ; les
tirages graphiques sont capturés lors de leur consommation existante.
La consultation ne consomme ni jour ni tirage supplémentaire. Les captures
SDL doivent couvrir aide, cartes, inscriptions, mort, victoire et tunnel ;
les tests vérifient aussi la sortie de chaque lecture et le silence.

## Validation et limites du comparatif

Les tests SDL capturent aide dans quatre contextes, trois familles de carte,
histogramme, Seth, décès/successeur et victoire/acquittement. Ils comparent
l'état et les sauvegardes avec une session sans rendu. Le refus de carte
sans possession est testé, ainsi que la pause des cartes intérieures en
faible difficulté. Les anciennes assertions qui traitaient ? comme touche
inconnue sont remplacées par Z.

L'oracle `tools/audit_tunnel_binary.py` exécute OVL0145 sans remplacer ses
calculs ; 245 images natives de la graine 1986 vérifient étoiles, centres,
rayons, couleurs, palettes, fonds et effacements dans le test C. Le test SDL
vérifie durée, silence, audio et sélection de l'image sur horloge virtuelle.

CONFIRMÉ : C5495 efface A744 à 5B80 pour chaque case. L'ancien doute sur son
résidu était infondé. Le pilote natif est maintenant exécuté jusqu'aux
écritures CGA : 44 cartes et les 245 images de tunnel correspondent pixel à
pixel aux fixtures. Les glyphes correspondent aux tables IBM de référence.
La cadence XT est un modèle nominal documenté, avec ses limites matérielles.

Validation finale : 37/37 tests C/SDL passent, dont le paquet autonome.
Comparatifs BIN/C relancés : fin 510 cas, Meduz 326 + 48 voyages,
Amarande 311 cas ; total 1 195, aucun écart de règles/RNG.
Les sorties vidéo abstraites de ces comparatifs ne certifient pas le rendu ;
l'oracle des 245 images OVL0145 et les captures SDL sont distincts.
L'écran de fin est un repère dans la timeline : il apparaît après les effets
précédents du combat et au début de ses mélodies, pas dès la mutation du core.
