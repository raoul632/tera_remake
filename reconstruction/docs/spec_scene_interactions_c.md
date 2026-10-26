# Points 4 et 5 : lieux et interactions SDL3

Périmètre : étapes 4 « Affichage des lieux » et 5 « Interactions » de la
mission. Aucun changement des règles, des commandes ou du calendrier RNG.

CONFIRMÉ [PAS] : TERAMAIN associe KRONO à Kronopolis, FACADE à Balgence et
Mortefontaine, MAISONJ à l'ermitage, MAISONS au hameau. Les overlays 00FB,
00F3, 0148, 0142, 00F0, 00EC décrivent les compositions de surface ; 002E,
0049, 0069, 0085 et 00A4 les intérieurs, Alfol et Meduz. Les coordonnées des
sprites sont en octets CGA (x*4), celles de DRAW en pixels. Les remplissages
historiques en octets sont convertis par leur primitive seulement.

Projection publique du core : lieu, orientation relative, trois faces,
type de salle et élément visible. Aucun nom d'objet caché ni piège futur
ne doit être révélé. Les coordonnées aléatoires de surface sont conservées
pendant les tirages déjà existants, jamais rejouées dans le renderer.
Elles sont transitoires, sans extension de la sauvegarde ; après chargement,
seules les parties déterministes du décor restent jusqu'au prochain pas.

Les événements du core alimentent les résultats visibles : ouverture,
piège, acquisition, refus, recrutement, achat/vente/hôpital, objectif et zone.
Les invites et le catalogue proviennent des API publiques. Aucun prix négocié,
soin, dégât, sélection de compagnon ou validation de porte n'est calculé par SDL.

À VÉRIFIER : palette par scène et primitives graphiques encore non analysées.
La transparence par octet nul de PutSprite/PutSprite_off/Off2Screen_skip0 est
maintenant CONFIRMÉE BIN et appliquée ; les pages de fond restent opaques.
La palette est celle documentée dans TERA_ASSETS ; les compositions
partielles ne sont pas déclarées identiques au pixel près. La fonte est celle
de remplacement. Les textes résidents manquants sont indiqués explicitement.
L'interface compacte de messages est une approximation de présentation.

## Réalisation

| Bloc | Présentation disponible | Limites graphiques |
|---|---|---|
| Amarande | 23 groupes fixes, sol/étoiles, lune à l'est, montagnes issues des tirages existants, tampon séparé et ordre PAS | Rasterisation des courbes à comparer à DOS |
| Dix lieux | Pages IMA, tours de Cité, Pyramide/temples, spatioport avec immeubles et vaisseaux, décorations et portraits | Remplissages et comparaison globale des captures à vérifier |
| Cité/Seth | Orientation, salles 1/2/3, portes et motifs, coffres, autel, échelles, monolithe/inscriptions graphiques | Texte résident de Seth manquant ; contours/remplissages à vérifier au pixel près |
| Alfol | Surface modulo 23, immeubles, arrivée, bureau/graphique, déchets radioactifs, saisie et investissement | Comparatif visuel global à effectuer |
| Meduz | Grille, étoiles, fosses orientées, catalyseur et sprites mentaux, méditation/objectif | Animation de transition par tunnels manquante |
| Interactions | Confirmations, mots avec effacement, choix A-D/0-9, objets visibles après ouverture, pièges, recrutement, catalogue A-S, achat/vente/hôpital | Textes UI abrégés de présentation ; aucune mécanique supplémentaire |
| Voyage | ASTRON, étoiles, carburant, cinq images de cible, rayon blanc temporisé et explosions | Essai humain et apparence matérielle des flashes à vérifier |

CONFIRMÉ [BIN] : les résidents C6AF6 et C6FDB ont été relus de façon ciblée.
`DrawStairs(162,100,2)` désigne une largeur et une hauteur, et non x/y ; le
code conserve la dernière marche qui dépasse la hauteur. C6FDB trace les
lignes y=130+i(i−1) et les rayons vers (160,130). Ces géométries ne dépendent
d'aucun RNG. Le dessin spatial suit OVL00AE (point puis sprites 1F/1E/1D/43)
aux temps déjà exposés par `tera_space_input_at` ; SDL ne calcule pas le tir juste.

`TeraGameView.scene` est emprunté jusqu'à la mutation suivante. Un obstacle
conserve le dernier paysage de la session. Un chargement READY ne rejoue pas
les tirages pour retrouver la décoration perdue. Le format reste 11521 octets.
Le champ `prompt.pool` expose aussi le coût annoncé du bureau des médias et
l'unité de carburant ; aucune multiplication de prix n'est faite dans SDL.

`TERA_PRESENT_INTERACTION` expose uniquement les résultats résidents déjà
présents : extension mentale, investissement des médias et manque d'argent.
La saisie ignorée d'Alfol est maintenant conservée pour affichage, sans changer
le nombre de lettres, les frontières clavier ou le résultat de l'invocation.
Les mots secrets attendus ne figurent jamais dans les invites.

Le frontend conserve 16 événements de résultat pour la commande courante,
dont un piège suivi des blessures de dix acteurs. Jusqu'à cinq lignes sont
affichées, avec priorité au piège ; les quatre indices d'un dialogue restent
visibles ensemble. Une commande refusée conserve les messages précédents.
Le journal technique de 64 événements reste disponible pour diagnostic.

## Validation exécutée

Compléments courants : [comparatif, validation 37/37 et limites](spec_scene_completion_c.md).
Les résultats 35/35 ci-dessous décrivent le jalon initial.

35 suites CTest sur 35, aucune exclusion. Le test `scene_view_contracts`
vérifie les quatre orientations, la non-révélation de 100 contenus fermés,
Alfol/Meduz, le calendrier RNG de la case 1201 (112 tirages), la consultation
sans mutation, le décor conservé sur obstacle et la sauvegarde sans rejeu.

`sdl3_scenes_interactions` charge les ressources originales et capture les dix
lieux, 23 paysages, ruines/salles, Alfol et Meduz. Des événements SDL réels
pilotent coffre/piège/ramassage, refus de porte, sollicitation/recrutement,
entrée/achat/vente/hôpital, mots/effacement, méditation, bureau et carburant.
Les personnages, invites et sauvegardes READY sont comparés à une session
recevant directement les mêmes octets. Chaque rendu compare la session
complète avant/après. Les cinq images spatiales utilisent un temps virtuel.
Ces fixtures sont explicitement synthétiques ; elles ne constituent pas une
preuve de parcours manuel complet ni de pixel-identité avec DOS.

Captures inspectées sous `build/sdl3/` : `amarande-00.bmp`, `place-00.bmp`,
`place-01.bmp`, `place-03.bmp`, `place-04.bmp`, `interior-2.bmp`,
`chest-trap-open.bmp`, `shop-purchase.bmp`, `dialogue-menu.bmp`,
`meduz-meditation.bmp`, `alfol-investment.bmp`, `space-shot-4.bmp`.
Le paquet autonome local `runtime-only/` est réactualisé par le smoke et
fonctionne sans COM, overlay, Python ni émulation.

## Correctif transparence et sortie de ville

Le premier rendu utilisait des sprites opaques et composait le sol avant
tous les groupes fixes ; il ne reproduisait pas les couches originales.
Le correctif utilise le saut d'octets nuls confirmé dans C5051/C5E04 et
C60BB, conserve le noir des octets mixtes et les remplissages du tampon
séparé, et replace le sol selon les 23 branches PAS. Les marches/colonnes
sont tracées avant la superposition de la Pyramide et des temples.

Kronopolis : Echap dans achat/vente retourne au menu ; Echap dans ce menu
termine la visite, sauf à la première attente où deux Echap sont requis.
Le core conserve ce REPEAT original. Le renderer explique maintenant cette
distinction, n'annonce plus Echap comme sortie du choix A-D (qui l'ignore),
et identifie la façade « extérieur ». Les flèches quittent cette case.

Le test pixels vérifie sur chaque sprite le fond préservé sous les octets
nuls ET les noirs opaques des octets mixtes ; il contrôle aussi l'alignement
191 -> 190 et la limite de dessin 192. Le test SDL traverse les deux Echap
initiaux, le retour extérieur et le choix de personnage qui ignore Echap,
avec comparaison à la session core directe. Cela ne certifie pas encore
l'identité de toutes les scènes avec une capture DOS.
