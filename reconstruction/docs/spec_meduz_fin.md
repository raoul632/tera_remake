# Meduz et fin de partie — lots 21 et 22

## Provenance et périmètre

**CONFIRMÉ — BIN/PAS/DATA** : OVL00A4 (`TERA.000[A400:AE00]`),
OVL0145 (`[14500:14800]`), OVL0142 (`[14200:14500]`), OVL00CB
(`[CB00:D200]`), entrée TERAMAIN E4C2..E570 et sortie E790..E7C0.
Le runtime arithmétique et de formatage du COM est exécuté dans les tests.
`audit_meduz_ending_binary.py` capture les régions et leurs empreintes dans
`build/audit/`, exclu de Git. Aucun original n'est modifié ou distribué.

Les nouveaux modules `engine/src/meduz.c` et `ending.c` décident les règles.
Les ponts et contrôleurs terminal ne font que transporter les états,
présenter les prompts et fournir les touches. Le Python historique reste
une référence de consultation, sans servir d'oracle.

## Entrée et voyage depuis la pyramide

Sur Amarande, la pyramide est le lieu 3. **M**, puis **O/Entrée**, lit un
mot selon C42AC : lettres majuscules, retour arrière, au plus dix lettres
acceptées mais neuf conservées. Seul `secret_words[1]` permet le voyage.
Un mauvais mot ou un refus laisse la position, le temps et la graine
inchangés. Un EOF au milieu de la saisie interrompt sans export partiel.

La position de la pyramide est sauvegardée avant le départ. Avec son,
C68F4 consomme 25 groupes de dix RANDOM(10*i), i=1..25. OVL0145 dessine
sept tunnels : R3/R5/R7 pour les paramètres, deux R3 de direction, 24
paires R255/R180, puis un R4, éventuellement R3, et R16 par étape.
Le port conserve ces tirages même sans dessin visible.

OVL00A4 arrive en WorldId=10, position 1000, précédente 999. Son premier
rendu et son calendrier précèdent REPEAT : une arrivée mortelle attend
encore une commande. Le snapshot Q conserve cette attente.

## Exploration et combats de Meduz

**2468** suit la navigation relative C7EF6 du monde 10. Les fosses/murs
de valeur zéro bloquent ; les passages générés sont de valeur un. Même
une tentative bloquée appelle CA0DD. Un mouvement autorisé consomme les
16 paires R300/R70 du ciel ; aucun bruit de pas n'est ajouté à cet overlay.

Un catalyseur (contenu 2) ou un sanctuaire (contenu 3) force le combat 22.
Ailleurs, un R9 égal à 4 le déclenche. Cinq sprites consomment cinq R66,
C6073 établit le contexte d'information, puis FightSequence(22,0) appelle
le combat directement. Son onde C6221 est déterministe : aucun RANDOM
sonore supplémentaire. Ne pas ajouter la présentation de SpawnMonster.
Le jour vient après ce combat et peut lui-même contenir une rencontre.
Les reprises après fuite sont conservées avant le test de sortie.

**M** sur un catalyseur demande O/Entrée. La première réussite met
`mental_extension` à vrai et ajoute un à **stats[5] et stats[2] de chaque
membre actif**, HS inclus. Ce sont des octets : 255 devient zéro. Aucun
soin, réinitialisation des maxima, jour ou bonus supplémentaire. Répéter
la méditation ne réapplique pas le gain. Un refus n'appelle pas la réponse
générique M.

**I** dans un sanctuaire demande O/Entrée puis le mot. La concordance avec
`secret_words[2]`, après extension mentale, valide `tasks_done[0]` et
incrémente le compteur signé 16 bits une seule fois. Avec son, les trois
mélodies originales sont consommées. Mauvais mot, absence d'extension,
objectif déjà fait : pas de nouvelle récompense. Cette commande n'ajoute
pas de jour. Hors sanctuaire, I conserve le lecteur générique et
l'invocation ARIOCH, avec ses filtres de villes/OnAlfol, combat et fuite.
N/S et A/B restent reliés aux modules résidents.

Le passage en 999 est encore rendu et actualisé avant UNTIL. Sortie ou
mort : sept jours supplémentaires (Integer 16 bits), retour à la position
de la pyramide et WorldId=11. TERAMAIN joue ensuite OVL0145, puis la façade
OVL0142 : 26 paires R76/R8 et 12 paires R300/R48. Le retour n'est pas annulé
par la mort du groupe ; les condoléances/successeur viennent ensuite.

## Déclenchement de la fin

Le test porte sur **les quatre drapeaux**, sans se fier au compteur.
Accomplir le dernier objectif dans un overlay ne ferme pas ce lieu : il
faut revenir à TERAMAIN. Le terminal attend donc le retour à Amarande
(WorldId=11, hors Alfol). La mort termine également l'overlay courant
avant la branche finale. Le HALT spatial garde sa priorité distincte.

Si les quatre objectifs sont faits et le groupe disponible, OVL00CB
présente les félicitations et le score. Si le groupe est mort, il présente
les condoléances, sans score et sans créer de successeur. Les deux branches
consomment les quatre mélodies originales avec son, puis une pause à
acquitter. Les fréquences diffèrent, leur calendrier RANDOM est identique.
Un rechargement après fin acquittée ne rejoue pas la musique.

## Score Pascal et erreurs originales

Formule confirmée à BF83..BFCD :

`(expérience / Integer16(jours - 4000)) * (1 + difficulté)`
`* (1 + (difficulté + 3) DIV 4) * 120`

La division et chacune des trois multiplications passent par le runtime
**Real48** : exposant biaisé, 39 bits fractionnaires et bit implicite,
tronqués après chaque opération. Le port reproduit ces étapes et expose
les six octets du résultat. Le test exécute aussi le convertisseur texte
Pascal 1F45 appelé par 28EE et compare les chiffres du score, en ignorant
seulement le remplissage de largeur minimale cinq.

Les jours signés peuvent déborder ; aucun plancher moderne n'est ajouté
au dénominateur ou à un score négatif. Jours=4000 provoque l'erreur runtime
Pascal **2**, avant musique/pause. Le terminal conserve cette erreur dans
le résultat final ; il n'invente pas de score de remplacement.

## État, validations et limites

Le snapshot moderne conserve `mental_extension`, `meduz_backup_position`,
`meduz_entry_pending`, les objectifs, `ending_complete`, `ending_victory`,
`ending_error`, `score_real48`, `score` et `score_text`. La graine et tous
les personnages/cartes restent ceux du moteur C. Le snapshot JSON n'est
pas le conteneur DOS `scenario.*` écrit par OVL00D2.

**MANQUANT** : autres lieux/événements secondaires d'Amarande et import/
export du conteneur de sauvegarde DOS. **À VÉRIFIER** : parcours intégral
sur un monde généré, sans repositionnement de banc ni objectifs prévalidés,
et interruptions clavier des animations. **MODERNISABLE** : graphismes,
sons audibles et présentation des écrans, encore abstraits au terminal.
Les quatre mécanismes d'objectif et la fin sont maintenant portés ; cela
ne certifie pas encore un parcours complet du jeu.
