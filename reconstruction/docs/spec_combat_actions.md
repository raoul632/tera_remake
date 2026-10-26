# Spécification indépendante : actions numériques de combat

Audit du 11 octobre 2026. **Ce document spécifie des sous-routines, pas le
combat complet.** Statuts selon `cadre_projet.md`. Python n'est pas utilisé
comme preuve originale. Port C : `engine/src/combat.c`.

Depuis la séparation demandée par l'utilisateur, ce document et le C sont
dans `reconstruction/`. Les mentions « Python corrigé » désignent le
patch `reconstruction/patches/python-combat-corrections.patch` et sa copie
de test isolée, **pas le `src/` historique**, restauré sans ces changements.
Les chemins des sources originales sont relatifs à la racine du dépôt ;
les chemins du moteur, des tests et des outils sont relatifs à reconstruction.

## Sources et reproduction

- BIN : `data/compiled program/TERA.COM`, SHA256
  `22fb836aa93f46dc2437e8b9586415d13552bfac35e8ac6647ba6157fd9b1a37`.
- BIN : `TERA.000`, SHA256
  `539d95781f9296d40ecd05698eb98255835a45cb03e60b49cf581e3ada7038f4`.
- PAS : `TERA.PAS::tSavedata` et `OVL_0010.PAS::INNER_BBA5`.
- DATA : `VERBES`, enregistrements de 19 octets, et `OBJETS`, 16 octets.
  La longueur Pascal précède le texte CP437. Le JSON extrait est un outil
  de consultation ; les fichiers bruts constituent la source.

`python reconstruction/tools/audit_combat_binary.py` capture uniquement les régions utiles
et les empreintes dans `reconstruction/build/audit/` (exclu de Git). Le désassemblage
linéaire montre aussi des chaînes inline comme des instructions : suivre
les sauts et les appels, ne pas interpréter ces octets comme du code.

Pour OVL_0010, extraire `TERA.000[0x1000:0x2200]`, origine **0xBA21**.
Cette origine aligne `INNER_BA2F` à BA2F, `INNER_C509` à C509 et les appels
RANDOM à 10DA. BA0F dans les anciens documents désigne le descripteur
résident, pas un segment ni l'origine du contenu de cet overlay.
Preuve d'alignement : `TERA.COM[BA0F-100h]` commence par le descripteur
contenant `tera.000`, et le Pascal nomme ces points d'entrée. L'alignement
est **CONFIRMÉ** pour cet overlay ; les autres ne sont pas audités ici.

## Interfaces originales — CONFIRMÉ BIN

Les arguments sont empilés de gauche à droite. Le dernier est à BP+4.
Un emplacement de résultat supplémentaire est réservé pour les fonctions.

| Routine | Arguments décodés | Preuve utile |
|---|---|---|
| C_8215 | personnage, compétence | BP+6 multiplié par 10 ; BP+4 ajouté ; lecture 7478h |
| C_8270 | objet, personnage | BP+6 = objet comparé ; BP+4 × 10 = ligne d'équipement 73B0h |
| C_8315 | personnage, stat, compétence | BP+8 = ligne ; BP+6 = stat ; BP+4 = compétence |
| C_8391 | personnage, objet maximal | BP+4 décrémenté ; appel C_8270 avec cet objet et BP+6 |
| C_842A | personnage, type d'action | BP+6 = ligne ; BP+4 sélectionne les branches |

Le pseudo-Pascal reconstruit de `TERAUTIL.INC` intervertit plusieurs de
ces noms et ne constitue pas une preuve indépendante.

## Équipement — CONFIRMÉ BIN

Adresses 839F–83D5 : mettre `candidat = objet_maximal`, rechercher ce
candidat **dans les dix emplacements du personnage**, décrémenter, puis
répéter jusqu'à objet trouvé ou trois essais. Le premier trouvé gagne.
83E4–83ED : bonus = `4 + candidat_après_décrément - objet_maximal`.

```
pour décalage = 0, 1, 2 :
    si équipement[personnage] contient objet_maximal - décalage :
        retourner 3 - décalage
retourner 0
```

| Objet maximal | Essai 1, bonus 3 | Essai 2, bonus 2 | Essai 3, bonus 1 |
|---|---|---|---|
| 3 | gemme de feu | diamant | cristal |
| 6 | orbe du temps | talisman de xur | talisman |
| 9 | anneau de mort | anneau d'argent | anneau |
| 12 | pierre-étoile | pierre de lune | pierre |
| 15 | épée runique | hache | épée |
| 18 | désintégrateur | revolver | arc |
| 21 | armure spatiale | cuirasse | cuir |

Aucun accès à l'équipement d'un autre personnage n'apparaît dans cette
routine. L'ancien « partage avec les deux compagnons précédents » est faux.

Effet secondaire **CONFIRMÉ BIN**, 83F0–841B : AFCC reçoit l'ID de l'objet
trouvé, ou 0 si absent, sauf quand l'objet maximal vaut 21 : AFCC reste
inchangé. Le lot 5 conserve désormais cet effet dans `interactive.c` et
compare AFCC aux frontières clavier ; C_8B5E puis la sortie non-F de
C_8CE3 le vident. Il sert à souligner le nom d'objet, sans retrait
d'équipement dans ces routines. Voir `spec_combat_interactif.md`.

## Jet de base — CONFIRMÉ BIN

C_8215 : `modificateur(niveau) = -2 si niveau = 0, sinon niveau`.
C_82E5 : `seuil(jet) = jet - 35 si jet > 44, sinon 0` (comparaison signée).

```
C_8315(personnage, stat, compétence) :
    puissance = Stats[personnage][stat]
              + modificateur(Skills[personnage][compétence])
              + Skills[personnage][7]
    jet = mot16(puissance × (RANDOM(15) + 3))
    retourner seuil(jet)
```

8332 lit Stats ; 8347 appelle C_8215 ; **8359 lit 747Fh**, soit Skills
base 7478h + colonne 7. PAS nomme cette compétence « système D ».
La clé Python `bonus_fixe` était sans fondement ; c'est le niveau brut
de « système D », même si cette compétence intervient déjà ailleurs.

## Les neuf branches — CONFIRMÉ BIN

843F–8448 : **toujours tirer** `r = RANDOM(15)+3` à l'entrée de C_842A.
Les types 1–5 et 9 appellent ensuite C_8315, qui consomme **un deuxième
tirage**. Les types 6–8 utilisent le premier. Ne pas supprimer le premier
tirage des actions simples parce que sa valeur est inutilisée.

Dans le tableau, B(s) désigne C_8315(personnage,s,s), E(i) le bonus
d'équipement du personnage, H[1..4] les aides de la partie.

| Type | Résultat |
|---|---|
| 1 | B(1) × (1 + E(15)) |
| 2 | (B(9) × E(18) × 3) DIV 2 |
| 3 | B(5) × (1 + E(H[4])) |
| 4 | B(3) × (1 + E(H[1])) |
| 5 | B(6) × (1 + E(H[3])) |
| 6 | seuil(((Stats[3]+Stats[6]+mod(Skills[6])+mod(Skills[3])) × r) DIV 2) × (1+E(H[2])) |
| 7 | (seuil(((Stats[4]+Stats[7]) DIV 2) × r) × (2+E(21))) DIV 2 |
| 8 | seuil((Stats[2]+Stats[5]+mod(Skills[5])) × r) |
| 9 | B(2) |

IMUL conserve le mot bas AX ; IDIV utilise CWD et tronque vers zéro.
Conserver les réductions en mot signé **avant** les divisions et seuils.
Les champs source sont des octets ; hors domaine d'entrée original,
aucune validité de mécanique n'est revendiquée.

Les types 7/8 existent, mais leurs déclencheurs hors menu restent
**À VÉRIFIER**. Ne pas leur inventer un verbe.

## Verbes — CONFIRMÉ BIN + DATA

Le menu C_8790 compare directement A674h à des lettres ASCII ; la
soustraction de 40h à 87E8 alimente une autre variable. Les anciennes
notes confondaient cette soustraction avec les comparaisons suivantes.

| Touche | DATA VERBES (1-based) | Type | Appel C_842A |
|---|---|---|---|
| T | 26 tirer | 2 | 881E |
| L | 27 lutter | 1 | 884C |
| J | 28 jeter un sort | 9 | 887A |
| D | 29 dissiper | 4 | 88A8 |
| A | 31 attaque mentale | 3 | 88D6 |
| P | 32 psalmodier | 5 | 8904 |
| C | 33 crier | 6 | 895A |

VERBES[30] « invoquer » n'est pas proposé par cette routine. F appelle
86E1 (fuite), pas C_842A. Les confirmations et leurs effets de tour ne
sont pas encore reconstruits. Crier lit un mot à 8932–894B : son effet
sur la résistance est confirmé dans `spec_combat_resolution.md` ; le Python le demande mais ne le
valide pas encore. Ce combat reste provisoire.

L'autel de soin appelle directement C_8315(personnage,6,6) dans
OVL_0049.PAS. Le Python utilise maintenant cette routine, pas une action
du menu : **un seul tirage**, aucun bonus d'équipement.

## Attribution des aides — CONFIRMÉ PAS + BIN

Dans OVL_0010::INNER_BBA5, confirmé à C246–C509 de l'overlay :

```
a = RANDOM(4)
b = RANDOM(4), répéter tant que b = a
c = RANDOM(4), répéter tant que c appartient à {a,b}
d = RANDOM(4), répéter tant que d appartient à {a,b,c}
objet = {0:6, 1:3, 2:9, 3:12}
H[1] = objet[b] ; H[2] = objet[d]
H[3] = objet[c] ; H[4] = objet[a]
```

Les indices sont conservés en `SaveGame.helper_items` (0-based en Python).
Le générateur est appelé après les mots secrets dans les deux frontends.
L'ordre d'initialisation de **toute la partie** n'est pas encore fidèle.
Le générateur Python n'est pas le RNG original : même suite de valeurs
fournies => mêmes appels locaux, sans prétendre à une même suite par seed.

Les sauvegardes anciennes sans H restent lisibles, mais leur combat affiche
MANQUANT. Ne pas leur attribuer une permutation inventée après coup.

## Validation et limites

Vecteur calculé depuis les instructions : toutes Stats=10, Skills=3,
E=3, premier facteur=7, second=5 : résultats des types 1..9 =
`180, 202, 180, 180, 180, 224, 87, 126, 45`.
Seuil strict : 44 => 0 ; 45 => 10. Débordement : type 2 avec
stat=255, compétence=255, système D=255 et facteur=17 donne -7171.

Tests Python : branches, tirages, seuils, hiérarchie sans partage, aides
(24 permutations et doublons rejetés), sauvegarde et appel direct d'autel.
Tests C : vecteurs de branches, seuils, équipement, débordement et API.
Ces tests valident la traduction de la spécification relue. Une comparaison
différentielle exécutée a depuis été ajoutée : `validation_binaire.md`
décrit ses entrées, résultats et limites.

Le C est une bibliothèque pure compilable C99. Les tirages sont fournis
explicitement. Le lot 2 ajoute résistance, riposte et demande d'empoisonnement
dans `spec_combat_resolution.md`. L'algorithme RNG, les tours complets,
les jours et les conditions de victoire du jeu restent à reconstruire.

Commandes Windows du poste :

```
python reconstruction/tools/test_python_reference.py
cmake -S reconstruction -B reconstruction/build/cmake -G "MinGW Makefiles"
cmake --build reconstruction/build/cmake
ctest --test-dir reconstruction/build/cmake --output-on-failure
```
