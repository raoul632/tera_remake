# Déplacements Amarande et passage du temps — lots 9 et 10

## Portée et sources

**CONFIRMÉ — BIN/PAS** : les règles de déplacement d'OVL00FB, la détection
des lieux et le calendrier RNG de ses décors sont portés dans
`engine/src/exploration.c`. C_A0DD (A0DD–A708) ajoute rencontres, jours,
récupération, poison, niveau, caractéristiques et compétences.
Le moteur ouvre les vrais combats C et reprend le jour après leur retour.
Le Pascal de déplacement est croisé avec le binaire ; le TERAUTIL.INC
reconstruit n'est pas utilisé comme preuve du calendrier quotidien.

## Déplacement et décor

Amarande utilise les positions 1000 à 1899. 8/nord ajoute 1, 2/sud retire
1, 6/est ajoute 30 et 4/ouest retire 30, puis un dépassement ajoute/retire
900. Les neuf montagnes horizontales et vingt-cinq verticales bloquent
le passage selon la position **avant** le déplacement ; une montagne
visible devant la case d'arrivée ne bloque pas l'arrivée elle-même.
Un pas bloqué ne consomme ni décor ni jour. AFD1 est tout de même effacé.

Un pas réussi positionne A749, efface AFCE et, avec SoundFlag, consomme
RANDOM(24). La recherche des vingt lieux retient le dernier correspondant.
Sans lieu, AFD3 conserve sa valeur antérieure mais AFD4 est effacé.
Les six placements complémentaires autorisés peuvent chacun consommer
RANDOM(3), sans dédoublonnage moderne.

**CONFIRMÉ — BIN** : le terminal conserve les tirages graphiques :

- horizon : seize paires RANDOM(300)/RANDOM(70), puis RANDOM(2)/RANDOM(20) ;
- montagne proche : sept séquences RANDOM(3) puis quatre RANDOM(4) ;
- sol : treize paires RANDOM(76)/RANDOM(20), répétées trois, quatre ou
  cinq fois selon Position MOD 23 ;
- façades des lieux 10 à 19 : quarante-huit paires RANDOM(2)/RANDOM(3).

L'horizon précède montagne et sol lorsque la montagne est proche ; sinon
il suit le sol. Les lieux 0 à 9 n'effectuent pas ce rendu de paysage.
Changer cet ordre modifie les rencontres et les caractéristiques futures.

## Calendrier quotidien

Le premier RANDOM(20−difficulté) est consommé **même** quand les autres
conditions de rencontre sont fausses. Sur une case 1..899, hors 800,
contenu 255, monde différent de 11, sans compagnon disponible parmi IDs
1..6 et sans AFD2, un résultat zéro tire un type 1..17. Les types **12 à 14,
ainsi que 16**, sont exclus après tirage, sans remplacement. Correction issue
de la comparaison globale : l'ensemble BIN est [12..14,16], et non [12,14,16].

AFD2 est ensuite effacé. RANDOM(7) est toujours consommé ; le jour avance
si monde 11, résultat 1 ou AFD0. Le compteur ajoute 1 avec débordement
Integer 16 bits. AFD0 est alors effacé. Chaque actif non-HS recalcule ses
maxima, effectue **deux** jets vitalité C_8315(6,6), y compris sans
blessure ou poison. Objet **22 exactement** ou premier jet positif peut
ajouter 1 HP sous le maximum. Les MP récupèrent 1 sous leur maximum.
EM et second jet <1 enlèvent 1 HP, puis C_7CC8 normalise l'état.

La rencontre errante consomme RANDOM(max(20, Integer16(255−XP))), même
dans les lieux protégés. Un résultat 3, monde 11, hors villes, placements
complémentaires, position 0 et base Alfol, tire RANDOM(4) : résultat 1
choisit un type 12..15, sinon 1..7. La disponibilité est renormalisée
pour **tous** les actifs à la fin du calendrier.

## Combats et progression

La présentation C_9B4E conserve ses tirages avant C_838A, dont les boucles
des types 3/6/9 et les positions aléatoires des types 1/2. La session se
suspend au menu du combat ; aucune récupération ni progression future
n'est calculée avant sa fin. Les HP, XP, argent, état, fuite, position,
AFCE et graine du combat sont réintégrés avant la suite du jour.

Le quotient XP / Integer16(30 + 10×niveau + 5×difficulté) doit être
strictement supérieur au niveau pour ajouter **un** niveau. Chaque actif
non-HS tire une caractéristique 1..9, augmentée seulement si <11.
Le héros augmente aussi son niveau auxiliaire. Avec un compagnon apte,
chaque apte tire une compétence 1..9 jusqu'à ce qu'elle soit connue par
au moins un actif, **HS compris**. Elle augmente seulement si <5.
Le port n'ajoute ni second niveau dans le même appel ni compétence inventée.

## Frontend et limites

`frontend/exploration.py` utilise l'état généré avec héros terminé.
L'arrivée de TERAMAIN après création ou fuite effectue position−1 puis
un pas nord **avant** la prochaine lecture. Une nouvelle fuite pendant
ce pas provoque une nouvelle arrivée ; elle n'est pas absorbée silencieusement.

8/2/6/4 déplacent ; S exécute séduire avec AFCE existant. Il n'ajoute pas
une sollicitation automatique : le dialogue solliciter reste accessible
par le banc `recruitment.py`. Q exporte/quitte l'hôte terminal, sans
prétendre reproduire la commande de sauvegarde/fin DOS. Le fichier source
reste inchangé. Une interruption ne publie aucun nouvel état final.

**MODERNISABLE** : présentation terminal, aucun rendu CGA/son effectif.
**À VÉRIFIER** : interruption au clavier de l'animation du monstre 15 ;
le port et l'oracle utilisent le chemin original sans touche, six images.
Cette animation peut consommer moins de tirages si une touche la coupe.
Les erreurs DOS de division et la boucle sans compétence connue sont
signalées ERROR, sans correction arbitraire ni export d'une partie terminée.

**MANQUANT** : entrées/actions des vingt lieux, autres dialogues/objets,
déplacements des labyrinthes, Alfol et Meduz, voyage spatial, objectifs,
recréation après défaite dans la boucle générale et rendu graphique.
Ce lot ne certifie pas encore un parcours entier jusqu'à la victoire.

## Vérification

`tools/compare_exploration_binary.py` exécute OVL00FB et C_A0DD avec RNG
original, puis les vrais combats imbriqués. Les sorties graphiques/BIOS
sont abstraites. Profils, compétences, équipement, HP/MP/maxima/statuts,
clés, compteurs, disponibilité, graine et flags sont comparés à chaque
frontière clavier et à la fin. Les tests incluent les 23 scènes, montagnes,
limites, villes 0..19, silence/son, compagnons, récupération/poison,
niveaux, rencontres et retour de fuite. Les preuves reproductibles et
empreintes sont dans `build/audit/exploration_manifest.json` ; elles ne
sont pas redistribuées avec les originaux.


Campagne obtenue : **798 scénarios, 848 frontières clavier/retour,
49 015 appels au RNG original, 25 rencontres, aucun écart**. Les onze
classes de rencontre errante ont chacune une graine dédiée. Huit suites
CTest et quinze tests des frontends passent. Le rapport local complet
est `build/audit/exploration_comparison.json`.

## Révision lors des lots cité (11 et 12)

Le bilan précédent décrit le banc avant correction des abstractions
sonores. Les pas C67D7 et les musiques C6352/C62B7 sont désormais exécutés
dans l'oracle et leurs tirages conservés dans le C : avant RANDOM24 et
les décors, comme OVL00FB. `world.fast_moves`/A74D est transporté depuis
le monde généré. Les présentations de monstres et combats imbriqués
conservent également leurs sons et effets de couleur. Les décomptes
actuels figurent dans `validation_binaire.md` et remplacent la campagne
antérieure pour ce périmètre.

La fuite de surface produit 3×RANDOM255, sans offset 1 000. La position
−1 avant l'arrivée automatique est donc acceptée. Seuls les pas sud/ouest
appliquent le rebouclage inférieur ; les pas nord/est conservent les
positions basses du COM. Le corpus ajoute 56 cas autour de cette plage.
Les nouvelles règles ne dérivent pas du Python historique.

L'entrée dans la cité des crânes est maintenant reliée au frontend ;
voir `spec_navigation_cite.md`. Les autres lieux restent hors périmètre.
