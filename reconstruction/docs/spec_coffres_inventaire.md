# Lots 13 et 14 — coffres/pièges et inventaire/équipement

**CONFIRMÉ — BIN/PAS/DATA**, dans les overlays 002E/0049 actuellement
jouables et les routines résidentes d'objets. Sources : TERA.COM,
TERA.000, OVL_002E.PAS, OVL_0049.PAS, OBJETS/VERBES/MESSAGE. Aucun
comportement n'est repris du Python historique comme preuve.

## Lot 13 : ouvrir, subir le piège, prendre

`engine/src/chest.c` conserve deux commandes séparées. O sur un contenu
<100 demande O/Entrée, puis un volontaire A-D sauf coffre vide. ESC
annule le volontariat ; une autre touche invalide redemande, comme C4B55.
Un volontaire HS ne tire rien et n'ouvre pas. Il n'y a pas de test
supplémentaire d'appartenance au groupe pour A, qui désigne toujours le héros.

Le piège tire RANDOM3. Si ce jet vaut zéro, le jet de désamorçage est
`(agilité + habileté + mod(compétence 7)) × (1 + RANDOM4) > 11 + difficulté`,
avec mod(0)=−2. Le test est strict. S'il échoue, RANDOM4 choisit :

| Jet | Effet conservé |
|---|---|
| 0 | Gaz : tous les actifs non HS passent EM |
| 1 | Explosion puis chacun des actifs non HS perd RANDOM(2+difficulté) HP |
| 2 | Cyanure : le volontaire passe EM |
| 3 | Lame : le volontaire perd RANDOM(2+difficulté) HP |

Les blessures appellent C7CC8 : HP<1 devient zéro/HS, tandis qu'un HS
préexistant n'est pas normalisé. Le piège ne réévalue pas artificiellement
PartyAvailable et ne saute pas la découverte du butin si le volontaire
vient de tomber HS. C4D46 avant/après le piège garde ses effets sur les
maxima des quatre premiers actifs, HS inclus.

Les tirages de présentation restent dans la graine : gaz/lame, si le son
est actif, consomment 146 RANDOM20. Explosion(TRUE)/C6D32 consomme huit
paires RANDOM4/RANDOM16 ; chaque image ajoute 16 RANDOM100 avec le son
actif. Ils précèdent les dégâts de l'explosion. La lame tire son dégât
avant son bruit. Le terminal ne modifie pas cette chronologie.

Un contenu 26..89 donne immédiatement contenu−20 pièces, avec débordement
signé 16 bits, puis la case devient vide. Les contenus 1..25 sont des
objets ; 90..98 sont des clés, 99 une carte. O les révèle et pose BP_03,
sans les retirer. P ne fonctionne qu'avec BP_03 vrai et un contenu non nul.
Après confirmation, une clé écrit Keys[contenu−90]=contenu−90 ; une carte
écrit HasMap[WorldId]. Les objets passent par CAA13 et restent dans la
case si le personnage choisi est HS ou plein. P n'admet pas ESC comme
annulation de C4B55 : le lecteur redemande A-D.

O/P inapplicables utilisent la phrase générique originale RANDOM3. Refuser
une confirmation de coffre consomme la commande sans inventer de journée,
rencontre ou repli. O sur un coffre déjà ouvert ne retire rien et ne tire
pas de nouveau piège. **BP_03 est local à la visite**, pas une propriété
permanente des cartes : chaque nouveau rendu le remet à faux, y compris
les transitions et retours de fuite. Un mur bloqué ne le réinitialise pas.

## Lot 14 : acquisition, transfert, abandon et potions

`engine/src/inventory.c` porte CAA13, CAB40, CAD08, CAE4F et C4B55.
L'équipement comporte dix slots numérotés 0..9, sans compactage, tri ou
nouveau mécanisme « porter/retirer ». Les bonus de combat déjà audités
scannent directement ces objets ; leur acquisition/remise les rend donc
immédiatement disponibles aux règles originales.

- **CAA13** : premier slot nul, dixième inclus. HS/plein refuse et conserve
  l'objet en attente. Aucun test d'activité supplémentaire : l'appelant
  sélectionne le personnage. Le succès met l'objet en attente à zéro.
- **N puis D** : dialogue donner, confirmation O/Entrée, source A-D, slot
  0..9 puis cible A-D. La source/cible doit être active. Copier par CAA13
  avant de retirer le slot source. Échec de réception conserve la source.
  Donner à soi-même reste permis : un inventaire plein refuse ; avec une
  place libre, la copie précède le retrait, sans duplication.
- **A puis O** : abandonner un objet, choisir le personnage et le slot.
  Vider seulement ce slot. Un slot vide ne retire aucun autre objet.
- **A puis C** : abandonner un compagnon B-D. Seul son flag actif change ;
  ses objets, HP, caractéristiques et état sont conservés. A ne chasse
  pas le héros. La disponibilité globale n'est pas réinventée.
- **B** : choisir un actif (ESC annule), puis un slot. Le refus HS se fait
  après la saisie du slot, comme le COM. L'objet 23 recalcule/restaure HP
  et MP sans guérir EM. L'objet 24 met OK sans soigner HP/MP. Ces potions
  sont consommées. Tout autre objet reste dans son slot.

C4B55 désigne A=héros, B/C/D=les trois premiers compagnons actifs dans
l'ordre du roster, HS inclus. Pas de correspondance fixe B=ID1. Le lecteur
C4418 exige un chiffre : ESC/lettres redemandent, sans abandon implicite.
Les phrases d'entrée A/B consomment RANDOM3. N/D, l'acquisition et les
effets des potions ne tirent rien. L'affichage de groupe recalcule les
maxima comme le binaire ; il ne soigne ni ne normalise les personnages.
Les fonctions documentent leurs intentions et particularités originales.

## Intégration et reprise

L'exploration accepte O/P dans la cité et A/B/N dans les zones portées.
Le frontend affiche les dix slots avec les vrais noms d'OBJETS. Les choix,
refus, effets et tirages appartiennent au C. `inventory_bridge.c` transporte
les mêmes personnages que création/recrutement ; les cartes, autres lieux
et objectifs du JSON sont préservés. Le pont navigation publie aussi
BP_03, le résultat du piège et HasMap, ainsi que les mutations de case.

Q reste un export technique de l'état moderne, qui conserve BP_03 entre
deux commandes pour reprendre exactement la session. **MODERNISABLE** :
ce snapshot n'est pas la sauvegarde DOS, dont BP_03 n'est pas un champ
durable et dont le rechargement complet redessine l'overlay. Une fermeture
au milieu d'une lecture ne publie aucun état final. L'entrée est lue sans
modification. Les tests couvrent aussi export/reprise entre ouvrir/prendre.

## Vérification et limites

`compare_inventory_binary.py` : **632 cas, 1 727 frontières, 232 appels
au RNG original, aucun écart**. Dix acteurs, OK/EM/HS, chacun des dix
slots libres/plein, toutes les boissons 0..25, reprises invalides,
transfert plein/HS/vers soi-même, groupes discontinus et profils variés.

`compare_chest_binary.py` : **1 076 cas, 2 460 frontières, 23 922 appels
au RNG original, aucun écart**. Deux overlays, confirmations/refus,
coffres ouverts/fermés/vides, argent/débordement, objets, clés/cartes,
quatre pièges, son actif/coupé, HS/inactifs, dix membres et profils variés.
Les instructions originales et RANDOM s'exécutent ; seuls clavier/sorties
matérielles/délais sont abstraits. Le banc ne calcule pas les règles Python.
Les snapshots comparent personnages, slots, HP/MP/maxima/états, argent,
clés, HasMap, BP_03, cartes et graine à chaque lecture et au retour.

Les captures et empreintes sont reproductibles avec
`tools/audit_inventory_chest_binary.py`, sous `build/audit/` exclu de Git.
Les originaux ne sont ni déplacés, ni modifiés, ni redistribués.

Une entrée manuelle incohérente « coffre ouvert » sur un marqueur ≥100
est rejetée sans mutation : le DOS écrirait hors de Keys. Cet état ne
peut pas être obtenu par O dans les règles originales ; il n'est pas
présenté comme un coffre valide. Ce garde-fou technique ne remplace pas
un piège ou une règle. Le contrat C est testé séparément.

**MANQUANT** : autres lieux/overlays, objets propres à leurs événements et objectifs,
mondes 9/10, Alfol/Meduz, voyages et fin du jeu. Le transfert N/D n'annonce
pas ces dialogues comme portés. **À VÉRIFIER** : animations interrompues
au clavier. **MODERNISABLE** : graphique/audio matériel, encore abstraits.
Ces lots n'affirment pas une validation de la partie entière.

Le module d'inventaire isolé limite N à D. Le frontend d'exploration route
désormais N vers le menu complet du lot 15, qui délègue D au transfert.
Voir `spec_dialogues_succession.md` pour les autres choix.
