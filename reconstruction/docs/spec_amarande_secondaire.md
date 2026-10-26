# Lieux et événements secondaires d'Amarande

## Sources et périmètre

**CONFIRMÉ — BIN/PAS/DATA** : TERAMAIN DC70..E1B1 et E5C7..E6E1,
CAFE8 AFE8..BA0F, halte A709..AA13, C4473, C81D7, carte C5495,
overlays EC/F0/F3/142/148. Le manifeste SHA256 et les désassemblages
ciblés sont produits par `tools/audit_amarande_binary.py` sous
`build/audit/`, exclu de Git. Les ressources MESSAGE/2/3 sont lues dans
leur emplacement historique ; aucun original n'est ajouté aux sources.

Les décisions, combats, jours et tirages sont en C dans `amarande.c` et
`commerce.c`. Les ponts suspendent aux lectures originales ; le Python
présente les textes et transporte les états. Les nouveaux fichiers restent
sous `reconstruction/` et ne modifient pas le prototype historique.

## Les cinq lieux secondaires

| ID | Lieu | Personnage contrôlé par la façade |
|---:|---|---:|
| 5 | Sanctuaire de Mahalore | 3 |
| 6 | Domaine de Balgence | 8 |
| 7 | Ermitage de la Roche aux Goules | 5 |
| 8 | Hameau de la Porte des sables | 8 |
| 9 | Abbaye de Mortefontaine | 3 |

Le programme affiche le lieu après un déplacement puis conserve la boucle
des commandes résidentes : **N** négocier, **N/I** informations,
**N/S** solliciter, **S** séduire et **C** combattre. Les indices et les
conditions de recrutement utilisent les données de la partie générée.
TERAMAIN réserve l'entrée **E** aux lieux 0/1/2/4 ; **M** à la pyramide
ouvre le voyage de Meduz. Dans les lieux 5..9, E reçoit une réponse
générique de C4473 et reste sur Amarande.

La condition de présentation du personnage est exactement :
`Loc[id] != Town[lieu] OR (!Active[id] AND condition[id] != HS)`.
Son résultat positif met A750 à vrai. Le OR surprenant est conservé,
y compris quand le personnage se trouve ailleurs. Une façade sans PNJ
visible ne remet pas un ancien A750 à zéro. Le spatioport vérifie la
disponibilité du personnage 4 par A074.

Les façades conservent les tirages de leurs dessins et musiques :

| Lieu | Tirages graphiques, son désactivé | Son consommant aussi RANDOM |
|---|---:|---|
| Kronopolis | 0 | deux mélodies country |
| Spatioport | 89 | aucun tirage sonore supplémentaire |
| Cité des crânes | 23 | aucun |
| Pyramide | 76 | aucun tirage sonore supplémentaire |
| Temple de Seth | 18 | aucun |
| Mahalore | 18 | aucun |
| Balgence | 0 | trois mélodies |
| Roche aux Goules | 0 | aucun |
| Porte des sables | 0 | bruit de durée 100, seuil 6 |
| Mortefontaine | 0 | deux mélodies |

Les bornes et l'ordre des tirages sont portés depuis le désassemblage ;
l'exécution différentielle compare les états et les graines finales.
Les retours de la Cité et d'Alfol rejouent leur
façade, respectivement 23 et 89 tirages, sans ajouter de journée.

## Combats résidents et invocation

**C** demande O/Entrée. Le programme parcourt les personnages 1..9,
dans cet ordre. Les types adverses sont `10,17,17,12,17,17,12,8,14`.
Une victoire contre un personnage disponible met sa condition à HS ;
elle n'ajoute pas de modification artificielle de ses HP ou d'Active.

Après chaque personnage, la rencontre supplémentaire exige LocExtra égal
à la position et A750 vrai. Son type est 17 pour 3/5/8, sinon 14 si ABD7
vaut zéro, 10 dans les autres cas. ABD7 est conservé depuis le tirage
d'arrivée. La rencontre supplémentaire remet A750 à zéro. Le parcours
continue après une fuite ou la mort du groupe, comme CAFE8. Le terminal
reprend ensuite le traitement d'arrivée commun aux fuites.

**I** lit le mot par les règles C42AC : retour arrière, neuf lettres
stockées et dixième frappe qui termine la lecture. ARIOCH appelle le
monstre 21 à l'extérieur des dix lieux et hors Alfol. Il active A750 et
la suppression de rencontre ; son dessin conserve RANDOM et laisse
D9B66 à 120. Une invocation générique ne déclenche pas de jour forcé.

## Panneaux, halte et autres commandes

**L** sur un lieu 10..19 demande confirmation et calcule
`delta = Integer16(Town[0] - position)` ; l'axe est-ouest vaut
`delta DIV 30`, le nord-sud `delta MOD 30`. La division tronque vers zéro,
sans chercher le plus court chemin autour du monde. Une composante nulle
affiche O ou S. La lecture conserve la pause C4210 et laisse D9B66 égal
au reste signé. Elle ne consomme ni jour ni RANDOM.

**H** produit sa réponse originale puis lit une seule sous-commande :

- **A** : confirmation pour les déplacements rapides ; un refus les désactive.
- **C** : carte locale 10 colonnes × 30 lignes, avec bouclage. Pour le monde
  11, l'accès lit l'octet adjacent 9417 (`initial_flag`), pas un achat de carte.
- **D/M** : confirmation, jour forcé, vrais combats et réponse finale.
- **H** : aide et ses 27 tirages graphiques, D9B66 final à 8.
- **I** : introspection, recalcul des limites des quatre premiers actifs.
- **S** : jet mental du héros ; les compagnons actifs sont montrés avec
  une pause C4210 chacun, D9B66 final à 9. Les branches HS/échec gardent
  leurs tirages respectifs.
- **Échap** : confirmation de sortie. L'interface exporte son état JSON.

**#** confirme le changement du drapeau sonore. **D/E/F/L/M/O/P** hors
traitement spécialisé affichent les vraies réponses MESSAGE/2/3, choisies
par le C. **U** conserve confirmation, réponse, pause C4210 puis lecture
C4277 distincte. Une interruption à une lecture obligatoire n'exporte
pas un état partiel.

Le jour CA0DD préserve D9B66. La vérification croisée corrige donc Alfol :
le dessin de l'atterrissage le laisse à **16**, puis le jour conserve cette
valeur. L'alias radioactif documenté dans `spec_spatioport_alfol.md` reste
reproduit explicitement, sans accès C hors tableau.

## Validation et limites

**CONFIRMÉ — BIN** : 320 scénarios de façades, 38 784 appels RANDOM
originaux ; 311 scénarios de commandes/panneaux, 775 états et 5 715 appels
RANDOM. Aucun écart sur le périmètre comparé. La comparaison des commandes
inclut les ID et variantes des réponses C4473, les victoires de PNJ,
les refus, pauses/EOF, jours, compagnons et directions signées.

Régression : exploration 854 cas/900 états/934 751 tirages, Alfol
613 cas/764 états/176 326 tirages, zéro écart. Les tests du terminal
couvrent les arrivées et indices secondaires, les cinq réponses E,
la carte, la halte, les combats de PNJ, la sortie et la reprise JSON,
ainsi que les façades de retour Cité/Alfol.

```powershell
python reconstruction/tools/audit_amarande_binary.py
python reconstruction/tools/compare_amarande_facades_binary.py
python reconstruction/tools/compare_amarande_binary.py
ctest --test-dir reconstruction/build/cmake --output-on-failure
python -m unittest discover -s reconstruction/tests -p '*frontend.py'
```

**MODERNISABLE** : présentation terminal des façades, fiches et carte ;
sons audibles, animations et rendu graphique restent à réaliser. L'oracle
abstrait les primitives vidéo/BIOS, le chargement des bitmaps et le dessin
de la carte/des fiches ; il conserve les calculs de limites et RANDOM.
**À VÉRIFIER** : parcours intégral d'un monde généré et clavier pendant les
animations. **MANQUANT** : écriture du conteneur de sauvegarde DOS.
Le jeu entier n'est pas déclaré validé de bout en bout.
