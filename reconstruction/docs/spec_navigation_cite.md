# Lots 11 et 12 — cité des crânes et interactions des niveaux

État : **CONFIRMÉ — BIN/PAS/DATA**, pour le périmètre décrit ci-dessous.
Sources : TERA.COM, overlays 002E et 0049 de TERA.000, OVL_002E.PAS,
OVL_0049.PAS, TERAMAIN.PAS et ressources originales. Le Python historique
ne sert pas d'oracle. Les captures et empreintes sont reproductibles avec
`tools/audit_navigation_binary.py` et restent sous `build/audit/`.

## Lot 11 : navigation et rencontres

`engine/src/navigation.c` porte C7EF6/C7DBD et la boucle des deux overlays.
Le déplacement est relatif à Position−Previous : 8 avance, 2 recule,
6 va à droite et 4 à gauche. Les quatre orientations ±1/±10 sont conservées.
Un mur bloque sans tirage ni journée. Une porte demande O ; Entrée ne
l'ouvre pas. Une porte verrouillée demande U. La valeur de clé 10 interdit
l'ouverture ; une autre valeur transforme tous les murs 3 de la case
courante en portes 2, sans modifier la voisine ni consommer la clé.

Chaque vue consomme ses tirages de décor dans l'ordre original, rencontre
le monstre fixe de la colonne 6, exécute C_A0DD et tire RANDOM18, même en
silence. Le combat, les jours, les soins et les niveaux utilisent le même
état C. Une fuite est redessinée avant la prochaine commande ; elle peut
enchaîner une autre rencontre ou franchir le portail vers le niveau 1.
Une victoire retire le monstre à la position courante, même après fuite.

Correction après comparaison globale TERAMAIN : dans 0049, les inscriptions
de contenus 104..113 appellent C724A via BA2F et consomment **96 tirages R2/R3**,
comme les fenêtres >113 via C7309 ; 255 reste vide. Le port avait omis la
première branche. Voir `validation_teramain_dos.md` pour la divergence et sa
régression C ciblée.

Le monde et l'overlay sont deux états distincts. Monter du niveau 1 vers
le monde 0 conserve temporairement 0049. La sentinelle 99 rend la main à
002E ; la sentinelle 0 dans 002E termine la visite. La précédente position
est conservée exactement, sans la recalculer au rechargement. Le flag de
marche Amarande A749 est distinct du déplacement de grille A742.

## Lot 12 : entrée, retour et actions des lieux

Sur Amarande, E au lieu 2 demande O/Entrée, mémorise la position, puis
entre dans 002E en position 1, précédente 0. Refuser conserve l'ancien
backup. Le retour et la mort dans la cité restaurent la position de
surface mémorisée. La branche TERAMAIN est vérifiée après sa relocation
runtime de 1FFC ; les appels des overlays originaux restent exécutés.

Dans 0049, D sur 101 descend ; M sur 100 monte. Le retour recherche le
dernier marqueur 101 du niveau inférieur, comme le COM. L sur une case
104..254 lit les vrais indices ou MESSAGE70 après confirmation. I sur
l'autel 103 tire dix jets, y compris pour les inactifs/HS, et restaure HP/MP
des actifs aptes ayant réussi. Les maxima des quatre premiers actifs
sont recalculés, HS inclus ; EM reste EM. Ces interactions n'ajoutent
aucune journée artificielle.

Les replis du menu générique sont conservés : D/M annulés ou sans lieu
tirent RANDOM3 ; L annulé consomme la commande sans repli. I annulé ou
sans autel demande un mot. Le mot ARIOCH appelle le vrai monstre 21 hors
des dix villes et hors Alfol. L'invocation générique ne valide pas un
objectif et n'avance pas le jour, sauf après une fuite qui redessine la
case. Les lettres, retour arrière, Entrée, dix touches puis neuf caractères
stockés suivent le lecteur original.

La victoire à la position d'Arioch valide l'objectif par position, sans
test de type. 002E ne change pas le compteur ; 0049 l'incrémente une seule
fois, avec débordement signé 16 bits, et conserve ses effets sonores.

## Tirages de présentation qui affectent les règles

**CONFIRMÉ — BIN** : les anciennes abstractions du combat excluaient
certains effets contenant RANDOM. Le banc interactif exécute désormais
ces routines, en abstraisant seulement la sortie matérielle. Le C conserve :

- T armé : 600 RANDOM255 après le jet d'action et avant la phrase.
- L : 29 RANDOM20, 2 000 RANDOM255, 33 RANDOM20 ; P : 31 RANDOM25.
- C : trois séquences de 30 RANDOM160/160/320, 54 RANDOM3 et 71 RANDOM2.
- Coup réussi : 9 RANDOM30 puis 49 RANDOM20 si SoundFlag est actif.
- J réussi et riposte des monstres ≥15 : huit paires RANDOM4/RANDOM16,
  même si SoundFlag est coupé. Le code 10 du flash désigne l'action J,
  pas le type du monstre.
- C62B7 : durée−1 jets RANDOM20 ; sous le seuil, RANDOM2 puis RANDOM255/256.
  C6496/C6518 utilisent les durées 60, 30 et 120, seuil 5. L'entrée joue
  une séquence, 1 000 RANDOM255, puis une autre séquence. Une première
  validation d'Arioch dans 0049 ajoute trois séquences.
- La présentation des monstres 3/6/9 ajoute le bruit C62B7, durée 100,
  seuil 12, après leurs sprites, si SoundFlag est actif.
- Les pas C67D7 consomment 2 850 tirages (trois fois 500 RANDOM155 puis
  450 RANDOM110) avec `fast_moves=0`, ou 500 RANDOM155 avec `fast_moves=1`.
  Ils précèdent les décors. Le flag vient de la génération originale.
- SoundFx_A/C68AD consomme 60 RANDOM30, 60 RANDOM90 et 60 RANDOM270.
  Il accompagne la présentation des types 17/21, la validation d'Arioch
  et l'ambiance quand RANDOM18 vaut 4. Les types 16/18/19/20 utilisent
  C693B : 200 RANDOM255 puis 47 RANDOM15.
- Sur Amarande, les pas précèdent RANDOM24. Si ce jet vaut 1, 2 ou 3,
  C6352 consomme sept segments C62B7 de durées 10/5/15/10/5/15/30, seuil 7.

Les fréquences audio n'ont pas d'effet supplémentaire sur les règles.
L'absence de haut-parleur dans le terminal ne coupe pas le flag SoundFlag.
Les états de monde le transportent ; le banc de combat isolé admet aussi
`sound: 0` et `sound: 1` et garde 0 par défaut pour ses fixtures historiques.

## Transport, sauvegarde et vérification

`frontend/exploration_transport.c` fournit un transport partagé, sans
formules Python. `navigation_bridge.c` publie les personnages, le contexte
et les seules cases modifiées. La frontière Z garantit que les mutations
de carte sont appliquées avant la touche suivante. Le JSON conserve
`city_overlay`, `city_backup_position`, position/précédente, graine,
cartes/clés, personnages et objectifs. Q exporte le fichier moderne ;
ce n'est pas une reproduction du conteneur DOS. Une interruption clavier
ne publie pas d'état final. Le fichier d'entrée reste intact.

Le frontend `exploration.py` permet une partie générée → héros → Amarande
→ E → cité → retour. Les tests vérifient annulation, portes, sortie,
victoire et objectif, puis export/rechargement. Les campagnes différentielles
comparent les personnages, HP/MP/maxima, argent/XP, jours/niveaux, flags,
clés, objectifs, cartes et graine à chaque frontière clavier et au retour.
Le bilan chiffré à jour est dans `validation_binaire.md`.

Un défaut d'intégration de la fuite sur Amarande a aussi été corrigé :
le COM produit 3×RANDOM255, soit 0..762, sans ajouter 1 000. L'arrivée
automatique accepte donc −1 avant le pas nord. Le rebouclage inférieur
ne s'applique qu'aux pas sud/ouest, comme OVL00FB.

## Frontières restantes

**MANQUANT** : coffres O/P, objets/inventaire et pièges, autres entrées
des lieux, dialogues complets, déplacements des mondes 9/10, Alfol/Meduz,
voyages, autres objectifs et fin du jeu. Le moteur refuse une descente
forcée du monde 8 vers 9 ; les règles de ces autres overlays ne sont pas
présentées comme portées. La boucle générale de recréation après mort
reste à réaliser. Ces deux lots ne certifient pas une partie entière.

**À VÉRIFIER** : interruption clavier des animations, notamment monstre 15.
Le banc conserve le chemin sans interruption, comme le lot précédent.
Les autres overlays restent hors du périmètre ; une absence d'écart
n'étend pas la certification à leurs routines. Les erreurs DOS, pauses et chargeur complet ne sont pas
émulés. Une carte de fuite sans case admissible donne ERROR au lieu d'une
boucle infinie, sans inventer de destination.

**MODERNISABLE** : entrée BIOS, délais, sortie sonore et rendu CGA. Le
terminal conserve les règles et textes utiles ; la sortie graphique et
le son audible restent à connecter au moteur.
