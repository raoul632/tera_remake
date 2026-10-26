# Audit de fidélité TERA

Inventaire commencé le 11 octobre 2026, état courant au 15 octobre 2026
(lots 17/18 au 15 octobre en fin de document). Cadre de référence : `cadre_projet.md`.
**Le prototype fidèle de bout en bout n'est pas obtenu.** Cet inventaire
initial oriente l'analyse ; il ne certifie pas les fonctions non examinées.

**Séparation des travaux :** cet audit, ses tests et le C appartiennent à
`reconstruction/`. Le Python historique est restauré. Ses corrections
validées sont conservées dans `patches/python-combat-corrections.patch`
et testées sur une copie sous `reconstruction/build/`. Toute mention de
Python corrigé ci-dessous désigne cette copie, pas le prototype historique.

## Sources et outils présents

| Source | Présence / portée |
|---|---|
| BIN | TERA.COM (50 909 octets), TERA.000 (84 992 octets) |
| PAS | 26 overlays, TERA.PAS, TERAMAIN.PAS ; corpus incomplet |
| PAS reconstruit | TERAUTIL.INC et KBD.INC ajoutés lors de travaux précédents : ne pas confondre avec des sources complètes |
| DATA | VALMON, VERBES, OBJETS, NOMON, NOMPERS, MESSAGE/2/3, sept fichiers IMA |
| MANUAL | manuel/Tera.pdf présent ; pas relu visuellement dans ce lot |
| OBS | Images de référence présentes ; aucune session originale exécutée dans ce lot |
| PY | Reconstruction existante, extracteurs et tests de fumée |
| Outils | Python 3.10, pytest, pygame, objdump i8086, GCC et CMake vérifiés sur le poste |

Aucun DOSBox n'a été trouvé dans le PATH lors de l'inventaire ; cela
n'établit pas son absence sur tout le poste. L'observation dynamique reste
à préparer. Les preuves BIN ciblées sont reproductibles via
`reconstruction/tools/audit_combat_binary.py`, sorties locales sous
`reconstruction/build/`, exclues de Git.

## Résultats du premier lot

| Fonction / mécanique | Statut | Provenance et résultat |
|---|---|---|
| Seuil 44/35 | CONFIRMÉ | BIN 82E5–8314, comparaison signée stricte |
| Compétence non acquise = -2 | CONFIRMÉ | BIN 8215–826F, Skills=0 |
| Jet de base et système D | CONFIRMÉ | BIN 8315–8389, colonne Skills[7], pas une variable bonus_fixe |
| Bonus d'équipement | CONFIRMÉ | BIN 8270–82E4 et 8391–8429 : trois objets successifs du même personnage |
| Écriture AFCC par le bonus | CONFIRMÉ | BIN 83F0–841B : ID d'objet, exception objet maximal 21 |
| Usage AFCC en résolution | CONFIRMÉ | BIN C_8B5E : affichage du nom puis remise à zéro ; pas de retrait d'inventaire dans cette routine, API d'affichage non portée |
| Neuf branches numériques | CONFIRMÉ | BIN 842A–86E0 ; spec indépendante, port C et Python corrigé |
| Tirage supplémentaire | CONFIRMÉ | BIN 843F–8448 puis appel C_8315 pour types simples |
| Menu de sept attaques | CONFIRMÉ | BIN 8790–89C7 croisé avec DATA VERBES ; trois anciens verbes mal associés |
| Aides H[1..4] | CONFIRMÉ | PAS INNER_BBA5 + BIN overlay C246–C508 ; attribution initiale, pas choix de joueur |
| Appel de soin de l'autel | CONFIRMÉ | PAS OVL_0049 : C_8315(perso,6,6), pas une attaque C_842A |
| Mot demandé par « crier » | CONFIRMÉ | BIN 8932–894B et 90D4–90E2 : catégorie 5 et SecretWords[1] ; traitement de saisie non porté |
| Types 7/8 : déclencheurs | CONFIRMÉ | BIN C_92FE : défense physique pour ID <15, mentale sinon |
| Résolution C_8CE3 | CONFIRMÉ | BIN 8CE3–92FD, divisions par catégorie/action ; 8D20 n'est pas une entrée de fonction ; port C lot 2 |
| Attaque du monstre | CONFIRMÉ | BIN C_92FE : HP actuels +4, défense 7/11 ou 8/9, C_8C45 ; port C lot 2 |
| Empoisonnement de riposte | CONFIRMÉ | BIN C_8C45 et routines d'ensemble : IDs 1/2/3/6/7, tirage sur toute touche ; API pure lot 2 |
| Seuil de défaite du monstre | CONFIRMÉ | BIN C_9419 : HP <3 après chaque compagnon ; distinct de la victoire globale |
| Préparation/initiative/cible | CONFIRMÉ | BIN 9757–9A5D : liste, régénération MP, moyenne, seuil strict et sélection ; port C + 251 comparaisons |
| Mise à jour HS | CONFIRMÉ | BIN C_7CC8 : HP <1 et état !=HS ; port C + 36 comparaisons |
| Maxima HP/MP recalculés | CONFIRMÉ | BIN C_4C6C appelé par C_4D46 : HP = Stats1+4+9, MP = Stats2+5 ; port C + 103 comparaisons |
| Tour numérique intégré | CONFIRMÉ | BIN 9757–9AAB et sous-appels, 880 tours comparés ; menu/jet fournis aux frontières |
| Relocation de fuite | CONFIRMÉ | BIN C_86E1 + variables PAS, 94 comparaisons ; confirmation et présentation externes |
| Compteurs de récompense | CONFIRMÉ | BIN 9AAE–9B42 + variables PAS, 24 comparaisons ; pas une validation des niveaux |
| Combat complet interactif | CONFIRMÉ BIN + exécution | Initialisation, menu/confirmations/annulations, mot, calendrier RNG et fuite sur cartes portés ; frontend terminal jouable isolément |
| RNG original | CONFIRMÉ BIN + exécution | 10DA/10E6 porté et comparé sur 770 cas ; RANDOMIZE 0F14 conserve les octets DOS de l'heure |

La confirmation couvre désormais le combat interactif et ses règles ;
le rendu CGA et le parcours du jeu complet restent hors de cette validation.
Voir `spec_combat_actions.md` pour les adresses, signatures, pseudo-code,
tables et limites de validation ; le lot 2 est dans `spec_combat_resolution.md`.

## État des autres blocs nécessaires

| Bloc | Statut courant | Travaux nécessaires |
|---|---|---|
| Création du héros | CONFIRMÉ BIN + exécution | OVL_0022 porté : nom, genre, tirages, répartitions, bonus, refus, PV/MP et compteurs ; frontend terminal disponible |
| Neuf compagnons | CONFIRMÉ BIN + exécution | Noms NOMPERS, spécialités, statistiques et équipements personnels initialisés en C |
| Recrutement/placement/indices | MANQUANT | Calendrier géographique de 0010, dialogues et règles d'entrée dans le groupe à reconstruire |
| Amarande et déplacements | À VÉRIFIER | Sources PAS présentes ; vérifier rebouclage, obstacles et sites réels |
| Cité des Crânes/génération | À VÉRIFIER | PY simplifie marche et marqueurs ; recroiser OVL_0000, ne pas conserver les invariants inventés |
| Entrées entre lieux | MANQUANT | PY choisit des positions fixes ; retrouver LocTowns/LocExtra et conditions exactes |
| Villes | MANQUANT | PY remplace toutes les villes par la boutique de Kronopolis ; suivre C_7309 et dispatch de TERA.PAS |
| Commerce et hôpital | À VÉRIFIER | PAS OVL_00BC disponible ; prix/effets affirmés fidèles dans les notes, non revalidés ici |
| Objets/coffres/pièges | À VÉRIFIER | Acquisition, ouverture distincte, inventaire, consommation et états à auditer |
| Alfol/Meduz | À VÉRIFIER | Générateurs et actions PY présents ; placements provisoires et progression à revalider |
| Temple de Seth/tâche 4 | MANQUANT | PAS OVL_0069 donne autel code 110, mot secret[4], monstre 21, drapeau task[3] ; accès/génération non portés |
| Séquence spatiale | MANQUANT | PAS OVL_00AE disponible ; déterminer et conserver son rôle dans le parcours original |
| Temps/XP/niveaux/défaite | MANQUANT | Dépendances résidentes et boucle principale à reconstruire |
| Victoire | À VÉRIFIER | PY all(tasks_done) insuffisamment démontré ; croiser retour de TERA.PAS et OVL_00CB |
| Rendu | À VÉRIFIER | PY contient zoom, sprites supposés, géométrie et scènes approximées ; besoin de comparaisons OBS ciblées |
| Entrées BIOS/fichiers/segments | MODERNISABLE | Abstraction moderne pour effet équivalent ; garder les exceptions qui influencent règles/RNG/affichage |
| Sauvegarde moderne | MODERNISABLE | Format moderne autorisé ; conservation de tout l'état utile restant à vérifier |

Ces statuts expriment le niveau de validation de ce nouvel audit, pas un
jugement définitif sur chaque ligne existante.

## Les quinze critères de fin

| # | Critère utilisateur | État de validation |
|---|---|---|
| 1 | Lancement du jeu | Génération, héros et exploration terminal reliés ; autres lieux et rendu graphique incomplets |
| 2 | Nouvelle partie | Cartes, roster, lieux, indices, mots et héros C générés ; arrivée automatique reliée |
| 3 | Personnages selon règles originales | Héros, neuf compagnons, recrutement et succession comparés au binaire |
| 4 | Mondes et lieux accessibles | Amarande, cité, Seth et services de Kronopolis reliés ; autres lieux et voyages incomplets |
| 5 | Déplacements corrects | Amarande/cité/retour/fuite comparés ; autres mondes et voyages manquants |
| 6 | Interactions essentielles | Dialogues, cité, Seth et commerce/hôpital de Kronopolis reliés ; autres lieux incomplets |
| 7 | Monstres et rencontres | Initialisation des 22 monstres, rencontres Amarande/cité et reprise reliées et comparées |
| 8 | Combat selon règles reconstruites | Combat interactif C relié à l'exploration/cité, comparé au COM ; rendu CGA hors lot |
| 9 | Objets/coffres/pièges | Coffres de cité, pièges, acquisition, abandon, transfert et potions comparés ; objets des autres événements manquants |
| 10 | Événements de progression | Incomplet |
| 11 | Objectifs originaux accomplissables | Objectifs cité et Seth accessibles ; autres objectifs manquants |
| 12 | Victoire atteignable | Non démontré dans le C : autres objectifs et fin victorieuse/score manquants |
| 13 | Parcours de bout en bout | Non |
| 14 | Aucune mécanique inventée nécessaire | Non démontré pour le parcours complet : passages historiques encore provisoires ; combat C fondé sur le BIN |
| 15 | Divergences documentées | Inventaire commencé, audit fonction par fonction à poursuivre |

Les tests de fumée ou numériques ne constituent pas une validation de
ces critères. Le prototype reste inachevé.

## Ordre de travail suivant

1. Le combat interactif est désormais porté et jouable isolément ; voir
   `spec_combat_interactif.md`. HP initiaux, menu, confirmations,
   annulations, saisie, RNG et fuite sur cartes sont intégrés. Son frontend
   terminal ne certifie pas les scènes CGA originales.
2. Le bloc personnages est désormais porté : héros interactif, profils
   et équipement des compagnons, recréation après défaite. Voir
   `spec_creation_personnages.md`. Terminer les générateurs et placements
   mondiaux, leur calendrier RNG, puis les règles de recrutement.
3. Reconstruire déplacements, parcours et événements, notamment accès/Seth
   et victoire, puis brancher les rencontres sur le combat C.
4. Étendre le C à partir des seules spécifications validées, frontend puis
   comparaison au jeu original et vérification des quinze critères.

## Validation effectuée dans ce lot

- 17 tests Python passés (dont 7 tests de fumée historiques).
- Bibliothèque C99 compilée GCC avec -Wall -Wextra -Werror -pedantic.
- CTest : 1 exécutable de vecteurs de source passé.
- Preuves BIN recapturées après correction de l'origine d'overlay.
- Construction de partie du frontend Pygame vérifiée sans fenêtre :
  génération et aides présentes, création interactive du héros substituée
  par un personnage de test. Cela ne valide pas l'écran de création.

Dans les premiers lots, pas d'émulation ni de parcours de bout en bout.

Après séparation : 10 tests de provenance passent sur la copie corrigée,
les 7 tests historiques passent sur le Python historique, et l'exécutable
CTest passe après recompilation depuis `reconstruction/`.

Lot 2 : les deux exécutables CTest passent après compilation stricte.
Les nouvelles preuves incluent C_8B5E..C_9419, le parent de combat et les
routines d'ensemble nécessaires à l'empoisonnement. Le Python historique
et le patch isolé du premier lot restent inchangés dans ce lot.

Une comparaison exécutée avec TERA.COM a ensuite été ajoutée : campagne
de 6 222 cas sans écart entre les routines originales émulées et
le C. Voir `validation_binaire.md` pour les frontières remplacées et le
rapport local actualisé. Cela ne valide pas un parcours de bout en bout.

Lot 3 : 390 comparaisons exécutées supplémentaires, aucun écart ; trois
suites CTest passent. Voir `spec_combat_tours.md` pour les résultats, les
frontières remplacées et les points restant à intégrer au combat complet.
Les 6 222 comparaisons précédentes ont été relancées sans écart.

Lot 4 : 880 tours intégrés et 118 sorties sans écart, quatre suites CTest
passent. Total cumulé : 7 610 cas différentiels distincts. Voir
`spec_combat_boucle.md` pour le périmètre et les limites.

Lot 5 : combat interactif complet en C avec frontend terminal, ressources
originales locales, import des personnages/cartes et export de l'état final.
Le banc exécute désormais le menu et le RNG originaux : 720 rencontres,
4 934 frontières et 770 cas de RNG, aucun écart. Cinq suites CTest et trois
tests de transport/import/export passent. Les limites de rendu et
d'intégration au parcours complet sont dans `spec_combat_interactif.md`.

Lot 6 (14 octobre 2026) : initialisation des neuf compagnons et création
interactive du héros portées en C. Le refus/recommencement, les répartitions,
les six bonus, les états finaux et la recréation après défaite sont comparés
aux overlays originaux : 662 scénarios, 11 128 frontières, aucun écart.
Six suites CTest et sept tests des frontends passent. L'export des vrais
personnages peut alimenter le combat ; les cartes hôtes restent préservées.
Le calendrier géographique de l'overlay 0010, les indices/mots/H et le
recrutement restent des blocs distincts, clairement non certifiés ici.


## Lots 7 et 8 — monde et recrutement (14 octobre 2026)

**CONFIRMÉ — BIN/PAS/DATA** : OVL0000 après scénario/difficulté/RANDOMIZE,
OVL0010 complet et leur calendrier RNG jusqu’à la création du héros sont
portés en C. Les nouveaux états incluent cartes, placements, indices,
mots/H et drapeaux. Voir `spec_generation_monde.md`. Les frontières
anciennes des lots personnages sont désormais complétées par ce banc.

`compare_world_binary.py` : 122 générations, 2 186 881 appels au RNG
original, 124 tentatives (deux cartes refusées), cinq collisions de
placement ; six créations à partir de la mémoire originale générée,
159 frontières clavier. Aucun écart. Le résidu non initialisé de BA2F
est vérifié pour pile initiale FFFE ; autres contextes DOS À VÉRIFIER.

`compare_recruitment_binary.py` : 696 scénarios, 492 appels au RNG,
aucun écart. Solliciter, séduire, confirmations/annulations, dix jets,
paiement et activation des HS sont conservés. Voir `spec_recrutement.md`.
Le drapeau AFCE est partagé avec le menu de combat, comme dans le COM.

L’intégration révèle des groupes >4 possibles après recrutement multiple :
les quatre emplacements et les cibles absentes du CASE original sont
maintenant reproduits. Campagnes étendues : 970 tours intégrés et
792 combats interactifs, 5 162 frontières clavier, aucun écart ; les
770 vecteurs RNG passent aussi. Les 662 créations précédentes ont
été relancées sans écart. Sept suites CTest et onze tests frontend passent.

Reproduire les preuves avec `tools/audit_world_recruitment_binary.py`,
les comparaisons avec `tools/compare_world_binary.py` et
`tools/compare_recruitment_binary.py`. Rapports sous `build/audit/`.
Les tests frontend incluent monde → héros → recrutement → combat,
préservation des cartes/H/mots, annulation et absence d’export interrompu.

**MANQUANT** : exploration/déplacements, autres interactions/objets et
événements, déclenchement automatique des rencontres, rendu graphique et
validation d’une partie entière. La génération et le recrutement ne
certifient pas encore ces blocs ni le conteneur de sauvegarde DOS.


## Lots 9 et 10 — déplacements Amarande et calendrier quotidien

**CONFIRMÉ — BIN/PAS** : OVL00FB pour la marche, obstacles, bouclage,
détection des lieux et tirages des décors ; C_A0DD pour rencontres, jours,
récupération, poison et progression. Les combats sont ceux du moteur C,
avec reprise du calendrier après leur retour et leur graine finale.
Voir `spec_exploration_temps.md` pour le détail et les limites.

`compare_exploration_binary.py` : **798 scénarios, 848 frontières, 49 015
appels au RNG original, 25 rencontres imbriquées, aucun écart**. Les tests
couvrent les 23 scènes, les limites, montagnes, villes, états/HP/MP,
argent/XP/niveaux/compétences, groupes 1..10 et les onze types de rencontre
errante. Les touchés clavier sont injectés ; les calculs et le RNG sont
exécutés par les instructions originales. Le chemin d’animation du type
15 sans touche est vérifié ; son interruption anticipée À VÉRIFIER.

Huit suites CTest et quinze tests frontend passent. Le frontend terminal
relie un vrai monde généré au héros, à l’arrivée automatique de TERAMAIN,
à la marche, au calendrier et aux combats. Les fichiers sources et le
contexte géographique sont préservés ; une interruption ne publie pas
d’état final. S n’invente pas de sollicitation automatique préalable.

Preuves reproductibles : `tools/audit_exploration_binary.py`, manifeste
et rapport sous `build/audit/`. Le dépôt historique demeure inchangé.
**MANQUANT** : entrées/actions des lieux, dialogues complets, objets,
labyrinthes, Alfol/Meduz, voyages, objectifs/victoire, recréation après
mort dans la boucle générale et rendu graphique. Le parcours entier du
jeu reste à valider ; ces deux lots ne le déclarent pas terminé.

## Lots 11 et 12 — cité des crânes et interactions des niveaux

**CONFIRMÉ — BIN/PAS/DATA** : navigation relative C7EF6/C7DBD, overlays
002E/0049, murs/portes/clés, portail et passages D/M, indices L, autel I,
invocation générique ARIOCH, entrée/retour TERAMAIN, rencontres fixes et
reprise après fuite. La graine et les mutations de carte/objectifs sont
réintégrées dans le vrai état de monde et conservées au rechargement.
Voir `spec_navigation_cite.md` pour les règles, adresses et frontières.

La comparaison a révélé une limite des anciens oracles : leurs sauts de
présentation excluaient plusieurs appels à RANDOM. Le moteur et les
oracles ont été corrigés pour garder les tirages des sons et couleurs du
combat, pas et musiques d'Amarande/cité, animations de monstres et
accomplissement d'Arioch. Les fréquences/BIOS peuvent être abstraits,
la chronologie RNG ne le peut pas. Les chiffres actuels de
`validation_binaire.md` remplacent les anciennes campagnes interactives
et exploration pour ce périmètre. Les anciennes comparaisons de formules
ne constituent pas une certification de cette chronologie.

Le frontend terminal relie surface → cité → niveaux → surface. Neuf suites
CTest et vingt tests frontend passent, dont victoire, portes, objectif et
rechargement. Les nouvelles preuves et changements restent intégralement
dans `reconstruction/` ; le dépôt historique reste inchangé.

**MANQUANT** : coffres/pièges/inventaire, autres lieux et dialogues, mondes
9/10, Alfol/Meduz, voyages, autres objectifs et fin du jeu, recréation dans
la boucle générale. **MODERNISABLE** : rendu CGA et audio audible, encore
à connecter. **À VÉRIFIER** : animations interrompues au clavier. La cité
jouable dans ce périmètre ne certifie pas une partie complète.

## Lots 13 et 14 — coffres/pièges et inventaire/équipement

**CONFIRMÉ — BIN/PAS/DATA** : ouvrir/prendre distincts, quatre pièges et
désamorçage, argent/clés/cartes, acquisition CAA13, transfert, abandon
d'objet/compagnon, potions 23/24, sélection A-D et lecteur de slot 0..9.
Pas de compactage, consommation ou équipement actif inventés. Les bonus
de combat continuent de scanner les objets du même personnage.

`spec_coffres_inventaire.md` documente les particularités et preuves :
1 076 cas de coffre et 632 d'inventaire sans écart ; navigation/entrée
relancées sans écart ; dix suites C et vingt-six tests frontend passent.
BP_03 est réinitialisé au prochain rendu, sans permanence inventée sur
les cartes. Le snapshot technique Q le conserve pour reprendre la session,
sans prétendre reproduire le conteneur de sauvegarde DOS.

**MANQUANT** : autres dialogues/lieux et objets de leurs événements,
autres mondes/voyages, objectifs et fin, recréation dans la boucle générale.
**MODERNISABLE** : graphique/audio matériel encore à connecter.
**À VÉRIFIER** : animations interrompues. Dépôt historique inchangé ;
nouveaux fichiers, moteur, ponts et preuves sous `reconstruction/`.

## Lots 15 et 16 — dialogues résidents et succession

**CONFIRMÉ — BIN/PAS/DATA** : N/I/A/V/D/S, S direct, contexte AFCE/A750,
indices observés, réponses originales et transfert. La musique après
négociation réussie conserve ses tirages, qui manquaient au banc isolé
sans son. Les présentations de PNJ et monstres propagent A750 ; ce flag
est distinct des facultés mentales 9418.

**CONFIRMÉ — BIN/PAS** : perte du groupe → condoléances OVL00CB → pause
→ OVL0022 → A748 faux → arrivée automatique dans le même monde.
Les quatre objectifs déjà vrais arrêtent la boucle après les condoléances,
même avec un groupe mort. Aucun nouveau monde ni RANDOMIZE. Les cartes,
mots, objectifs, lieux et butin sont conservés.

466 cas de dialogue et 120 de succession sans écart à chaque frontière ;
11 suites C et 33 tests frontend passent. Voir les règles, exclusions et
commandes dans `spec_dialogues_succession.md` et `validation_binaire.md`.
Tout le travail reste sous `reconstruction/`, historique inchangé.

**MANQUANT** : autres lieux et transactions, objets de leurs événements,
mondes 9/10, Alfol/Meduz, voyages, autres objectifs, fin victorieuse/score.
**MODERNISABLE** : graphique et son audible, encore abstraits.
**À VÉRIFIER** : animations interrompues au clavier et parcours complet.
Ces deux lots ne déclarent pas le jeu terminé.

## Lots 17 et 18 — Kronopolis et temple de Seth

**CONFIRMÉ — BIN/PAS/DATA** : OVL00BC entier, achats A..S, ventes,
hôpital gratuit, prix négociés, compteurs signés et refus plein/HS.
OVL0069 : entrée/retour, navigation et clés, coffres/pièges propres au
temple, rencontres fixes/calendrier, inscriptions et quatrième objectif
par invocation du mot secret. La cité et Seth ont des contextes d'overlay
distincts malgré leur partage de WorldId=8 et des données de carte.

Les tirages des façades TERAMAIN/OVL00F0, sons, inscriptions et ordre
de dégâts sont conservés. Une entrée mortelle chez Seth attend encore
sa première commande, particularité de REPEAT désormais préservée au
rechargement. Voir `spec_kronopolis_seth.md` pour les règles et limites.

512 cas de commerce, 274 de Seth/entrée et 16 de façades sans écart.
887 scénarios de cité, 1 076 de coffres de cité et 51 d'entrée/retour
relancés sans écart ; 11 suites C et 41 tests frontend passent.
Les deux blocs sont reliés au terminal et aux vrais mondes générés.
Tous les changements restent sous `reconstruction/`.

**MANQUANT** : autres lieux/événements et voyages, mondes 9/10,
Alfol/Meduz, autres objectifs et fin victorieuse/score.
**MODERNISABLE** : graphique/audio matériel encore abstraits.
**À VÉRIFIER** : animations interrompues et parcours complet.

## Lots 19 et 20 — spatioport et Alfol

**CONFIRMÉ — BIN/PAS/DATA** : TERAMAIN E222..E3EA et OVL00AE en jeu,
sélection du dernier pilote, dix jets, pirates, ravitaillement signé,
tirs, compteur double sur réussite, explosions, sept jours, HALT spatial.
OVL0085 : surface circulaire, bâtiments du monde 9, portes et clé 9,
PNJ, médias (objectif 1), dépôts radioactifs, monstres R3+12, calendrier,
fuites, sortie des bâtiments et retour au spatioport. Le refus de R au
point zéro sort quand même d'Alfol. Une entrée mortelle conserve sa
première commande obligatoire après export/rechargement.

Les effets de D9B66 sont conservés : dernier compagnon abandonné, scans
d'invocation/négociation, jour et dessin extérieur. Le cas 16 de la
radiation reproduit explicitement l'alias MP[6]/premier texte d'indice du
COM, sans corruption mémoire C. Voir `spec_spatioport_alfol.md`.

173 vols et 120 sélections de pilote, puis 613 scénarios Alfol sans écart,
à chaque frontière de lecture ou poll final. Les 906 cas totalisent
2 133 états comparés et 398 082 appels au RNG original. Le timing du
terminal est une adaptation ; les tests imposent les touches au poll final.
11 suites C et 54 tests frontend passent, dont treize nouveaux parcours
pour les vols, médias, radiations, combats et reprise après défaite.
Tout le nouveau code, tests, outils et documentation reste sous
`reconstruction/`. Les originaux sont inchangés.

**MANQUANT** : Meduz/monde 10 et son objectif, autres lieux/événements
d'Amarande, fin victorieuse/score et parcours intégral.
**MODERNISABLE** : rendu graphique et sons audibles.
**À VÉRIFIER** : timing interactif réel, animations interrompues au clavier.

## Lots 21 et 22 — Meduz et fin de partie

**CONFIRMÉ — BIN/PAS/DATA** : TERAMAIN/pyramide, OVL0145/0142,
OVL00A4, navigation du monde 10, combat direct 22 et calendrier après
tentative bloquée, méditation (extension et octets de caractéristiques),
invocation et objectif 0, retour de sept jours même après mort. La première
commande obligatoire d'une arrivée mortelle est conservée au rechargement.

**CONFIRMÉ — BIN** : test des quatre drapeaux au retour de l'overlay,
OVL00CB victoire/condoléances, musique, pause et absence de successeur en
fin de quête. Score Real48 : les six octets et les chiffres affichés sont
comparés au runtime du COM, troncature après chaque opération, jours signés
et erreur runtime 2 pour un dénominateur nul. Aucun score corrigé inventé.

326 scénarios Meduz, 48 de voyage/entrée/retour et 510 de fin, sans écart :
1 904 états, 53 392 appels RANDOM originaux. Voir `spec_meduz_fin.md`.
Les quatre mécanismes d'objectif et la condition de fin sont portés.
Les ajouts restent sous `reconstruction/` ; historique inchangé.

11 suites CTest et 67 tests frontend passent. Les nouveaux parcours
vérifient notamment la fin différée jusqu'à la sortie de Meduz, la mort
après les quatre objectifs, l'absence de double musique après reprise,
la commande obligatoire d'une arrivée mortelle et les EOF sans export.

**MANQUANT** : autres lieux/événements secondaires d'Amarande et conteneur
de sauvegarde DOS. **À VÉRIFIER** : parcours intégral d'un monde généré sans
prévalidation d'objectifs/repositionnement, clavier des animations.
**MODERNISABLE** : rendu graphique et sons audibles. Le jeu entier n'est
pas déclaré validé de bout en bout.

## Lieux et événements secondaires d'Amarande — 15 octobre 2026

Le manque « autres lieux/événements secondaires d'Amarande » du jalon
précédent est traité : lieux 5..9, façades des dix lieux, PNJ et A750,
combats résidents, invocation, panneaux, halte et réponses génériques.
**CONFIRMÉ — BIN/PAS/DATA** : 320 scénarios de façades et 311 de
commandes/panneaux, zéro écart. Les ID et variantes des textes sont aussi
contrôlés. Les retours Cité/Alfol conservent les tirages de leur façade.

Régressions exploration : 854 cas, zéro écart. Alfol : 613 cas, zéro
écart, avec D9B66 ajouté à la comparaison ; l'atterrissage laisse 16 et
le jour le préserve. Les règles restent en C, les ressources historiques
sont lues sans modification. Voir `spec_amarande_secondaire.md`.

11 suites CTest et 78 tests frontend passent, dont onze nouveaux parcours
sur ce bloc. Les interruptions n'exportent pas d'état partiel.

**À VÉRIFIER** : parcours intégral sans repositionnement ni prévalidation
d'objectifs, clavier des animations. **MANQUANT** : conteneur de sauvegarde
DOS. **MODERNISABLE** : graphismes et sons audibles ; l'interface actuelle
est un terminal. Aucun jalon ne certifie le jeu entier de bout en bout.

## Orchestration commune en C — 16 octobre 2026

L'API `game.h` rassemble le routage des commandes et les transitions qui
etaient auparavant assemblees par les outils terminal : arrivees/fuites,
facades, lieux, voyages, commandes residentes, retours apres defaite,
succession et fin differee jusqu'a la sortie d'un overlay.
Les lectures et combats restent suspendus dans leurs sessions C, sans
dependance Python. Les trois commandes obligatoires apres arrivee mortelle,
les globals de retour et le HALT spatial sont conserves explicitement.

**CONFIRME — tests C d'integration** : onze groupes de contrats supplementaires,
comparaisons d'etats et de graines avec les chaines directes de modules,
et transmission des evenements d'un vrai combat imbrique.
Voir `spec_orchestration_c.md`. Ce resultat ne remplace pas une comparaison
globale de TERAMAIN au binaire ni une partie complete sans repositionnement.
La sauvegarde/reprise durable C et le clavier des animations restent a traiter.
Compilation reussie ; 12 suites CTest sur 12 passent sans Python.

## Tests directs des derniers modules C — 16 octobre 2026

Six nouvelles suites CTest couvrent commerce, voyage spatial, Alfol, Meduz,
evenements/facades d'Amarande et fin de partie. Les attentes ciblent les
particularites confirmees dans les specifications : debordements signes,
seuils d'achat, HS/inventaire plein, paiements/objectifs repetes, pauses,
comptages RNG, alias radioactif, bonus mentaux et octets Real48 du score.

**CONFIRME — tests C** : 18 suites sur 18 reussies, sans Python ni ressources
externes necessaires a l'execution. Les fixtures sont synthetiques annoncees ;
les assertions des six nouvelles suites sont actives aussi avec NDEBUG.
Voir `validation_modules_c.md` pour la couverture precise.
Aucune regle du moteur n'a ete modifiee dans ce lot. Ce resultat complete
les campagnes BIN anterieures sans constituer une nouvelle campagne BIN,
ni certifier un parcours complet sur un monde genere.

## Parcours complets C sur deux mondes generes — 16 octobre 2026

**CONFIRME — integration C** : graines 1986 et 12345, difficulte 0 et son
desactive, ressources NOMPERS/VALMON originales locales. Generation et
creation, recrutement, achats, Cite/Arioch, Seth, fonds acquis dans les
coffres, voyages/Alfol/medias et Meduz atteignent les quatre objectifs puis
la fin victorieuse avec scores 5 et 7. Les parcours contiennent respectivement
8 et 7 successions reelles, sans rechargement du monde ni modification de
position, statistiques, argent ou objectif par le conducteur.

20 suites CTest sur 20 passent, campagnes incluses et non sautees localement.
Le banc verifie les drapeaux/compteurs, les douze mondes visites et deux
traces deterministes. Aucune modification du moteur n'a ete necessaire.
Voir `validation_parcours_c.md` pour les chiffres, ressources et limites.
Le planificateur lit les cartes et mots generes ; cette preuve ne valide
pas la decouverte humaine des enigmes, toutes les graines/difficultes ni
une execution integrale comparee au DOS. Persistance et timing restent
des travaux separes, comme le frontend SDL du contexte 4.

## Entrées de gameplay — reprise du 16 octobre 2026

Les services C de purge et de présence clavier du monstre 15, ainsi que les
événements spatiaux horodatés, sont maintenant couverts par une suite dédiée.
Les nouveaux contrats vérifient aussi le passage entre deux tirs, l'absence de
source clavier et les refus sans mutation. Voir `spec_entrees_gameplay.md` pour
les frontières et les limites : une comparaison dynamique des interruptions au
DOS n'a pas encore été effectuée dans ce lot.

Après l'alerte antivirus, seule la cible de test d'entrée a été recompilée.
**20 suites exécutées sur 20 réussissent**, campagnes comprises ; la suite
`space_module_contracts` est exclue et son exécutable n'est pas restauré ni
reconstruit. Cela ne constitue pas un nouveau résultat de 21 suites réussies.
Voir `audit_securite_test_space_2026-10-16.md` : faux positif non confirmé.

## Sauvegarde C et comparaison native des entrées — 16 octobre 2026

**MODERNISABLE — format, CONFIRMÉ — tests C** : format explicite v1, longueur
et CRC, validation avant publication, pointeurs exclus, reprise READY sans
transition ni RNG. Deux campagnes complètes avec 46 et 41 reprises retrouvent
leurs traces et scores de référence. La suite de sauvegarde passe en 32 et
64 bits. Voir `spec_sauvegarde_c.md`, notamment les limites de publication du
fichier et les opérations en cours non sauvegardables.

**CONFIRMÉ — BIN/C ciblé** : oracle natif C/Unicorn, 70 cas monstre 15 et
166 cas de fenêtres spatiales, zéro écart. Les instructions originales de RNG,
branches et purges sont exécutées ; aucun interpréteur Python n'est lancé.
Voir `spec_entrees_gameplay.md` pour les hooks matériels et bornes exactes.

Régression ordinaire : **23 suites exécutées, 23 réussies**, après compilation
des cibles concernées ; une suite spatiale exclue pour l'alerte. Configuration
native 64 bits : deux suites sélectionnées réussies (sauvegarde et oracle).
La comparaison globale de TERAMAIN au DOS demeure À VÉRIFIER ; aucune nouvelle
validation de l'exécutable de l'alerte n'est revendiquée.

## Réserve TERAMAIN levée pour deux parcours — 16 octobre 2026

**CONFIRMÉ — BIN/C sur deux parties complètes** : TERAMAIN relogé et seize
overlays originaux sont exécutés par un banc C natif en parallèle du moteur.
Les mêmes 7 143 touches donnent 3 990 frontières stables concordantes et les
mêmes octets Real48 du score final. La machine DOS conserve son état entre
création, exploration, lieux, combats, huit successions et victoire ; aucun
recalage de graine ou d'état n'est effectué après l'initialisation commune.

Deux divergences réelles ont été corrigées : 96 tirages manquants pour les
inscriptions 104..113 de la Cité, et exclusion du type 13 dans l'ensemble
Pascal [12..14,16] des rencontres aléatoires. Les campagnes C et leurs variantes
avec sauvegarde retrouvent maintenant les traces 2d0ca4bb et b246cd73, scores
8 et 10 ; les anciens chiffres de cette chronologie sont historiques.

**23/23 suites C ordinaires exécutées** et **5/5 suites natives sélectionnées**
réussissent. Une suite spatiale reste exclue pour l'alerte. Le périmètre est
deux graines, difficulté zéro et son coupé, depuis le monde généré commun
avant création jusqu'à la victoire. Démos, chargement/sauvegarde DOS,
graphismes/audio et toutes les variantes non parcourues ne sont pas certifiés.
Voir `validation_teramain_dos.md` pour les preuves et les abstractions exactes.

## Test spatial reconstruit et réintégré — 16 octobre 2026

À la demande explicite de l'utilisateur, `test_space_c.exe` a été reconstruit
et exécuté. Les tests du module spatial et des entrées passent ; la régression
complète est désormais **24/24 sans exclusion**. L'oracle natif des entrées
passe également. Aucun blocage observé ni modification de liste blanche.
Les journaux sont sous `build/audit/spatial-recheck-20261016/` ; l'audit de
sécurité distingue cette nouvelle construction du fichier initial détecté.
