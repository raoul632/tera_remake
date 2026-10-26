# Comparatif final des blocs graphiques

17 octobre 2026. Objectif : comparer le framebuffer CGA du programme original
aux pixels réellement relus après rendu SDL, puis corriger les écarts confirmés.
Ce document remplace les réserves générales sur les lignes, les silhouettes,
les portes et les remplissages des précédents jalons pour les cas ci-dessous.

## Méthode et provenance

CONFIRMÉ BIN/DATA : `tools/audit_final_raster.py` exécute les instructions
du pilote GRAPH, de C4544, des blitters et des routines de composition.
Les primitives ne sont pas remplacées par celles du remake. Les empreintes
FNV portent sur les 64000 indices extraits de la vraie mémoire B800.
Les tests C rendent ensuite avec les fonctions de production SDL et comparent
les mêmes empreintes. Des buffers locaux sont conservés en cas d'écart.

COM SHA256 : `22fb836aa93f46dc2437e8b9586415d13552bfac35e8ac6647ba6157fd9b1a37`.
Overlay SHA256 : `539d95781f9296d40ecd05698eb98255835a45cb03e60b49cf581e3ada7038f4`.
Les originaux restent lus uniquement depuis le dépôt parent. L'outil Python
est optionnel : les vecteurs C permettent les contrôles sans émulateur.

Abstractions : chargement des ressources dans les buffers DOS, sorties texte
et clavier, arrêt avant les règles du combat/jour (comparées par leurs bancs
propres). Les écritures du monolithe sont isolées de sa géométrie. Les scènes
sont comparées en indices ; les palettes sont contrôlées séparément.

## Écarts corrigés

- C4544/C46BC excluent x2 ; les deux lignes de la dernière paire sont incluses.
- GRAPH 3BCD conserve le point de départ pour les égalités de pente. Les
  lignes SDL ordinaires ne donnaient pas tous les mêmes pixels.
- C5051/C5E04 reçoivent des coordonnées BYTE. Le débordement horizontal suit
  les adresses CGA et reprend sur les paires de lignes suivantes.
- Un Y impair et certaines hauteurs de sprites lisent une paire supplémentaire
  dans les plans contigus. Les textures conservent maintenant ces octets.
- C60BB teste l'octet entier du calque : les noirs d'un octet mixte écrasent
  aussi le fond. La fusion ne doit pas tester chaque pixel séparément.
- Les arcs de porte suivent l'arrondi Newton propre à Arc, différent de Circle.
  Les portes et le monolithe utilisent un remplissage borné par le contour.
- Mahalore retrouve ses flèches, rochers et l'ordre original des calques.
  Les bases et chapiteaux des temples suivent les rectangles d'octets CGA.
- Les intérieurs retrouvent les deux sprites manquants de la première
  perspective, les traits exacts des portes et leurs remplissages.
- Les ruines respectent l'ordre côtés/décor/fond, leurs buissons fixes et
  l'association droite/gauche des points aléatoires face à l'ouest (-10).
- Les effacements et diagonales des fosses de Méduz suivent les coordonnées
  distinctes des deux côtés ; ils ne sont pas une simple symétrie en 319-x.
- Le viewport est réinitialisé lors du changement de cible/mode. Les contours
  utilisent des pixels logiques complets aussi avec un agrandissement.

## Couverture

458 captures natives de référence :

| Bloc | Cas |
| --- | ---: |
| Lignes dirigées, pentes, clipping | 14 |
| Effacement, catalyseur, grandes figures, portrait, spatioport, monolithe | 7 |
| 82 sprites, position normale, bord droit/Y impair, BYTE négatif/bas | 246 |
| 23 paysages et 5 variantes lune/montagnes | 28 |
| 21 types de monstres en monde extérieur et intérieur | 42 |
| Façades cité, pyramide, Seth, Mahalore | 4 |
| Arc fermé | 1 |
| Intérieurs : trois perspectives et douze configurations de portes | 36 |
| Ruines : seize configurations, quatre orientations | 64 |
| Méduz : huit configurations de fosses | 8 |
| Alfol : huit configurations de portes | 8 |

Ces preuves s'ajoutent aux 44 cartes, 245 images de tunnel, deux tables
CP437, six images de fond, 82 sprites décodés et partitions sonores des
jalons précédents. Le monstre 15 reste couvert par ses tests d'animation,
temps et interruption ; il est exclu des 42 présentations fixes ci-dessus.

## Limites explicites

CONFIRMÉ pour ces fixtures, pas pour toute combinaison possible de monde,
PNJ et contenu de salle. Le texte BIOS de ces captures natives est abstrait ;
les tests CP437/menus ne sont pas une capture d'une session DOS entière.
Le profil XT reste un modèle temporel nominal ; le timbre et l'affichage
physiques d'un PC/XT/CGA ne sont pas mesurés.

À VÉRIFIER : parcours humain complet SDL, réactivité du vol, toutes les
combinaisons de PNJ/objets/décors. L'absence de détails aléatoires juste après
chargement reste documentée ; aucun nouveau tirage n'a été ajouté pour la masquer.

La correction ne change ni les ressources originales, ni le format de
sauvegarde. Le seul ajustement de présentation du core concerne l'ordre des
coordonnées de ruines face à l'ouest ; le total de tirages est conservé.

Validation : 37 suites CTest sur 37 réussies. Les 458 captures sont relues
après rendu SDL en 320x200 puis en 640x480 avec bandes noires, soit 916
comparaisons dans les tests C. L'oracle natif complet a été régénéré sans
cache. Les 68 cas du banc de navigation face à l'ouest sont aussi rejoués
contre les règles et le RNG originaux : aucun écart. Le paquet autonome
local est reconstruit par le test d'autonomie.
