# Comparaison globale TERAMAIN / moteur C — 16 octobre 2026

## Résultat et portée

**CONFIRMÉ — BIN/C sur deux parcours complets** : les graines 1986 et 12345
atteignent les quatre objectifs et la victoire dans une machine x86-16 exécutant
TERAMAIN et ses véritables overlays, en parallèle du moteur C. Les mêmes touches
alimentent les deux exécutions. Les états comparés concordent aux **3 990
frontières stables**, score Real48 final compris. Aucun recalage de graine ou
de données de jeu n'est effectué après l'initialisation.

Ce lot remplace la réserve « aucune comparaison globale de TERAMAIN au DOS »
pour ces parcours, difficulté zéro, son désactivé. Il ne signifie pas que toutes
les graines, difficultés, branches de menu et interruptions possibles ont été
énumérées. Les bancs de modules et d'entrées restent complémentaires.

| Graine | Touches communes | Frontières comparées, fin incluse | Tirages DOS exécutés | Chargements d'overlay | Successions | Score |
| --- | --- | --- | --- | --- | --- | --- |
| 1986 | 3 886 | 2 182 | 140 790 | 807 | 5 | 8 |
| 12345 | 3 257 | 1 808 | 96 823 | 533 | 3 | 10 |

Au total : **7 143 touches**, **237 613 vrais tirages DOS**, **1 340 chargements
d'overlay**, huit successions et les douze identifiants de monde parcourus.
Le conducteur ne téléporte pas les personnages, n'attribue pas de fonds, ne pose
pas les objectifs et ne recharge pas une partie antérieure après une défaite.

## Banc C et indépendance de l'oracle

Les fichiers `tests/teramain_oracle.c` et `.h` ajoutent un oracle natif au
conducteur C existant `tests/test_campaign_c.c`. Le symbole de compilation
`TERA_BINARY_CAMPAIGN` l'active uniquement dans `test_teramain_binary_c`.
Les campagnes C ordinaires ne dépendent toujours ni d'Unicorn ni de Python.
L'oracle utilise l'API native C d'Unicorn ; aucun interpréteur Python n'est lancé.

Un monde généré avec les ressources originales locales est fourni **une fois**
aux deux versions. La génération elle-même n'est pas rejouée dans cet oracle :
ses campagnes BIN précédentes constituent une preuve distincte. Le démarrage
DOS se fait à DBAC, après génération, avant la création du héros. L'écran titre,
la démo, le menu de chargement et le conteneur de sauvegarde DOS sont hors portée.

Le COM est chargé à 0100. Le corps relogé de TERAMAIN est copié à DA21 comme
dans le programme original. Lors d'un appel à BA0F, DX sélectionne l'overlay et
AX sa longueur ; ses octets réels de TERA.000 sont installés à BA21, le cache
de traduction est invalidé puis l'exécution reprend à BA28. La mémoire et la
pile sont conservées pendant tout le parcours, y compris après une succession.

Les overlays exécutés sont : 0022, 002E, 0049, 0069, 0085, 00A4, 00AE,
00BC, 00CB, 00EC, 00F0, 00F3, 00FB, 0142, 0145 et 0148. Ils couvrent création,
Amarande, recrutement, commerce, Cité/Arioch, Seth, Alfol/médias, Meduz et fin.
Les combats, rencontres, déplacements, soins, niveaux, récompenses, mots et
calcul du score exécutent leurs instructions originales.

### Frontières et champs comparés

Après chaque touche, le DOS avance jusqu'à sa prochaine lecture ou fin. Chaque
retour READY du C doit correspondre à une vraie frontière de commande DOS,
identifiée par le site d'appel, et non à une lecture de confirmation/combat.
La lecture brute des bâtiments d'Alfol, CD09, a son adaptation clavier dédiée.

À ces frontières sont comparés :

- graine 32 bits, monde, position et précédente position ;
- argent, expérience, niveaux, jours, énergie et compteur d'objectifs ;
- les 7 700 octets des cartes, cartes possédées, aides et quatre objectifs ;
- noms, états, appartenance au groupe, HP/MP et maxima des dix personnages,
  caractéristiques, compétences, équipement et clés ;
- placements de PNJ/rencontres supplémentaires, villes, indices et mots ;
- OnAlfol, son, marche rapide, extension mentale, disponibilité du groupe,
  arrivée/fuite, jour forcé, suppression de rencontre, recrutement et contexte
  d'information ;
- position de retour AFD5 selon le lieu actif (Cité/Seth, Alfol ou Meduz).

À la fin, les **six octets Real48** calculés par le runtime Pascal sont comparés
à `score_real48` du C, avant toute conclusion sur le score affiché. Un écart de
champ, de frontière ou de fin fait échouer immédiatement le test, avec adresse
DS, dernière région, pile et historique des touches. Le digest C est seulement
un résumé de régression ; il ne remplace pas ces comparaisons directes.

### Abstractions déclarées

Les hooks remplacent uniquement les I/O et les sorties matérielles identifiées :
clavier BIOS, temporisations, dessins, copies/chargements de bitmaps, texte et
ondes sonores déterministes sans RANDOM. Les appels sonores qui consomment le
RNG restent exécutés lorsqu'ils sont atteints. Les sites d'acquittement sont
identifiés dans le DOS, sans lire les phases C pour piloter les branches DOS.
Les nettoyages de pile Pascal sont explicités dans le banc.

Le tir spatial reçoit une touche au poll final ; les purges précédentes voient
un clavier vide. Le monstre 15 n'est pas interrompu. Le banc spécifique
`test_binary_input_c.c` compare séparément les cas horodatés et interruptions.
Les pauses purement visuelles sans équivalent de commande sont abstraites.

L'overlay de sauvegarde DOS 00D2 est ignoré ; aucun fichier original n'est écrit.
L'exécution s'arrête à la frontière terminale E7C0 avant cette sauvegarde et la
restauration du mode texte. Ce lot n'est donc pas un émulateur DOS complet ni
une preuve du rendu/audio du futur contexte 4.

## Deux divergences trouvées et corrigées

### Inscriptions de la Cité : 96 tirages manquants

Dans OVL0049, les contenus 104..113 appellent BA2F, qui appelle C724A à BAB6
(retour BAB9). Le dessin consomme 48 paires R2/R3. Les contenus >113 hors 255
utilisent C7309, avec le même calendrier de tirages. Le C ne conservait ces
tirages que pour la seconde branche. Au premier écart, graine 1986, touche 273,
position 194, contenu 111 : le C consommait trois tirages, le DOS 99.

`navigation.c` conserve maintenant ces 96 tirages pour les deux branches.
`test_inscription_rng` couvre les contenus 104..114 et compare le calendrier
avec un contrôle dont la graine commence après le dessin, sans combat parasite.

### Rencontre aléatoire : intervalle Pascal mal interprété

À A1C3..A1D6, le runtime construit l'ensemble **[12..14,16]** : C1515 ajoute
l'intervalle 12 à 14, puis C150E ajoute 16. Le C excluait seulement 12, 14 et 16.
Il pouvait donc déclencher un combat de type 13 que le DOS ignore après R17.

`exploration.c` exclut désormais aussi 13, sans retirer le tirage de sélection.
`test_excluded_random_monster13` utilise la graine 15595 : R20=0, R17=12,
R7 différent de 1. Le jour termine après trois tirages, sans combat.

Ces corrections expliquent le changement des anciennes traces C, scores et
nombre de successions. Les anciennes traces documentées restent des jalons
historiques et ne doivent plus servir de référence actuelle.

## Reproduction et preuves

Configurer le banc facultatif comme dans `spec_entrees_gameplay.md`, avec une
installation native Unicorn Windows 64 bits. Les SHA-256 des deux originaux
sont vérifiés à la configuration CMake, et conservés dans le manifeste local.

```powershell
cmake --build reconstruction/build/native-oracle --target test_teramain_binary_c
ctest --test-dir reconstruction/build/native-oracle -R '^teramain_binary_' -j 2 --output-on-failure
```

Les preuves locales sont sous `build/audit/teramain-20261016/` : journal natif,
journal de régression C, manifeste d'empreintes et résumé JSON. Aucun original
ni dépendance tierce n'est redistribué dans les sources.

Les 23 suites C ordinaires exécutées passent, y compris les campagnes avec
sauvegarde/reprise. Le test `space_module_contracts` de l'alerte antivirus reste
exclu et son exécutable n'a pas été restauré ou reconstruit. Cette comparaison
ne constitue pas un nouveau verdict de sécurité sur ce fichier.
