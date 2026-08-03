# Guide — désassembler `TERA.COM` et porter le Pascal en Python

Ce guide explique, avec des exemples réels tirés de la session de
reverse engineering de `TERAUTIL.INC` (voir `docs/TERA_notes_moteur.md`
section 17), la procédure complète : de l'octet brut dans `TERA.COM`
jusqu'à la fonction Python dans `combat.py`. L'objectif est que tu
puisses refaire la même démarche toi-même sur une autre fonction si
besoin, sans redécouvrir la méthode à chaque fois.

## 1. Les formats de fichiers dans `data/`

### 1.1 `TERA.COM` — l'exécutable DOS

Un fichier `.COM` est le format d'exécutable DOS le plus simple qui
existe : **une copie brute de mémoire, sans en-tête, sans table de
symboles, sans information de relocation**. Trois règles suffisent à le
comprendre :

- Le fichier est chargé tel quel à l'adresse `CS:0100h` (les 256 premiers
  octets du segment sont réservés par DOS pour le PSP, *Program Segment
  Prefix* — infos sur la ligne de commande, etc.).
- Un seul segment mémoire pour tout : `CS = DS = ES = SS`. Pas de
  "sections" comme dans un `.EXE` ou un ELF moderne — juste du code et
  des données mélangés dans un même bloc de 64 Ko maximum.
- L'exécution commence directement à l'offset `0100h`, sans passer par
  un point d'entrée déclaré ailleurs (contrairement à `.EXE`).

C'est pour ça que la commande de désassemblage utilisée partout dans ce
projet a cette forme précise :

```bash
objdump -D -b binary -m i8086 -Maddr16,data16 --adjust-vma=0x100 TERA.COM
```

- `-b binary` : dire à objdump "ce n'est pas un format connu (ELF,
  PE...), désassemble-le comme un blob brut".
- `-m i8086` : jeu d'instructions 8086 16 bits (pas de 386+, cohérent
  avec un jeu DOS de 1986).
- `--adjust-vma=0x100` : **l'étape cruciale**. Sans elle, objdump
  numéroterait les octets à partir de 0 ; avec elle, les adresses
  affichées correspondent aux vraies adresses mémoire au moment de
  l'exécution (`0100h` et au-delà). C'est ce qui permet aux adresses
  affichées par objdump de correspondre **exactement** aux noms
  `C_XXXX`/`D_XXXX` déjà présents dans les fichiers `.PAS` décompilés il
  y a 15 ans (la personne qui a fait ce travail avait nommé chaque
  identifiant d'après son adresse mémoire réelle, faute d'avoir les
  symboles d'origine).

Premier octet du fichier, pour donner une idée concrète :

```
100: e9 79 2c    jmp   0x2d7c
```

Le programme saute immédiatement vers `0x2d7c` : les 0x2c7c octets
juste après ne sont pas du code exécuté en premier, mais des données
(chaînes, tables) placées là par le compilateur/linker. C'est une chose
à garder en tête : **dans un `.COM`, rien ne distingue "code" de
"données" au niveau du format lui-même** — objdump désassemble tout
bêtement tout le fichier comme si c'était du code, et il faut donc
s'attendre à voir des instructions absurdes (`aaa`, `iret`, `outsw`...)
partout où on tombe en fait sur une chaîne de caractères ou une table
numérique. Le seul moyen de s'y retrouver est de suivre les vrais
chemins d'exécution (les `call`/`jmp`), pas de lire le fichier
linéairement du début à la fin.

### 1.2 `TERA.000` — le fichier d'overlay

Turbo Pascal 3 gérait le dépassement mémoire (le jeu complet ne tient
pas dans 640 Ko) grâce aux **overlays** : chaque module
(`OVERLAY PROCEDURE OVL_XXXX`) est compilé à part et stocké dans un
fichier séparé (`TERA.000` ici), puis chargé par-dessus la même zone
mémoire au moment où il est appelé, en écrasant l'overlay précédent.
C'est la raison pour laquelle `TERAUTIL.INC` et `KBD.INC` — inclus
directement dans `TERA.PAS` **avant** la liste des `OVL_XXXX` (voir
`TERA.PAS` lignes 266-267) — se retrouvent dans `TERA.COM` (code
résident, toujours en mémoire) et pas dans `TERA.000` : c'est justement
ce qui a permis de les désassembler avec la même méthode que le reste
du moteur de combat (section 13 des notes moteur). Le contenu détaillé
de `TERA.000` (désassemblage des `OVL_*`) n'a pas été nécessaire pour
cette session, précisément parce que la logique de combat vit dans le
code résident.

### 1.3 Les fichiers `.IMA` — sprites

`ASTRON.IMA`, `BANNIERE.IMA`, `CRAPIM.IMA`, `FACADE.IMA`, `KRONO.IMA`,
`MAISONJ.IMA`, `MAISONS.IMA` sont des blobs d'images CGA, pas du code.
D'après `OVL_00DB.PAS` (section 10 des notes moteur) : chaque fichier
contient une suite de sprites, chacun préfixé par sa largeur/hauteur,
avec les données stockées en **deux plans** (pair/impair) qui
correspondent directement à l'entrelacement mémoire du CGA
(`$B800`/`$BA00` pour les lignes paires/impaires). Le nom de chaque
fichier correspond probablement à l'écran/la scène où ses sprites sont
utilisés (`BANNIERE` = écran-titre, `CRAPIM` = atlas principal de sprites
de jeu, `KRONO`/`FACADE`/`MAISONS`/`MAISONJ` = décors de Kronopolis,
`ASTRON` = séquence spatiale) — **ceci est une déduction depuis les noms
de fichiers, pas confirmé par lecture de code cette session**, à
vérifier si un jour le rendu de ces écrans est implémenté.

## 2. Retrouver les fonctions dans le désassemblage brut

### 2.1 Reconnaître une fonction Pascal compilée

Turbo Pascal 3 compile systématiquement le même squelette pour une
fonction/procédure avec paramètres :

```
PROLOGUE (toujours ces 4 instructions, dans cet ordre) :
    push   bp          ; sauve le bp de l'appelant
    mov    bp, sp       ; bp = nouveau repère de pile pour CETTE fonction
    push   bp           ; (uniquement si FUNCTION avec retour scalaire :
                        ;  reserve un mot pour stocker la valeur de retour)
    jmp    <corps>      ; saut vers le vrai début du corps (souvent juste
                        ;  3 octets plus loin, parfois beaucoup plus loin
                        ;  si le corps est un gros CASE compile ailleurs)

EPILOGUE :
    mov    sp, bp
    pop    bp
    ret    N            ; N = nombre d'octets de PARAMETRES a nettoyer
```

Le point le plus important, et le plus contre-intuitif si on vient du
C : **c'est l'appelée (callee) qui nettoie la pile**, via `ret N` — pas
l'appelant. `N` est donc directement le nombre de paramètres × 2 octets
(plus un mot supplémentaire si la fonction est imbriquée dans une autre,
voir section 2.3).

Exemple réel, la fonction de seuil de combat (la plus simple du lot,
adresse `0x82E5`) :

```
82e5: 55             push   %bp
82e6: 8b ec          mov    %sp,%bp
82e8: 55             push   %bp
82e9: e9 00 00       jmp    0x82ec
82ec: 8b 46 04       mov    0x4(%bp),%ax     ; ax := parametre 1
82ef: 3d 2c 00       cmp    $0x2c,%ax        ; compare a 0x2c = 44
82f2: 7f 03          jg     0x82f7           ; si ax > 44, saute
82f4: e9 0c 00       jmp    0x8303           ; sinon (echec)
82f7: 8b 46 04       mov    0x4(%bp),%ax
82fa: 2d 23 00       sub    $0x23,%ax        ; ax -= 0x23 = 35
82fd: 89 46 06       mov    %ax,0x6(%bp)     ; stocke le resultat
8300: e9 06 00       jmp    0x8309
8303: b8 00 00       mov    $0x0,%ax         ; resultat := 0 (echec)
8306: 89 46 06       mov    %ax,0x6(%bp)
8309: 8b 46 06       mov    0x6(%bp),%ax     ; charge le resultat pour le retour
830c: 8b e5          mov    %bp,%sp
830e: 5d             pop    %bp
830f: c2 04 00       ret    $0x4             ; nettoie 4 octets = 2 mots
```

`ret $0x4` = 2 mots nettoyés : 1 paramètre réel (`jet`, à `[bp+4]`) + 1
mot pour l'emplacement de retour (`[bp+6]`, une convention Turbo Pascal
pour stocker la valeur de la fonction avant de la charger en `AX` juste
avant `ret` — ce n'est **pas** un vrai second paramètre). Ça se traduit
directement en :

```pascal
FUNCTION C_82E5(jet: INTEGER): INTEGER;
BEGIN
    IF jet > 44
        THEN C_82E5 := jet - 35
        ELSE C_82E5 := 0;
END;
```

### 2.2 Retrouver l'ordre et le nombre des paramètres

Turbo Pascal empile les arguments **de gauche à droite** au site
d'appel. Comme la pile grandit vers le bas, **le premier argument
poussé (le plus à gauche dans le code source) se retrouve au décalage
`bp+N` le plus GRAND**, et **le dernier poussé (le plus à droite) est au
décalage le plus PETIT** (`bp+4`, juste après l'adresse de retour et le
`bp` sauvegardé qui occupent `bp+0` et `bp+2`).

Exemple avec `C_8315` (calcul de la puissance d'attaque brute), qui a 3
paramètres réels. Au site d'appel dans `OVL_0049.PAS` :
`C_8315(D_9B66,6,6)` — 3 arguments poussés dans cet ordre. En relisant
le corps de la fonction, `[bp+8]` sert d'index de ligne dans la table
des stats (donc c'est le **premier** argument, `D_9B66` = le
personnage), `[bp+6]` sert de colonne dans cette même table (donc le
**deuxième** argument), et `[bp+4]` est passé tel quel à un sous-appel
`C_8215(perso, [bp+4])` (donc le **troisième** argument). Une fois
qu'on a ce mapping, le prototype Pascal s'écrit sans ambiguïté :

```pascal
FUNCTION C_8315(competence, stat, perso: INTEGER): INTEGER;
```

(l'ordre des noms de paramètres dans la déclaration suit l'ordre des
décalages croissants `bp+4, bp+6, bp+8`, pas l'ordre d'appel — un piège
facile si on va trop vite.)

### 2.3 Le cas des fonctions imbriquées (nested) — le "mot en trop"

Certaines fonctions ont un `ret N` qui semble compter un paramètre de
trop par rapport aux arguments visibles aux sites d'appel. Exemple :
`C_8315` a 3 arguments visibles (6 octets) mais fait `ret $0x8` (4
mots). L'explication : Turbo Pascal ajoute automatiquement un paramètre
caché — le **static link** — quand une fonction est déclarée à
l'intérieur d'une autre (`FUNCTION Foo; FUNCTION Bar; ... END; END;`),
pour lui permettre d'accéder aux variables de la fonction englobante.
On voit cette transmission explicitement dans le prologue de `C_842A` :

```
842a: 55             push   %bp
842b: 8b c4          mov    %sp,%ax
842d: ff 76 fe       push   -0x2(%bp)   ; repousse le static link du PARENT
8430: 8b e8          mov    %ax,%bp
8432: 55             push   %bp
```

C'est aussi ce détail, laissé sans solution, qui a bloqué la
reconstruction de la fonction de résistance (`0x8D20`, voir section 17
des notes moteur) : son code adresse ses variables via `BX := [BP-2]`
sans jamais l'assigner dans la fonction elle-même — preuve que c'est
une fonction imbriquée dont je n'ai pas retrouvé la fonction englobante.
**Signal utile à retenir** : si tu vois des `[BP-2]` lus mais jamais
écrits, ne cherche pas plus loin dans cette fonction — remonte au
niveau au-dessus.

### 2.4 Retrouver les tables (tableaux Pascal indexés)

Un accès `Sav.f_Stats[perso][stat]` (tableau 2D) compile en un calcul
d'index classique "ligne × largeur + colonne" :

```
mov al, [bp+8]      ; al := perso
mul cx (=0xa)        ; ax := perso * 10   (10 = largeur d'une ligne)
xchg ax, di          ; di := perso * 10
mov al, [bp+6]       ; al := stat
add di, ax           ; di := perso*10 + stat
mov al, [0x7414+di]  ; lit l'octet a l'adresse de base + di
```

Deux informations à en tirer : **l'adresse de base** (`0x7414` ici) et
**la largeur de ligne** (`10`, donc probablement 10 colonnes par
personnage — cohérent avec `f_Stats` qui a 9 statistiques + 1 emplacement).
Une fois la base et la largeur identifiées, on peut confirmer avec
d'autres tables qui partagent la même largeur (`f_Equipment` à `0x73B0`,
largeur 10 aussi) ou une largeur différente pour une structure connue
différemment (`VALMON` à `0xA745`, largeur `12` = 6 colonnes de 2 octets
— cohérent avec "22 monstres × 6 valeurs" en `INTEGER`, pas en `BYTE`,
d'après la section 12.2 des notes).

### 2.5 Retrouver `RANDOM()` et les autres primitives du runtime

`RANDOM(n)` de Turbo Pascal compile en un simple appel avec `n` poussé
comme unique argument :

```
b8 0f 00      mov    $0xf,%ax     ; ax := 15
e8 71 8d      call   0x10da        ; RANDOM(15), résultat dans ax
05 03 00      add    $0x3,%ax      ; +3
```

Je l'ai identifié en remarquant que **tous** les sites d'appel à
`0x10DA` suivent ce même moule (constante poussée juste avant, résultat
utilisé immédiatement comme borne aléatoire), et que la formule qui en
ressort (`RANDOM(15)+3`) correspond exactement au "facteur aléatoire x3
à x17" déjà déduit dans les notes moteur avant même ce round de
désassemblage — une bonne validation croisée.

### 2.6 Corréler avec les fichiers `.PAS` déjà décompilés

Pour les fonctions **déjà appelées par leur nom** dans les `.PAS`
fournis (ex. `C_8315(D_9B66,4,4)` dans `TERA.PAS`), la corrélation est
triviale : le nom `C_8315` contient littéralement l'adresse hexadécimale
à chercher dans le désassemblage. J'ai fait ça avec `grep` sur tous les
`.PAS` pour lister tous les `C_XXXX`/`D_XXXX` référencés mais jamais
définis (donc forcément dans `TERAUTIL.INC`/`KBD.INC`, absents du
corpus), puis vérifié un par un que l'adresse correspondante dans
`TERA.COM` commence bien par un prologue de fonction (`push bp`).

Pour les fonctions **jamais nommées nulle part** (comme `C_842A` ou
`C_8790`, découvertes cette session), il n'y a pas de raccourci : il
faut remonter le graphe d'appels depuis une fonction connue
(`grep "call   0xXXXX"` dans le désassemblage complet) pour voir qui
appelle quoi, en repérant le même moule de prologue à chaque nouvelle
adresse suspecte.

## 3. Adapter le Pascal en Python — les 6 transformations récurrentes

Une fois le pseudocode Pascal reconstruit (comme `TERAUTIL.INC`), le
portage vers `combat.py`/`worldgen.py` suit toujours les mêmes règles de
traduction :

### 3.1 Fin des tableaux 1-based et des `VAR` par référence

Pascal indexe ses tableaux à partir de 1 dans plusieurs endroits
(`Sav.f_Stats[perso][1..9]`), et les compétences/faiblesses passent
souvent des index bruts. En Python, on utilise des **dictionnaires
nommés** (`char.stats["force"]`) plutôt que des listes 1-based, ce qui
élimine toute une classe d'erreurs de décalage d'index — mais impose de
documenter la correspondance index Pascal → nom (fait dans
`models.py` : `STATS = ["force","intelligence",...]`, `ACTION_TO_STAT`).

### 3.2 L'état global mutable devient des paramètres explicites

Tout l'état du jeu original vit dans **une seule variable globale**
(`Sav: tSavedata`), modifiée par effet de bord par toutes les fonctions.
En Python, chaque fonction reçoit explicitement ce dont elle a besoin
(`attack_power(char, action, party, ...)`) plutôt que de lire/écrire un
global caché — plus facile à tester unitairement, mais ça veut dire que
le portage doit d'abord identifier PRÉCISÉMENT quel sous-ensemble de
`Sav` chaque fonction touche (c'est tout le travail de la section 2).

Exemple concret, `C_8391` (bonus d'équipement) :

```pascal
FUNCTION C_8391(item, perso: INTEGER): INTEGER;
VAR i, bonus: INTEGER;
BEGIN
    bonus := 0;
    FOR i := 0 TO 2 DO
        IF (perso - i >= 0) AND C_8270(perso - i, item) THEN
        BEGIN
            bonus := 3 - i;
            D_AFCC := perso - i + 1;   { effet de bord sur un global }
        END;
    C_8391 := bonus;
END;
```

devient, sans le global `D_AFCC` (jamais réutilisé ailleurs dans le
portage Python, donc simplement supprimé plutôt que reproduit) :

```python
def equipment_bonus(char: Character, party: list[Character], required_item: str | None) -> int:
    if required_item is None:
        return 0
    idx = party.index(char) if char in party else 0
    for offset, bonus in ((0, 3), (1, 2), (2, 1)):
        i = idx - offset
        if 0 <= i < len(party) and required_item in party[i].equipment:
            return bonus
    return 0
```

### 3.3 `RANDOM(n)` Pascal (0 à n-1) → `random.randint`/`randrange` Python

`RANDOM(15)` de Pascal tire un entier dans `[0, 15)`. L'équivalent
Python le plus direct est `rng.randrange(15)`, ou de façon équivalente
`rng.randint(0, 14)`. Piège classique : `random.randint(a, b)` en
Python est **inclusif des deux bornes**, alors que Pascal `RANDOM(n)`
exclut `n` — il faut donc bien utiliser `randrange(n)` ou
`randint(0, n-1)`, jamais `randint(0, n)`. Vu dans `combat.py` :
`RANDOM_MULT_RANGE = (3, 17)` avec `rng.randint(*RANDOM_MULT_RANGE)`
reproduit `RANDOM(15)+3` correctement car `randint(3,17)` est bien
équivalent à `randrange(15)+3` (17 = 15+3-1, borne haute incluse).

### 3.4 `DIV`/`MOD` Pascal (troncature vers zéro) vs `//` Python (troncature vers -∞)

Pascal `DIV` tronque **vers zéro** ; l'opérateur `//` de Python tronque
**vers moins l'infini**. Les deux ne sont identiques que pour des
opérandes positifs — ce qui est le cas de tous les calculs de combat
observés ici (PV, résistances, dégâts sont toujours ≥ 0), donc `//` est
sans risque dans ce portage précis. **Point de vigilance pour la
suite** : si un futur calcul manipule des valeurs pouvant devenir
négatives (par exemple un modificateur de moral, une jauge qui descend
sous zéro), il faudra remplacer `//` par une troncature explicite
(`int(a / b)` ou `math.trunc(a / b)`) pour rester fidèle à Pascal.

### 3.5 `CASE OF` avec branches codées en dur → `if`/`elif` ou dispatch par dict

Le désassemblage de `C_842A` est un immense `CASE action_type OF 1..9`.
En Pascal reconstruit ça reste un `CASE` (fidèle à la structure
d'origine, utile pour la relecture) ; en Python, `worldgen.py` et
`combat.py` utilisent plutôt des dictionnaires de correspondance quand
la logique par branche est uniforme (`ACTION_TO_STAT` dans
`models.py`), et un vrai `if`/`elif` quand chaque branche a une formule
vraiment différente (comme ce serait le cas si on portait les 9
branches de `C_842A` telles quelles — voir la discussion en cours sur
`combat.py`, pas encore appliquée).

### 3.6 Les drapeaux "scratch" globaux disparaissent

Le code original stocke beaucoup d'état transitoire dans des octets
globaux à adresse fixe (`0xA743`, `0xA744`, `0xAFCC`...) simplement
parce que Turbo Pascal 3 ne permet pas de valeurs de retour multiples
facilement et que passer par un global est plus court à compiler qu'une
structure de retour. Ces drapeaux n'ont **aucune valeur pour le
portage** : ils deviennent des variables locales ou des valeurs de
retour directes en Python (`return bonus` au lieu de `D_AFCC := ...;
puis lire D_AFCC ailleurs`).

## 4. Exemple complet, de bout en bout

Pour resituer les 3 étapes sur un seul exemple (le calcul de PV à
l'invocation d'un monstre, `0x9727`, section 17 des notes moteur) :

**1. Désassemblage brut :**
```
9727: b8 e8 03    mov    $0x3e8,%ax
972a: e8 e6 6a    call   0x213                 ; SOUND/DELAY, hors-sujet ici
972d: 8b 46 06    mov    0x6(%bp),%ax          ; ax := monster_type
9730: b9 0c 00    mov    $0xc,%cx              ; largeur de ligne = 12
9733: f7 e1       mul    %cx
9735: 97          xchg   %ax,%di
9736: 8b 85 45 a7 mov    -0x58bb(%di),%ax      ; lit [0xA745 + di]  (colonne 0 = HP)
973a: 50          push   %ax
973b: 8b 46 06    mov    0x6(%bp),%ax
973e: b9 0c 00    mov    $0xc,%cx
9741: f7 e1       mul    %cx
9743: 97          xchg   %ax,%di
9744: 8b 85 4f a7 mov    -0x58b1(%di),%ax      ; lit [0xA745+10 + di]  (colonne 5 = variance)
9748: e8 8f 79    call   0x10da                 ; RANDOM(variance)
974b: 59          pop    %cx                    ; recupere HP
974c: 03 c1       add    %cx,%ax                ; HP + RANDOM(variance)
```

**2. Pseudocode Pascal (`TERAUTIL.INC`) :**
```pascal
FUNCTION C_95D9_SpawnHP(monster_type: INTEGER): INTEGER;
BEGIN
    C_95D9_SpawnHP := VALMON[monster_type][1] + RANDOM(VALMON[monster_type][6]);
END;
```

**3. Port Python (`combat.py`, déjà en place, aucune correction
nécessaire — cet exemple confirme une fonction déjà bien portée) :**
```python
def spawn_monster_hp(monster: Monster, rng: random.Random | None = None) -> int:
    rng = rng or random
    return monster.hp_base + rng.randint(0, max(0, monster.hp_variance - 1))
```

Les deux colonnes VALMON (`hp_base`, `hp_variance`) sont devenues des
attributs nommés du dataclass `Monster` (`models.py`) au lieu d'indices
de tableau — cf. transformation 3.1 — et le `RANDOM(n)` Pascal est
devenu `rng.randint(0, n-1)` — cf. transformation 3.3.
