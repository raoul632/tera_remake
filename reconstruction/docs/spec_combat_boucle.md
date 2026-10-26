# Tour intégré, fuite et récompenses — lot 4

**CONFIRMÉ BIN + exécution différentielle**, pour les frontières décrites.
Nouveau port : `engine/src/encounter.c`, API `tera/encounter.h`. Le Python
historique reste inchangé. Les adresses sont celles du COM chargé à 0100.

Ce document conserve le périmètre numérique du lot 4. Le lot 5 a depuis
intégré les frontières interactives, le calendrier RNG et les cartes :
voir [le combat interactif](spec_combat_interactif.md). L'API de rejeu
ci-dessous demeure disponible pour les tests de tours isolés.

## Tour intégré

`tera_combat_execute_round` combine les ports des lots précédents :
préparation, attaques dans l'ordre des IDs, riposte, marquage HS, recalcul
des maxima, drapeaux de fin et décision de continuer. L'état conserve les
personnages, leurs attributs, le monstre et les drapeaux. Un rapport donne
la trace des actions/riposte et les demandes de tirages de cible/poison.

Les choix et résultats bruts de C_842A sont fournis comme entrées de
rejeu. Cette API numérique ne réalise pas encore la saisie interactive
ni le calendrier complet des appels RNG de C_842A. Un frontend ne devra
pas fabriquer tous ces jets à l'avance : il devra les demander lorsque
la frontière correspondante est atteinte. Le RNG original reste à porter.
L'agilité de `actors` doit correspondre à Stats[7] dans `attributes`.

Séquence portée, suivant 9977–9AAB et C_9419 :

1. Préparer la liste du tour et régénérer la magie.
2. Groupe premier : ses actions, puis victoire si HP monstre <3 ; sinon
   sélection de cible et riposte, **même si AFCF a été posé par une fuite**.
3. Monstre premier : sélection et riposte ; puis actions si le drapeau
   A745 antérieur est vrai. Les HP de la cible ne sont pas un nouveau
   filtre : HS n'est pas encore mis à jour à cette frontière.
4. C_9419 teste les HP après les trois premiers emplacements ; après le
   quatrième, le parent teste le seuil. Les actions suivantes sont
   interrompues par victoire/fuite, selon les gardes originales.
5. C_9567 met A745 à faux puis applique C_7CC8 à chaque membre du tour.
6. C_4D46 recalcule les maxima via C_4C6C pour les quatre premiers actifs
   affichés, HS inclus. Le C conserve les maxima HP et MP recalculés.
7. Continuer si A745 vrai, A746 faux et AFCF faux ; sinon sortir.

F dans un paquet de rejeu signifie une fuite **déjà confirmée**. Cela
pose le drapeau, sans demander une nouvelle confirmation ni choisir une
position dans ce tour numérique. La relocation est une API séparée.
Les entrées invalides sont rejetées sans demi-tour appliqué : calcul sur
une copie locale puis publication, contrat technique de l'API moderne.

## Relocation après fuite : C_86E1

Le menu pose AFCF et AFD1 après confirmation positive (897C–898B), puis
appelle C_86E1. Les variables sont recoupées avec TERA.PAS : WorldId à
9B76, Position à 75D2, carte à 75EE, OnAlfolFlag à A748.

Pour WorldId 0..10 :

```
répéter :
    Position = WorldId * 100 + RANDOM(90) + 5
    D_9B72 = Position - 1
jusqu'à Maps[Position,7] != 0
```

Le test est bien sur la septième colonne (8725), sans supposer une règle
générale de déplacement pour toutes les cartes. Chaque refus consomme
un tirage. Le C reçoit les colonnes 7 et une suite de valeurs, sans
remplacer la répétition par un tirage uniforme dans des cases filtrées.

Pour WorldId 11 :

```
si OnAlfolFlag : Position = RANDOM(99) + 1
sinon : Position = 3 * RANDOM(255)
```

Aucun test de carte et aucune écriture D_9B72 dans ces deux branches.
Les anciennes valeurs de D_9B72 sont donc conservées. Sons/messages et
écriture A679 de présentation ne sont pas produits par le moteur C.

## Récompenses : 9AAE–9B42

Le test est **victoire ET groupe encore apte** (A746 AND A745), sans test
supplémentaire AFCF. Si vrai :

```
argent = signed16(argent + RANDOM(50) + 2)
expérience = signed16(expérience + HP_initiaux_monstre)
```

Les noms Argent/ExpPoints à 75D4/75D6 sont confirmés dans TERA.PAS.
Les HP initiaux proviennent du cadre parent `[BP-1C]`, copie du tirage de
création du monstre, pas des HP restants ni des dégâts infligés.
Sans survivant, même si le monstre est vaincu, aucun tirage de récompense.
Le port conserve les débordements 16 bits. Montée de niveau et progression
après ces compteurs ne sont pas affirmées validées par cette fonction.

## Validation exécutée et limites

```
python reconstruction/tools/compare_encounter_binary.py
python reconstruction/tools/compare_combat_exit_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

880 tours intégrés sans écart : 22 monstres, groupes de 1..4 membres,
deux priorités et cinq scénarios (échec, coups modérés, victoire, fuite,
cible mortellement touchée/empoisonnement). Comparaisons : trace des appels,
HP/MP/maxima/états de tous les personnages, HP monstre, victoire/fuite,
aptitude du groupe, continuation et demandes de tirages cible/poison.

118 sorties sans écart : 24 récompenses et 94 fuites, tous les mondes,
deux drapeaux Alfol, tirages extrêmes, candidats refusés, conservation
D_9B72 et débordements des compteurs. Les quatre suites CTest passent.
Avec les lots précédents : **7 610 cas différentiels distincts couverts**.

Le banc intégré sépare CS et DS, car les adresses de données et de code
se chevauchent dans l'organisation du programme. Il exécute C_9419,
C_8CE3/C_8B5E, C_92FE/C_8C45, C_9567/C_7CC8 et le recalcul de C_4D46.
Les routines de chaînes et d'ensembles Pascal restent exécutées. Il part
à 9757 et s'arrête au retour à 9757 ou à 9AAE, avant les récompenses.

Frontières remplacées : menu accepté et sortie brute de C_842A, RANDOM
imposé, affichages des lots antérieurs, puis 7D36→7D8D, 4D6B→4D9C,
4DC2→4DFC et 4E03→4EC3 pour les présentations de fin de tour.
C_4C6C est conservé. La fuite du tour pose le drapeau mais ne déplace pas
le joueur dans ce banc ; C_86E1 est testé séparément.
Pour les sorties : présentation 9AC3→9AE6, 9AF2→9B0F, 9B18→9B3C et
8765→8783 contournée. Rapports locaux : `build/audit/encounter_comparison.json`
et `build/audit/exit_comparison.json`, exclus des commits.

Restent à intégrer/valider : entrée du combat et tirage de HP initiaux,
menu réel (passer, validations, annulations), ordre intégral des tirages,
RNG, effets des sous-appels de présentation, combats successifs dans une
partie, recrutement et progression. Un tour numérique validé ne démontre
pas encore une partie jouable fidèle de bout en bout.
