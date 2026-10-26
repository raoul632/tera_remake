# Résolution des attaques — lot 2

Statut : **CONFIRMÉ BIN**, règles ci-dessous uniquement. Source : même
TERA.COM et empreinte que `spec_combat_actions.md`. Toutes les adresses
sont des adresses de chargement COM (octet fichier + 0100), pas des offsets
de fichier. Captures reproductibles : `tools/audit_combat_binary.py`.
Pas de session originale complète. Une comparaison différentielle exécutée
des routines est désormais décrite dans `validation_binaire.md`.

## Portées et données

C_838A est l'entrée du combat, avec corps à 95D9. Les fonctions imbriquées
retrouvent son cadre via leur lien statique `[BP-2]`. Dans le parent :
`[BP-4]` = lettre du menu moins 64, `[BP-6]` = résultat C_842A,
`[BP-8]` = HP actuels du monstre, `[BP-1C]` = HP initiaux.
C_8CE3(personnage, type_monstre) demande l'action puis la résout.
**8D20 est une instruction de C_8CE3, pas une entrée de fonction.**

VALMON est une table de six mots signés par monstre, index Pascal 1..22.
Les accès utilisent `type*12 + A745`, ce qui place la première ligne à
**A751**, pas A745. Colonnes utilisées et confirmées :

| Colonne | Rôle | Preuve |
|---|---|---|
| 2 | Résistance A | 8D9D, 8E8C, 909F, 9272 |
| 3 | Résistance B | 8E1B, 8F0A, 8F7B, 8FE5, 9056, 9110, 917A, 920C |
| 4 | Catégorie de réaction | 8D45 et cascade jusqu'à 928D |

La colonne 4 n'est donc pas un objet de faiblesse. Les catégories présentes
dans DATA sont 0, 1, 3, 4, 5, 6. Aucune ressource originale ni table extraite
n'est ajoutée aux fichiers destinés aux commits.

## Attaque du personnage

Notations : R = résultat signé de C_842A, MP = magie disponible. Les lettres
du menu doivent être distinguées des types internes de C_842A.

| Catégorie | Action | Attaque réduite | Résistance | Condition / coût MP | Adresses |
|---|---|---|---|---|---|
| 0 | L, T | R DIV 6 | A | Aucun | 8D51–8DBA |
| 0 | J, A | R DIV 6 | B | MP > 2 ; coût 3 | 8DBA–8E51 |
| 1 | T | R DIV 5 | A | Aucun | 8E54–8EA9 |
| 1 | J, A | R DIV 7 | B | MP > 2 ; coût 3 | 8EAC–8F40 |
| 3 | D | R DIV 5 | B | Aucun | 8F43–8F98 |
| 3 | J | R DIV 7 | B | MP > 2 ; coût 3 | 8F9B–901B |
| 4 | P | R DIV 5 | B | Aucun | 901E–9073 |
| 4 | L | R DIV 9 | A | Aucun | 9076–90B8 |
| 5 | C | R DIV 3 | B | Mot égal à SecretWords[1] | 90BB–912D |
| 5 | A | R DIV 9 | B | MP > 5 ; coût 6 | 9130–91B0 |
| 6 | A | R DIV 8 | B | MP > 2 ; coût 3 | 91B3–9246 |
| 6 | T | R DIV 5 | A | Aucun | 9249–928D |

Une combinaison absente est inefficace. La comparaison du mot est à
90D4–90E2 ; la normalisation de la saisie n'est pas portée. L'API C reçoit
le résultat de cette comparaison, sans inventer de tolérance.

C_8B5E(attaque, résistance), 8B5E–8C44 :

```
si attaque > résistance (comparaison signée) :
    dégâts = signed16(attaque - résistance + 3) DIV 3
sinon : dégâts = 0
HP_monstre = signed16(HP_monstre - dégâts)
```

DIV tronque vers zéro. Chaque addition/soustraction conserve un mot 16 bits.
Les dégâts ne sont pas bornés par l'API : les débordements sont conservés.
La magie est retirée dès que la branche est admissible, même si elle rate.
Le drapeau A74C représente cette admissibilité, pas une touche réussie.

AFCC sert à nommer l'objet lors du message C_8B5E, puis est remis à zéro.
Aucun retrait d'inventaire n'a été trouvé dans cette résolution ; cela ne
prouve pas l'absence de consommation dans les autres mécanismes du jeu.
Le port numérique ne produit pas les messages et ne gère pas AFCC.

C_9419 vérifie **HP_monstre < 3** après chaque compagnon et interrompt les
attaques suivantes après victoire ou fuite. Il s'agit de la défaite de ce
monstre, distincte de la condition de victoire globale du jeu.

## Riposte et empoisonnement

C_92FE(personnage, type_monstre), 92FE–9418 :

```
attaque = signed16(HP_actuels_monstre + 4)
si type_monstre < 15 : défense = C_842A(personnage,7) DIV 11
sinon : défense = C_842A(personnage,8) DIV 9
dégâts = C_8C45(attaque, défense, type_monstre, personnage)
HP_personnage = signed16(HP_personnage - dégâts)
```

Les types 7 et 8 sont donc respectivement utilisés pour les défenses
physique et mentale. L'équipement du type 7 reste dans C_842A.
La force du monstre n'utilise aucun tirage séparé dans cette fonction.

C_8C45, 8C45–8CE2, utilise le même calcul de dégâts que C_8B5E. Si attaque
> défense, il construit l'ensemble {1,2,3,6,7}, teste l'appartenance du type,
**puis consomme RANDOM(11-difficulté)**. Si le tirage vaut zéro ET le type
appartient à cet ensemble, l'état du personnage devient « EM ».
Le tirage est consommé pour toute touche, y compris pour un autre type.
Les routines 14FC/150E/15E1 ont été relues : construction d'ensemble,
insertion de bit et appartenance. Leur effet sur la règle est confirmé.
Une attaque qui ne dépasse pas la défense ne consomme pas ce tirage.

Le C expose `poison_draw_required` et reçoit le résultat du test de zéro.
L'appelant devra fournir le tirage avec la difficulté originale ; aucune
plage de difficulté ou variante de RNG n'est inventée dans cette API.
L'API retourne la demande d'état EM et les dégâts, sans modifier une cible.

## Validation et limites

`test_resolution_c.c` contient les quinze combinaisons admissibles, les
autres paires catégorie/action, les seuils MP 2/3 et 5/6, le mot incorrect,
le coût sur échec, le seuil HP 3/2, les débordements signés, la frontière
des monstres 14/15 et les 22 IDs pour l'empoisonnement et ses tirages.
Les résultats attendus sont calculés depuis les instructions BIN, sans
utiliser l'ancien Python comme oracle. Les deux exécutables CTest passent.

Le lot 3 porte préparation, initiative, cible et mise à jour HS dans
`spec_combat_tours.md`, avec comparaison exécutée. Restent **À VÉRIFIER /
MANQUANT** avant un combat complet : ordonnancement intégré, fuite, fin de
tour, récompenses, effets des messages et algorithme RNG. Ces lots ne sont
pas transférés au Python historique ni à sa copie corrigée du premier lot.
