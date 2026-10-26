# Préparation et frontières du tour — lot 3

**CONFIRMÉ BIN + exécution différentielle**, sur le même TERA.COM que les
lots précédents. Adresses de chargement COM = offset fichier + 0100.
Le port de ce lot est `engine/src/round.c`, API `tera/round.h`.
Il ne transfère aucune règle au Python historique.

## Maxima et effet de l'affichage : C_4C6C

C_4D46 appelle C_4C6C pour chaque personnage actif affiché. Cette routine
recalcule et écrit les maxima, sans modifier les valeurs actuelles :

```
HP maximum = Stats[1] + Stats[4] + Stats[9]
MP maximum = Stats[2] + Stats[5]
```

Force, endurance et habileté déterminent donc le maximum de vie ;
intelligence et volonté celui de magie. Les compétences n'interviennent
pas. Preuve : 4C73–4CEC, écritures dans 7504/7518 + 2*personnage.
Ce calcul doit rester dans l'orchestration lorsque le rendu DOS est
remplacé. Il a été porté et comparé à C_4C6C sur 103 profils, y compris
tous les octets à 0 ou à 255. Les états HP/MP initiaux complets et les
autres effets des sous-appels d'affichage ne sont pas validés par ce test.

## État HS : C_7CC8

Appelé au début du combat pour chaque actif, puis en fin de tour par
C_9567 pour les membres de la liste du tour. Les indices de personnage
sont **0..9**, contrairement aux indices de statistiques **1..9**.

```
si HP < 1 ET état != "HS" :
    HP = 0
    état = "HS"
si état != "HS" : drapeau groupe_apte = vrai
```

Preuves : 7CCF–7D33 pour la condition et l'écriture ; 7D8D–7DB3 pour le
drapeau A745. Un HS déjà marqué reste HS, même avec HP positifs ; ses HP
négatifs ne sont pas remis à zéro par cette routine. EM avec HP positifs
reste EM. Aucun dégât de poison n'est appliqué par C_7CC8.
Le C retourne l'aptitude individuelle ; l'appelant accumule le drapeau
du groupe comme le binaire. L'état est une chaîne conservée, sans supposer
que tous les états originaux ont déjà été recensés.

## Liste, magie et initiative : 9757–9977

La liste est reconstruite au début de chaque tour. Pour IDs 0..9, dans
l'ordre : retenir actif ET état différent de HS. Le binaire stocke jusqu'à
quatre IDs dans les variables du parent et conserve l'effectif séparément.
Le port rejette un effectif nul ou supérieur à quatre : ces entrées sont
hors des cadres valides audités. Aucun choix arbitraire de quatre membres
n'est introduit. L'API ne modifie rien lors de ce rejet.

Chaque membre récupère **1 MP si MP actuels < MP maximum**, avant de
décider qui attaque en premier. Au maximum, ou au-dessus, pas de changement.
L'agilité du groupe est la moyenne entière des membres, tronquée vers zéro.

```
initiative_monstre = signed16(VALMON[type,5] + RANDOM(5) - 2)
groupe_premier = moyenne_agilité > initiative_monstre
```

L'égalité donne donc la priorité au monstre. Chaque tour consomme un
RANDOM(5) pour cette décision. La colonne 5 de VALMON est maintenant
confirmée comme paramètre d'initiative. Preuves : 9801–994D pour somme,
régénération et moyenne ; 9954–9974 pour colonne, tirage et test strict.

## Choix de la cible : 9992–9A5D

Quand la riposte est atteinte, RANDOM(effectif) choisit un indice de la
liste du **début du tour**, puis le personnage correspondant. Il ne tire
pas directement un ID dans 0..9. C_92FE n'est appelé qu'une fois dans
chacune des deux branches de priorité.

La liste n'est pas reconstruite entre les attaques de ce tour. Ne pas
remplacer les tests d'état par un filtre immédiat sur HP : le marquage HS
est effectué à une autre frontière. `tera_combat_select_round_target`
consomme un indice fourni par l'appelant, sans génération aléatoire interne.

## Déroulé intégré dans le lot suivant

Lecture statique **CONFIRMÉ BIN**, puis port intégré et comparaisons
exécutées dans `spec_combat_boucle.md` :

- Priorité au groupe : C_9419, test HP monstre <3, sinon choix de cible et
  C_92FE. La branche ne teste pas directement AFCF entre ces deux appels.
- Priorité au monstre : choix de cible et C_92FE, puis C_9419 si A745 vrai.
  C_9419 utilise la liste déjà préparée et les drapeaux victoire/fuite.
- C_9419 teste HP <3 après les trois premiers emplacements, même si une
  attaque a été sautée. Après le quatrième, le test est dans le parent
  (997E ou 9A70). Ce détail doit être respecté dans un port intégré.
- Après les attaques : C_9567 remet A745 à faux et appelle C_7CC8 sur
  chaque membre du tour ; C_4D46 est ensuite appelé. La boucle continue si
  A745 vrai, A746 faux et AFCF faux, puis revient à 9757.

Ne pas affirmer que la disparition d'un personnage empêche aussitôt son
action : les états et drapeaux ont un calendrier original, à reproduire
avec l'orchestration entière. Le rôle de tous les sous-appels de C_4D46
reste à vérifier avant de traiter cet appel comme du simple rendu.
Le lot 4 porte aussi la relocation de fuite C_86E1 et les compteurs de
récompense séparément. Les autres effets de sortie restent à intégrer.

## Validation exécutée

```
python reconstruction/tools/compare_round_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

390 comparaisons sans écart : 103 cas de C_4C6C et 36 cas de C_7CC8 (HP négatifs/nuls/positifs,
HS/EM/autre état, drapeau A745 initial faux/vrai), et 251 préparations avec
groupes dispersés de 1..4 membres, seuils et débordements d'initiative,
MP sous/au-dessus de limite et tous les indices de cible. Les trois suites
CTest passent. Les 6 222 cas du lot précédent ont également été relancés
sans écart : **6 612 comparaisons exécutées au total**.

Le banc exécute la région 9757 jusqu'à l'entrée C_92FE, où il s'arrête.
Il observe la branche, les cadres, la cible et les MP. C_9419 est remplacé
par un retour sans attaque ; le monstre a 100 HP pour atteindre la riposte.
RANDOM reçoit les deux valeurs imposées, et ses bornes 5/effectif sont
vérifiées. Pour C_7CC8, seul le bloc de présentation 7D36→7D8D est sauté,
après l'écriture HP/HS et avant le test A745 ; les comparaisons et les
affectations de chaînes utilisent les routines Pascal originales.

Le rapport local est `build/audit/round_comparison.json`. Cela valide les
frontières décrites, pas un combat complet ni la fidélité de l'affichage.
