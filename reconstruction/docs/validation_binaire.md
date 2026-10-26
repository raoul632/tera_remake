# Comparaison exécutée avec TERA.COM

Les campagnes sont présentées par lot. L'état courant des dialogues et
de Kronopolis et Seth figure dans les lots 17/18 en fin de document ; les
anciennes listes de blocs manquants décrivent leur jalon historique.

Le banc `tools/compare_combat_binary.py` exécute les instructions originales
en x86 16 bits avec Unicorn et compare leurs sorties à la bibliothèque C
compilée. Il ne retraduit pas les formules BIN en Python pour fabriquer
l'oracle. Le Python prépare les entrées, la mémoire, la pile Pascal et lit
les sorties ; `tests/binary_comparison_runner.c` appelle les fonctions C.
Le processus C séparé permet d'utiliser le GCC 32 bits du poste depuis
Python 64 bits. Référence de l'API :
[documentation officielle Unicorn](https://www.unicorn-engine.org/docs/tutorial.html).

## Exécution reproductible

Depuis la racine du dépôt, avec les originaux présents localement :

```powershell
python -m pip install --target reconstruction/build/emulator_deps -r reconstruction/tools/requirements-emulator.txt
cmake -S reconstruction -B reconstruction/build/cmake -G "MinGW Makefiles"
python reconstruction/tools/compare_combat_binary.py
```

L'installation de la dépendance reste sous `build/`, sans modifier
l'environnement Python global. Aucun octet original n'est recopié dans
un fichier destiné aux commits. Le script vérifie l'empreinte SHA256 de
TERA.COM avant toute exécution ; une autre version interrompt le test.
VALMON fournit localement les 22 lignes utilisées dans les cas de réaction.
Le rapport JSON est écrit dans `build/audit/binary_comparison.json`, exclu
de Git. Les erreurs, limites d'instructions et divergences font échouer le
script ; il vérifie aussi le retour et l'équilibre de la pile Pascal.

## Périmètre comparé

| Bloc | Instructions originales | Sorties / contrôles comparés |
|---|---|---|
| Neuf actions | C_842A, appels originaux C_8315/C_8215/C_82E5/C_8391/C_8270 | Résultat signé ; tirages consommés et bornes autorisées |
| Jet de base | C_8315 et sous-appels | Résultat signé, seuils 44/45 |
| Équipement | C_8391/C_8270 | Bonus, absence, qualité et ordre des emplacements |
| Attaque du personnage | C_8CE3/C_8B5E | Admissibilité, dégâts, magie restante, HP du monstre |
| Riposte | C_92FE/C_8C45 et routines d'ensemble/chaîne Pascal | Dégâts, nombre de tirages d'empoisonnement, état EM |

Les entrées couvrent chaque action, les bonus 0..3, plusieurs facteurs et
profils, ainsi que des cas pseudo-aléatoires reproductibles (seed du banc
1986). Ils incluent les sept lettres contre les 22 monstres, magie 2/3/5/6/9,
mot correct/incorrect et résultats d'action positifs/nuls/négatifs. Des
valeurs synthétiques extrêmes complètent les données de jeu pour vérifier
les débordements signés ; leur atteignabilité en partie n'est pas affirmée.

## Frontières remplacées, explicitement

Le COM est chargé en mémoire de test ; il n'est pas modifié sur disque.
CS/DS/SS sont utilisés dans un espace de test commun, avec des zones
distinctes pour les cadres et données utiles ; aucun chargeur DOS n'est
exécuté. Chaque cas repart d'une image neuve, sans état hérité.

- RANDOM à 10DA reçoit une suite imposée. L'appel et sa borne sont relevés ;
  une demande inattendue ou une valeur hors borne provoque une erreur.
  L'algorithme RNG et les parties obtenues à partir d'une seed ne sont pas
  validés par cette méthode.
- Pour C_8CE3, le menu est contourné à 8D20–8D29 ; les données choisies sont
  placées dans le cadre parent. C_842A est testé indépendamment.
- Pour C_92FE, C_842A reçoit directement une défense brute imposée.
  Le choix physique/mental et sa division sont exécutés dans le binaire,
  mais cette comparaison n'est pas un test intégré de toute la riposte
  avec génération de la défense.
- Les intervalles 8B6A→8B70, 8B8A→8B9A, 8BAC→8C33,
  8C1E→8C33, 8CEF→8D20, 9298→92EF, 9364→9410 et 9400→9410
  contournent les séquences de présentation et de temporisation.
  Le rapport indique combien de fois chaque contournement intervient.
  Les effets de leurs sous-appels ne sont pas validés dans ce banc.
- La comparaison des mots et l'affectation de la chaîne EM utilisent les
  routines Pascal originales. Les mots fournis sont des entrées de test,
  pas une validation de la saisie clavier originale.

## Résultat et portée

Campagne de calculs : **6 222 cas, aucun écart**, exécutée avec Unicorn 2.1.4.
Répartition : 524 actions, 122 jets de base,
40 cas d'équipement, 4 656 réactions et 880 ripostes. Les deux suites CTest
passent également. Le rapport local conserve les compteurs et les limites.

Cette preuve d'exécution complète les preuves statiques BIN et les tests C.
Elle confirme les sorties numériques couvertes et certains effets mémoire
dans les conditions décrites. Elle ne certifie pas le combat complet ni la
fidélité du jeu : entrée/saisie, initiative, fuite, cible, tour complet,
récompenses, rendu, temps et progression restent hors de cette campagne.
Le booléen C `defeated` reste vérifié par les vecteurs de seuil et le
désassemblage C_9419 ; cette routine n'est pas exécutée dans ce banc.


## Combat interactif et RNG executes (lot 5)

Les injections des premiers bancs ci-dessus ne decrivent pas le nouveau
banc `tools/compare_interactive_binary.py`. Celui-ci charge CS et DS
separement et execute C_838A jusqu'a sa sortie, avec C_8790, C_4800,
C_42AC, C_4473, C_842A et RANDOM 10DA/10E6 reellement executes. Le Python
ne calcule pas d'attaque, de defense ou de tirage pour cet oracle.

Seules les touches sont fournies a C_4277 (lecture BIOS modernisable).
Les sorties video, texte, son et delais sont abstraits. Les hooks runtime
respectent la pile Pascal : textes fixes, chaines temporaires de longueur
variable et arguments par registre restent distingues. Les intervalles de
presentation des bancs precedents restent sautes, ainsi que la presentation
des recompenses et de la fuite ; aucune formule HP/MP n'est sautee.
C_4473 execute son RANDOM(3) meme quand l'affichage est remplace.
Une limite d'instructions/temps sans frontiere prevue fait echouer le banc.

**720 rencontres, 4 934 frontieres clavier, 770 cas de RNG, aucun ecart.**
718 rencontres finissent ; deux traces s'arretent a la fin des touches
fournies, sans fabriquer une issue. La graine, HP initiaux et courants,
HP/MP/maxima/etats des dix personnages, argent, experience, Position, 9B72,
AFD1, AFCC, disponibilite, victoire, fuite, contexte du menu et nombre de
tours sont compares. Tous les types 1..22 et tous les choix sont couverts,
avec groupes 1..4, priorites, refus/confirmation Entree, mots, retour arriere,
troncature a neuf caracteres et cartes clairsemees. La campagne de fuite
inclut **1 932 candidats rejetes** dans les mondes 0..10 et les deux branches
OnAlfol du monde 11. Les cartes sont des fixtures, jamais des originaux
redistribues. Les veritables cartes peuvent etre fournies par le frontend.

Les 770 vecteurs RNG couvrent graines 32 bits avec retenues/debordements,
les bornes du combat et 32768/65535. RANDOM n'est jamais injecte dans ce
banc. RANDOMIZE a aussi ete relu a 0F14 : CX et DX de l'horloge DOS sont
stockes directement, sans melange ; un test C verifie l'encodage.

```powershell
python reconstruction/tools/compare_interactive_binary.py
python reconstruction/tests/test_combat_frontend.py
```

Rapport local : `build/audit/interactive_comparison.json`. Les trois tests
frontend verifient les offsets du bloc tSavedata avec des sentinelles,
une fuite utilisant une unique case admissible fournie par JSON,
la preservation du fichier d'entree et le refus d'exporter un combat
interrompu. Cinq suites CTest passent, dont le contrat de la machine a
etats interactive. Cette validation certifie ce bloc de regles et ses
entrees/sorties ; elle ne demontre pas le rendu CGA ni une partie TERA
entiere de bout en bout. Voir `spec_combat_interactif.md`.

## Personnages et création du héros (lot 6)

Le banc `tools/compare_creation_binary.py` ajoute l'exécution des overlays
originaux 0010 et 0022. Il compare les deux régions personnages de 0010
(initialisation et équipement final), sans injecter de tirages, puis le
parcours interactif 0022 entier pour ses règles et mutations de l'état.
Le chargeur overlay DOS et les sorties de présentation sont abstraits.
Les touches sont fournies à C_4277 ; C_42AC/C_4418, C_4C6C/C_4CF9 et
RANDOM sont exécutés. Le binaire et le C reçoivent les mêmes états et graines.

**662 scénarios, 11 128 frontières clavier, 56 222 appels au RNG original,
aucun écart.** 564 créations validées et 98 traces partielles annoncées.
Noms, caractéristiques, compétences, équipement, groupe, HP/MP/maxima,
états, clés, argent, XP, jours, niveaux, énergie, position, monde, A74F,
disponibilité et graine sont comparés à chaque touche et à la fin.

Le calendrier géographique entre les deux régions personnages de 0010
n'est pas exécuté par ce banc. Cela ne valide donc pas une génération
mondiale ni son lien RNG complet avec le héros ; cette frontière est
documentée dans `spec_creation_personnages.md`. Aucun monde ou indice
inventé n'est exporté avec les personnages.

```powershell
python reconstruction/tools/audit_creation_binary.py
python reconstruction/tools/compare_creation_binary.py
python reconstruction/tests/test_creation_frontend.py
```

Les quatre tests frontend de ce lot vérifient un héros neuf sans arme
ni compétence, le profil des compagnons, la recréation conservant
équipement/argent, les refus successifs, l'intégration au combat préservant
les cartes/H/mots fournis, et l'absence d'export en cas d'interruption.
Six suites CTest passent. Rapports sous `build/audit/creation_comparison.json`
et `creation_manifest.json`, sans redistribution des originaux.


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

## Lots 11 et 12 — cité et interactions des niveaux (14 octobre 2026)

Les overlays originaux 002E/0049 sont exécutés avec C7EF6/C7DBD,
C9B4E, C838A, C_A0DD et les replis de commandes génériques. Les touches
sont injectées aux frontières originales. Les règles, cartes et RNG ne
sont pas remplacés par des formules Python. Les corps TERAMAIN sont
exécutés après la relocation 1FFC, sans émuler le chargeur DOS complet.
Voir `spec_navigation_cite.md` pour les adresses et comportements.

Les anciens sauts d'affichage supprimaient des tirages de son/couleur :
ils ont été corrigés, ainsi que le C. Les instructions RANDOM de ces
routines sont maintenant exécutées, y compris avec le son actif et avec
le flag A74D de pas rapides. Seules les sorties matérielles et délais
sont abstraits. Les campagnes antérieures de combat interactif et
d'exploration sont donc remplacées par ce nouveau périmètre. Les autres
bancs de formules restent des vérifications numériques distinctes.

- `compare_navigation_binary.py` : **875 cas, 1 530 frontières,
  1 237 466 appels au RNG original, aucun écart** : orientations, murs,
  portes/clés, décors, D/M/L/I et replis, mots, 22 monstres fixes,
  victoires/objectifs, profils d'autel, portail et pas rapides/silencieux.
  L'entrée et le retour sont vérifiés séparément par le banc suivant.
- `compare_city_entry_binary.py` : **51 cas, 99 frontières, aucun écart**.
  Confirmation/refus E, backup conservé au refus, premier rendu complet
  et retour réel vers trois positions de surface.
- `compare_interactive_binary.py` : **946 combats, 6 836 frontières,
  770 vecteurs RNG, aucun écart**. 944 fins et deux interruptions
  volontaires ; 154 nouveaux cas de sons actifs, tous types/actions.
- `compare_exploration_binary.py` : **854 cas, 900 frontières,
  934 751 appels au RNG original, 23 rencontres, aucun écart**.
  Le corpus ajoute 56 positions basses issues de la fuite. Les nombres
  de rencontres/frontières ont changé avec la restauration du RNG sonore,
  sans modifier les fixtures pour reproduire les anciens résultats.

**9 suites CTest et 20 tests frontend passent.** Les tests d'intégration
couvrent vraie génération/création, entrée annulée, retour, portes et
clés, victoire et objectif, export/rechargement, conservation de la
source et interruption sans export. Les profils renforcés de tests de
victoire sont explicitement synthétiques ; ils ne sont pas des héros
distribués ou un équilibrage du jeu.

```powershell
python reconstruction/tools/audit_navigation_binary.py
python reconstruction/tools/compare_navigation_binary.py
python reconstruction/tools/compare_city_entry_binary.py
python reconstruction/tools/compare_interactive_binary.py
python reconstruction/tools/compare_exploration_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
```

Rapports locaux : `navigation_comparison.json`, `city_entry_comparison.json`,
`interactive_comparison.json`, `exploration_comparison.json` et manifeste
`navigation_manifest.json`, tous sous `build/audit/`. Empreintes des sources
vérifiées avant les campagnes ; aucun original n'est redistribué.

**MANQUANT** : coffres/pièges/inventaire, autres lieux/dialogues et mondes
9/10, Alfol/Meduz, voyages, autres objectifs, fin et recréation dans la boucle
générale. **À VÉRIFIER** : animations interrompues au clavier.
**MODERNISABLE** : CGA/audio matériel, encore remplacés par le terminal.
Ces campagnes ne constituent pas une validation d'une partie entière.

## Lots 13 et 14 — coffres/pièges et inventaire (14 octobre 2026)

**CONFIRMÉ — BIN/PAS/DATA** : O/P dans 002E/0049, CAA13 (acquisition),
CAB40 (abandon), CAD08 (transfert), CAE4F (boisson), C4B55 (personnage)
et C4418 (slot). Le banc exécute ces instructions, les états et le RNG,
sans remplacer les pièges, soins ou transferts par du Python. Les sorties
vidéo/audio matérielles et délais sont abstraits, leurs tirages conservés.
Voir `spec_coffres_inventaire.md` pour les règles et limites exactes.

| Campagne | Cas | Frontières | Appels RNG original | Écarts |
|---|---:|---:|---:|---:|
| Coffres/pièges | 1 076 | 2 460 | 23 922 | 0 |
| Inventaire/équipement | 632 | 1 727 | 232 | 0 |
| Navigation relancée | 875 | 1 530 | 1 237 466 | 0 |
| Entrée/retour relancés | 51 | 99 | — | 0 |

Les snapshots incluent HP/MP/maxima/états, dix inventaires, argent/clés,
HasMap, BP_03, cartes et graine ; ils sont comparés à chaque lecture et
au retour. Le contenu/roster et les mêmes touches sont fournis aux deux
exécutions. Les captures, empreintes et rapports sont dans `build/audit/`
(`inventory_chest_manifest.json`, `chest_comparison.json`,
`inventory_comparison.json`). Les originaux restent en lecture seule.

Les pièges couvrent gaz/explosion/cyanure/lame, désamorçage, son actif/coupé,
HS/inactifs, groupes discontinus et débordement d'argent. L'inventaire
couvre dix positions de slot libre/plein, acquisition auprès d'un HS,
transfert vers soi-même ou une cible pleine/HS, abandon, potions 0..25,
reprises sur touches invalides et profils variés. Le contexte forcé
« BP_03 vrai sur marqueur ≥100 » est impossible dans un parcours normal :
le C le refuse, et un contrat vérifie l'absence de mutation ; le DOS
écrirait hors de Keys. Il est exclu des scénarios de coffres valides.

**10 suites CTest et 26 tests frontend passent.** Six nouveaux parcours
vérifient ouvrir/prendre/exporter/recharger, fermeture au déplacement,
refus inventaire plein puis abandon/prise, potions distinctes, transfert
refusé puis réussi, et interruption du lecteur de chiffre sans export.
Le test de victoire antérieur quitte exactement à la dernière frontière
de combat : des O supplémentaires seraient maintenant des commandes
réelles de coffre. Les règles n'ont pas été changées pour ce test.

```powershell
python reconstruction/tools/audit_inventory_chest_binary.py
python reconstruction/tools/compare_chest_binary.py
python reconstruction/tools/compare_inventory_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
```

Le frontend permet ces objets dans l'exploration actuelle. Les autres
dialogues de N, lieux et objets propres à leurs événements, voyages,
autres objectifs/fin, recréation générale et graphique/audio audible
restent **MANQUANTS**. Les animations interrompues restent **À VÉRIFIER**.
Une partie entière n'est pas certifiée par ces seuls blocs.

## Lots 15 et 16 — dialogues et succession (14 octobre 2026)

**CONFIRMÉ — BIN/PAS/DATA** : commandes résidentes N/S entières,
condoléances OVL00CB, création OVL0022 et branches TERAMAIN de fin/reprise.
Le RNG x86 original s'exécute, y compris les mélodies. Les mêmes touches
et contextes sont transmis au C. Seuls chargement DOS, BIOS, affichage,
fréquences matérielles et délais sont abstraits. Le score victorieux
reste hors de cette campagne.

| Campagne | Cas | Frontières | Appels RNG original | Écarts |
|---|---:|---:|---:|---:|
| Dialogues N/S + arrivée PNJ | 466 | 1 280 | 80 905 | 0 |
| Défaite/successeur TERAMAIN | 120 | 1 911 | 75 092 | 0 |
| Navigation relancée, A750 inclus | 887 | 1 542 | 1 271 822 | 0 |
| Exploration relancée, A750 inclus | 854 | 900 | 934 751 | 0 |
| Entrée/retour relancés, A750 inclus | 51 | 99 | — | 0 |

Les snapshots comprennent le roster, noms, caractéristiques, compétences,
équipements, HP/MP/maxima/états, argent/clés, compteurs, position, graine,
AFCE/A750, IDs d'indices et variantes de réponses effectivement lues.
Le cycle couvre les seize combinaisons d'objectifs, son actif/coupé,
argent extrême, équipement conservé, interruption et refus de création.
Le chargeur d'overlays est abstrait ; la décision de recréer ou terminer
et l'affectation A748 sont exécutées dans TERAMAIN, pas injectées.

**11 suites CTest et 33 tests frontend passent.** Les sept nouveaux
parcours couvrent informations → séduction, réponses A/V sans transaction,
annulations, mort → successeur → arrivée → export/rechargement, mort en
cité, interruption sans export et arrêt avec objectifs déjà accomplis.
Le contrat C vérifie aussi la conservation des données du monde non
transportées par les ponts. Les fichiers sources restent inchangés.
Les 887 cas de navigation incluent douze contextes supplémentaires de
compagnons de cité. La comparaison de A750 a identifié puis permis de
corriger son activation lors de l'invocation d'Arioch ; la campagne entière
relancée passe sans écart. Exploration et entrée/retour conservent aussi
ce flag dans leurs snapshots et leurs rapports actuels.

```powershell
python reconstruction/tools/audit_dialogue_lifecycle_binary.py
python reconstruction/tools/compare_dialogue_binary.py
python reconstruction/tools/compare_lifecycle_binary.py
python reconstruction/tools/compare_navigation_binary.py
python reconstruction/tools/compare_city_entry_binary.py
python reconstruction/tools/compare_exploration_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
```

Rapports et manifeste dans `build/audit/` : `dialogue_comparison.json`,
`lifecycle_comparison.json`, `dialogue_lifecycle_manifest.json`.
**MANQUANT** : autres lieux/transactions/événements, mondes 9/10,
Alfol/Meduz, voyages, autres objectifs et fin victorieuse/score.
**MODERNISABLE** : graphique/audio audible. **À VÉRIFIER** : animations
interrompues et parcours complet. Aucune certification du jeu entier.

## Lots 17 et 18 — Kronopolis et Seth (15 octobre 2026)

**CONFIRMÉ — BIN/PAS/DATA** : OVL00BC complet, OVL0069 et entrée
TERAMAIN E57B, façades DCEF/OVL00F0, RNG, inventaire, soins et objectif 3.
Les instructions originales décident les prix, pièges, résultats et
mutations ; Python fournit les entrées et lit les frontières.

| Campagne | Cas | Frontières | Appels RNG original | Écarts |
|---|---:|---:|---:|---:|
| Commerce/hôpital | 512 | 3 385 | 492 | 0 |
| Temple/entrée Seth | 274 | 780 | 34 410 | 0 |
| Façades Kronopolis/Seth | 16 | — | 1 262 | 0 |
| Cité relancée | 887 | 1 542 | 1 271 822 | 0 |
| Coffres de cité relancés | 1 076 | 2 460 | 23 922 | 0 |
| Entrée/retour de cité relancés | 51 | 99 | — | 0 |

Le commerce couvre tout le catalogue, seuils et débordements d'argent,
plein/HS, ventes 0..25, confirmations/reprises/ESC et profils de soins
variés. Seth couvre portes et quatre orientations, entrée/refus, trésors,
quatre pièges avec/sans son, monstres fixes, défaite, mots et victoire,
objectif déjà accompli, débordement du compteur et inscriptions.
Les deux objectifs cité/Seth sont suivis séparément. BP_03 est comparé
dans son overlay ; il n'existe pas dans le prompt d'entrée TERAMAIN.

**11 suites CTest et 41 tests frontend passent.** Huit nouveaux parcours
couvrent achat/vente/rechargement, services sans slot, hôpital actif/inactif,
annulation et interruption sans export, entrée/lecture/retour de Seth,
ouverture/prise, entrée mortelle avec commande obligatoire après reprise,
invocation et objectif conservé. Les façades utilisent les tirages du C,
sans recalcul de prix ni règle de jeu Python. L'original reste inchangé.

```powershell
python reconstruction/tools/audit_commerce_seth_binary.py
python reconstruction/tools/compare_commerce_binary.py
python reconstruction/tools/compare_seth_binary.py
python reconstruction/tools/compare_places_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
```

Rapports locaux : `commerce_comparison.json`, `seth_comparison.json`,
`places_comparison.json`, manifeste `commerce_seth_manifest.json` sous
`build/audit/`, exclu de Git. Clavier/BIOS/bitmap/vidéo/délais abstraits ;
chronologie RNG conservée. Voir `spec_kronopolis_seth.md`.

**MANQUANT** : autres lieux/événements et voyages, mondes 9/10,
Alfol/Meduz, autres objectifs et fin victorieuse avec score.
**MODERNISABLE** : rendu graphique/son audible encore à connecter.
**À VÉRIFIER** : animations interrompues et parcours complet.

## Lots 19 et 20 — spatioport/Alfol (15 octobre 2026)

**CONFIRMÉ — BIN/PAS/DATA** : vraies instructions TERAMAIN E222 et
OVL00AE/0085, RNG exécuté sans injection de résultat. Les mêmes états,
touches et graines sont transmis au C ; personnages, compteurs, cartes,
indicateurs et premier texte d'indice sont comparés.

| Campagne | Cas | Frontières | Appels RNG original | Écarts |
|---|---:|---:|---:|---:|
| Vol spatial OVL00AE | 173 | 1 134 | 220 585 | 0 |
| Pilotage TERAMAIN | 120 | 235 | 1 171 | 0 |
| Alfol OVL0085 | 613 | 764 | 176 326 | 0 |

Vol : carburant, refus/Entrée, chiffres invalides/ESC, seuils d'argent et
d'énergie, débordements, touches correctes/incorrectes/absentes, pirates
0/1/4/15, sons et HALT. Pilotage : refus, absence de pilote, HS/inactifs,
dix jets, dernier succès et caractéristiques extrêmes. Alfol : cent
positions dans quatre directions, rendu d'arrivée, portes/orientations,
médias déjà payés/argent insuffisant/débordement d'objectif, radiation
sur dix indices et alias 16, combats fixes/fuites/victoires, PNJ 7/9,
invocation refusée, sortie et arrivée mortelle.

Les chargements bitmap, sorties vidéo, ondes sans RANDOM et délais sont
abstraits. Le test spatial fournit une touche au dernier KeyPressed ;
les purges antérieures voient un clavier vide. Le COM garde ses vraies
boucles, tirages, calculs de combat et mutations. Le test ne valide pas
le timing d'un vrai terminal ni le rendu du jeu complet.

```powershell
python reconstruction/tools/audit_space_alfol_binary.py
python reconstruction/tools/compare_space_binary.py
python reconstruction/tools/compare_alfol_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
```

Rapports locaux : `space_comparison.json` (dont `pilot_selection`),
`alfol_comparison.json` et `space_alfol_manifest.json` dans `build/audit/`.
Le snapshot moderne garde les drapeaux d'attente et HALT ; aucun export
partiel n'est publié après interruption d'une lecture/du combat.
Voir `spec_spatioport_alfol.md` pour les règles et limites restantes.

**11 suites CTest et 54 tests frontend passent.** Les treize nouveaux tests
couvrent aller/arrivée/retour après export, refus/sans pilote, HALT conservé,
vol interrompu sans export, surface circulaire, refus de R au point zéro,
médias (paiement répété, Entrée et EOF), porte/clé, irradiation et alias
mémoire, abandon de compagnon/invocation, mort d'entrée et successeur,
victoire sur monstre fixe. Les 632 scénarios d'inventaire et 466 de
dialogues sont également relancés pour vérifier le transport du global
D9B66 ajouté aux ponts existants.

## Lots 21 et 22 — Meduz et fin (15 octobre 2026)

| Campagne | Cas | Frontières | Appels RNG original | Écarts |
|---|---:|---:|---:|---:|
| Meduz OVL00A4 | 326 | 652 | 10 183 | 0 |
| Pyramide/voyage/retour TERAMAIN | 48 | 238 | 38 353 | 0 |
| Fin OVL00CB, Real48 et formatage | 510 | 1 014 | 4 856 | 0 |

**CONFIRMÉ — BIN/PAS/DATA** : les instructions décident mouvements,
combats, jours, troncatures, extension mentale et objectif. Les tests
comparent les personnages, graines, drapeaux et compteurs à chaque lecture.
Le voyage charge les vrais overlays 0145/A4/0142 par le hook du chargeur DOS.
Les calculs flottants originaux et le convertisseur texte 1F45 sont exécutés ;
les six octets du score et les chiffres imprimés sont comparés au C.

Meduz couvre quatre orientations, fosses bloquantes, combats forcés ou
R9, fuite/reprise, arrivée mortelle, refus, mots faux/corrects, retour
arrière et dixième lettre, extension déjà faite, octets 255, objectif déjà
fait et compteur 32767, invocation générique ARIOCH et retour en 999.
La fin couvre difficulté 0..9, expérience signée, jours négatifs et
débordement de jours−4000, scores nuls/négatifs, troncature Real48,
formatage décimal, victoire/mort, pause/EOF, son et division par zéro.

```powershell
python reconstruction/tools/audit_meduz_ending_binary.py
python reconstruction/tools/compare_meduz_binary.py
python reconstruction/tools/compare_ending_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
```

Manifestes/rapports locaux : `meduz_ending_manifest.json`,
`meduz_comparison.json` (dont `travel`) et `ending_comparison.json` sous
`build/audit/`. Seuls matériel vidéo/son/délais/clavier et écriture DOS sont
abstraits ; aucun jet ou résultat de combat/score n'est injecté dans l'oracle.
Les snapshots sont modernes ; l'écriture `scenario.*` n'est pas simulée.
Le parcours complet sur un monde généré reste à valider séparément.

**11 suites CTest et 67 tests frontend passent.** Treize nouveaux tests
parcourent confirmation/mot de voyage/aller-retour/export, méditation
active seulement et débordement d'octets, refus/mot incorrect, objectif
unique et conservé, fin différée jusqu'à la sortie du lieu, score/pause/
rechargement sans double musique, erreur 2, fin après mort sans successeur,
arrivée mortelle et recréation, invocation générique et pas bloqué avec jour.

## Lieux et événements secondaires d'Amarande (15 octobre 2026)

| Campagne | Cas | Frontières | Appels RNG original | Écarts |
|---|---:|---:|---:|---:|
| Dix façades d'Amarande | 320 | états finaux | 38 784 | 0 |
| Commandes résidentes et panneaux | 311 | 775 | 5 715 | 0 |
| Régression exploration/jour | 854 | 900 | 934 751 | 0 |
| Régression Alfol, D9B66 inclus | 613 | 764 | 176 326 | 0 |

**CONFIRMÉ — BIN/PAS/DATA** : les lieux 5..9 donnent accès aux commandes
résidentes, avec la condition OR originale de présence et A750. C parcourt
les PNJ et rencontres supplémentaires, I conserve les restrictions
d'ARIOCH, H conserve ses sous-commandes, jours et pauses, L conserve
les directions signées et # la confirmation du drapeau sonore.
Les ID/variantes/compteurs de réponses C4473 sont observés dans le COM
et comparés à la sortie du pont. Les textes sont lus dans MESSAGE/2/3.

Les façades exécutent le vrai dessin et ses RANDOM dans l'oracle ; leur
chargement bitmap DOS et les primitives vidéo sont abstraits. La carte et
les fiches sautent seulement le dessin : C4D46 reste exécuté, et l'effet
D9B66 de C5495 est explicite. Aucun résultat de combat ni de jour n'est
injecté. Le calendrier conserve D9B66 ; l'atterrissage Alfol laisse 16.

Preuves locales : `amarande_manifest.json`, `amarande_comparison.json`,
`amarande_facades_comparison.json` sous `build/audit/`. Voir
`spec_amarande_secondaire.md` pour les commandes, le périmètre et les limites.

**11 suites CTest et 78 tests frontend passent.** Onze nouveaux tests
couvrent ce bloc ; les sources historiques restent inchangées.

## Comparaison globale TERAMAIN native C — 16 octobre 2026

Les deux parties générées (1986 et 12345) sont désormais exécutées jusqu'à
la victoire avec TERAMAIN relogé et les vrais overlays, en parallèle du core C.
Résultat après correction de deux divergences : **7 143 touches communes,
3 990 frontières stables concordantes, 237 613 tirages DOS exécutés** ; les six
octets du score Real48 final concordent aussi. Les cartes et personnages ne
sont injectés qu'une fois, avant création ; aucun recalage n'est effectué.

Les inscriptions de contenus 104..113 consommaient 96 tirages manquants dans
le port. L'ensemble d'exclusion des rencontres est [12..14,16], donc inclut
aussi 13. Ces deux points sont corrigés et protégés par des tests C ciblés.
Les anciennes traces de campagnes C sont remplacées par les nouvelles traces
validées face au DOS. Voir `validation_teramain_dos.md` pour le périmètre,
les hooks, les résultats par graine et les commandes de reproduction.

Les deux nouveaux tests différentiels, l'oracle des entrées, la sauvegarde et
la régression navigation passent dans la configuration native : **5/5**.
Régression C ordinaire : **23/23 exécutées**, avec la suite spatiale de
l'alerte antivirus toujours exclue. Aucun Python n'est requis ou exécuté.
