# Points ouverts révélés par le frontend SDL3

## Lieux et interactions — jalon 4/5

Les diagnostics des lieux sont remplacés par les compositions décrites dans
[la spécification des scènes](spec_scene_interactions_c.md). Le catalogue,
les dialogues, coffres/pièges, recrutements et interactions de progression
sont présentés à partir du core. Les coordonnées aléatoires d'Amarande sont
conservées pendant leurs tirages existants ; après chargement, leurs détails
manquent jusqu'au pas suivant, sans modifier le RNG ni la sauvegarde.

Le [bloc de compléments des scènes](spec_scene_completion_c.md) intègre
montagnes/lune, décorations des façades, portraits/tours, immeubles d'Alfol,
catalyseur de Meduz, déchets/bureau, inscriptions graphiques et rayon.
Les palettes par scène suivent maintenant les CASE PAS. Cartes/aide,
textes de Seth, tunnels et fins sont intégrés. Le [profil IBM PC/XT](spec_visual_xt_c.md)
valide le raster de 44 cartes et 245 images de tunnel, et utilise les tables
BIOS/CGA CP437 avec les proportions du mode texte. L'ancien doute sur A744
est levé par sa remise à zéro à 5B80. Le temps XT est modélisé. Le
[comparatif final des blocs graphiques](spec_final_comparison_c.md) vérifie
458 captures natives : sprites aux bords, paysages, monstres, portes, ruines,
fosses et façades. Les limites de couverture y sont détaillées.
Le combat terrestre a maintenant son affichage et ses principaux sons/délais ;
les types 16/17/19 sont complétés et comparés aux primitives BIN. Ambiances,
sept lieux, entrée souterraine, dépôt Alfol et voyage sont sonorisés.
Les mélodies des quatre objectifs, le recrutement, les pièges, l'entrée de
Méduz, ses gardiens et le refus Alfol sont raccordés ; voir
[sons de progression](spec_progression_sound_c.md). Le rayon est raccordé. Voir
[comparatif des étapes 6/7](spec_combat_media_sdl_c.md). Les images de tir spatial sont
maintenant rendues au temps du core et testées au temps virtuel ; leur
lisibilité et réactivité en parcours humain ne sont pas certifiées.

Résolu depuis le signalement utilisateur : transparence par octet CGA nul
confirmée pour C5051/C5E04/C60BB, tampon de composition séparé, ordre du sol
dans les 23 paysages et superposition des façades Pyramide/temples. Les tests
SDL contrôlent les noirs des octets mixtes et la limite de dessin à 192.

## Monstre 15 — présentation temporisée raccordée

Source : spec_entrees_gameplay.md, exploration.c / tera_monster_presentation_random.
Le core consomme trois tirages par image et interroge une source d'entrée,
jusqu'à six images. Le contrat actuel s'exécute dans un seul appel synchrone.
BIN C78EA..790C confirme 700+RANDOM255 ms puis la même fréquence pendant
20 ms. Le core émet maintenant les points et durées avant FRAME. Le service
SDL présente cette image tout en traitant la fermeture, puis teste le
clavier ; aucun tirage n'a été déplacé dans le renderer. Le smoke natif
injecte une touche pendant la première image et vérifie trois tirages,
interruption et purge avant le combat. Le contrat reste synchrone côté C,
avec une attente de présentation explicite autorisée pour FRAME seulement.

## Événements de présentation hors combat (CONFIRMÉ — SOURCE C / tests C)

Résolu pour le contrat de présentation : voir spec_presentation_c.md et
presentation.h. Les résultats transitoires sont émis par valeur avant les
changements de sous-session. Les invites courantes sont exposées par game_view.
Le frontend SDL est raccordé à ce flux ; aucun événement n'est reconstitué
depuis les valeurs affichées. Les sélecteurs COMMAND_RESPONSE sont maintenant
résolus par la table CAFE8 documentée dans TERA_ASSETS.md. Les textes externes,
noms et phrases de combat sont chargés en C. Les inscriptions résidentes de
Seth, les fins et les sons de progression sont maintenant intégrés.

## Fidélité des assets (À VÉRIFIER / MANQUANT)

Formats et ressources externes intégrés : voir TERA_ASSETS.md et le manifeste
SHA-256. Les images restent indexées sans perte. Les palettes 2/3 des
scènes sont raccordées. Les blitters et la fusion par octets sont comparés
au raster natif ; l'apparence physique d'un moniteur CGA reste À VÉRIFIER.
Les polices matérielles IBM BIOS/CGA CP437 sont intégrées et vérifiées par
empreintes et pixels de référence. Elles ne dépendent pas d'un fichier TERA.
Les sprites ne sont pas associés aux
monstres par ressemblance. Leurs affectations/compositions confirmées sont
raccordées sans modification du RNG.
