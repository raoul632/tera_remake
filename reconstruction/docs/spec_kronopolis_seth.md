# Kronopolis et temple de Seth — lots 17 et 18

## Sources et intégration

**CONFIRMÉ — BIN/PAS/DATA** : OVL00BC, OVL0069, OVL00F0 et branches
TERAMAIN, C8315, CAA13, C4CF9, C7EF6/C7DBD, C9B4E et CA0DD.
Les catalogues/indications viennent de MESSAGE et OBJETS locaux.
`tools/audit_commerce_seth_binary.py` capture les régions et leurs empreintes
sous `build/audit/`. Aucun original n'est modifié ou redistribué.

**E** à Kronopolis (lieu 0) ou au temple (lieu 4) demande O/Entrée.
Une autre touche annule. Les deux lieux sont reliés au monde généré et
au frontend d'exploration. Le rendu reste terminal ; les tirages de
présentation demeurent dans le C.

## Kronopolis : commerce et hôpital

**CONFIRMÉ — BIN** : menu A acheter, V vendre, H hôpital, ESC sortir.
Le premier ESC lu dans OVL00BC entre quand même dans REPEAT et demande
une autre touche ; un ESC suivant quitte. Cette particularité est conservée.
Les confirmations acceptent O/Entrée. A-D sélectionne le héros ou les
compagnons actifs, HS inclus, avec les reprises du sélecteur C4B55.

Catalogue A..S :

| Lettre | Objet/service | Prix de base |
|---|---|---:|
| A/B/C | objets 17/18/21 | 100/500/1000 |
| D | énergie +50 | 50 |
| E/F | cartes mondes 10/9 | 100/100 |
| G/H/I | objets 13/14/16 | 25/50/75 |
| J/K | potions 23/24 | 90/40 |
| L | clé 9 mise à 9 | 25 |
| M/N | objets 19/20 | 90/300 |
| O/P/Q/R | objets 1/4/7/10 | 200/250/300/350 |
| S | objet 22 | 750 |

Un jet C8315(charme, négociation) est consommé par choix A..S, même si
l'argent manque. Prix = `(base*30) DIV (30 + jet DIV 5)` avec les Integer
signés 16 bits, divisions vers zéro et comparaison originale
`prix < money+1` : l'addition déborde à 32767, sans correction moderne.
Un inventaire plein ou HS refuse l'objet et conserve l'argent. Les
services spéciaux n'utilisent pas de slot et restent achetables par un HS.

La vente choisit un slot 0..9, sans confirmation supplémentaire. Un slot
vide ne consomme pas de jet. Le prix est `base + (base DIV 100)*jet`,
l'objet disparaît et l'argent est ajouté avec débordement. La liste
affiche 1..24, mais l'objet 25 possède aussi son prix (10). Hors 1..25,
C004 conserve le prix précédent ; aucun nouveau tarif n'est inventé.

L'hôpital est gratuit. Il cherche le plus grand `MaxHP-HP` des membres
actifs, guérit leur statut en OK et ajoute `7*(maximum+1)` jours. Puis
C4CF9 recalcule et restaure HP/MP des **dix** personnages ; les statuts
des inactifs restent inchangés. Pas de jour quotidien, rencontre ou
recrutement ajouté pendant cette session.

## Temple de Seth

**CONFIRMÉ — BIN** : entrée à Position=800, WorldId=8, Previous=799 ;
BackupPosition conserve la position de surface. Le contexte d'overlay
vaut 2, distinct de 0/1 pour la cité : un WorldId=8 seul ne suffit pas
à identifier le temple. Le temple et le huitième niveau de cité utilisent
les mêmes données de carte comme dans l'original, sans duplication inventée.

Les touches 2/4/6/8 réutilisent le mouvement relatif et les portes/clés.
Atteindre 799 revient à la position de surface et WorldId=11. D/M n'y
changent pas d'étage. Les monstres fixes, fuites et le calendrier restent
actifs. Chaque rendu consomme RANDOM(12), même sans son et à 800 ;
sur 3, hors entrée et avec son, C68AD est exécuté. Les inscriptions de
l'autel et de l'entrée consomment leurs 48 paires RANDOM(2)/RANDOM(3).

O ouvre uniquement un contenu <90. P prend un objet déjà ouvert.
Le piège commence par RANDOM(2), contre RANDOM(3) en cité. L'explosion
inflige RANDOM(3+difficulté), la lame RANDOM(2+difficulté). La lame tire
**après** le son chez Seth, alors que la cité tire avant ; le gaz, cyanure,
désamorçage, soins de statut et argent gardent leurs règles originales.

L lit l'avertissement à 800 et le nom de l'autel au contenu 110.
I sur l'autel demande le quatrième mot secret (index 3), avec le lecteur
original de neuf caractères. Une annulation ne lance pas l'invocation
générique. Le bon mot invoque le monstre 21. La victoire active
`tasks_done[3]` et ajoute une tâche une seule fois, avec ses trois mélodies.
Un monstre fixe à AriochPosition dans le temple ne valide pas l'objectif
de la cité. Hors autel, l'invocation générique ARIOCH reste distincte.

**CONFIRMÉ — BIN** : le premier rendu précède REPEAT. Même si l'entrée
tue le groupe, l'original attend encore une commande avant son test
de sortie. Le pont conserve `temple_entry_pending` ; l'export/rechargement
préserve cette attente. Ensuite viennent le retour, la façade et les
condoléances/succession déjà portées. Aucun successeur anticipé n'est ajouté.

## Façades et sauvegarde

**CONFIRMÉ — BIN** : la façade de Kronopolis joue deux C6352 si le son
est actif. Celle de Seth tire neuf paires RANDOM(76)/RANDOM(10), à l'arrivée
et au retour. Ces effets sont exécutés après le pas/jour à la frontière
TERAMAIN appropriée ; le chargement des bitmaps et le matériel sont
**MODERNISABLES**, pas leurs tirages. Un simple Q/rechargement ne rejoue
pas la façade d'un état déjà traité.

Q exporte le monde moderne après une opération terminée. ESC quitte un
sous-menu de commerce ; Q y reste une lettre valide du catalogue. Une
interruption de transaction, confirmation ou combat ne publie rien.
Le JSON source reste inchangé. Les prix ne sont pas calculés par Python.

## Validation et limites

- Commerce : **512 cas, 3 385 frontières, 492 appels RNG, zéro écart**.
- Seth et entrée TERAMAIN : **274 cas, 780 frontières, 34 410 appels RNG,
  zéro écart**. Déplacements, portes, pièges avec/sans son, coffres,
  monstres fixes, mort, mots, victoire et objectif déjà accompli inclus.
- Façades : **16 cas, 1 262 appels RNG, zéro écart**.
- Cité relancée : **887 cas, zéro écart** ; coffres de cité : **1 076 cas,
  zéro écart** ; entrée/retour de cité : **51 cas, zéro écart**.
- **11 suites C et 41 tests frontend passent**, dont huit nouveaux parcours
  Kronopolis/Seth avec sauvegarde/rechargement et interruption sans export.

Les oracles exécutent les instructions et le RNG originaux. Seuls clavier,
BIOS, bitmap/vidéo et délais sont abstraits. BP_03 n'est comparé que dans
l'overlay où ce local existe, pas dans la pile du prompt d'entrée TERAMAIN.

**MANQUANT** : autres lieux/événements et voyages, mondes 9/10,
Alfol/Meduz, autres objectifs et fin victorieuse/score. La quatrième tâche
est désormais accessible, sans annoncer une victoire complète du jeu.
**À VÉRIFIER** : animations interrompues au clavier et parcours entier.
**MODERNISABLE** : rendu graphique et audio audible encore à connecter.
