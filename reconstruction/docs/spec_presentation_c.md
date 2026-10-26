# Contrat de présentation du core C

## Provenance et portée

Ce contrat expose les décisions déjà prises dans les modules C, dont les
sources BIN/PAS/DATA sont décrites dans leurs spec_*.md. Il ne reconstruit
aucune règle depuis une comparaison d'états faite dans SDL.
Statut : **CONFIRMÉ — SOURCE C / tests C exécutés**. Aucune nouvelle preuve BIN n'est
revendiquée pour cette couche d'observation.

## Deux canaux complémentaires

Complément personnages : `TERA_PRESENT_CHARACTER` transmet une fiche autonome
pour H/I et chaque pause H/S ; `tera_character_card` expose la même projection
sans effet de bord. Voir [contrat des écrans](spec_character_screens_c.md).

Complément lieux : `game_view.scene` projette les faces et éléments visibles
sans révéler un coffre fermé. Les paysages gardent les coordonnées des tirages
existants dans le rapport de déplacement. `TERA_PRESENT_INTERACTION` expose
extension mentale, médias payés ou argent insuffisant, sans effet RNG ni
extension du format de sauvegarde. Voir [contrat des scènes](spec_scene_interactions_c.md).

- Vue persistante : `tera_game_view` décrit l'état actif et la saisie
  attendue (commande, menu, confirmation, mot, personnage, objet, pause,
  création, tir). Consultation sans effet sur le RNG ni callback.
- Événements : `TeraPresentationSink`, service facultatif emprunté, attaché
  aux données runtime du jeu comme la source d'entrée. Les modules émettent
  au point où ils décident mouvement/refus, message/indice, coffre/piège,
  inventaire, commerce, recrutement, objectif, changement de zone et fin.
  Les événements de combat existants sont aussi relayés, sans nouvelle règle.

Le callback reçoit une valeur typée sans pointeur vers une union de session.
Il doit copier cette valeur s'il veut la conserver après le retour. Il ne
doit pas modifier/rappeler le core. Il n'y a pas de file bornée cachée dans le
core, ni perte silencieuse : chaque émission appelle immédiatement le service.
Le frontend choisit son stockage et doit rendre un éventuel débordement visible.
Un sink NULL ne change ni règles, ni touches acceptées, ni RNG.

Les événements sont émis dans l'ordre d'exécution, y compris ceux précédant
une erreur technique ultérieure. Leurs champs de lieu décrivent l'instant
de l'émission ; un changement de zone conserve aussi le lieu de départ.
Un refus d'API (retour 0) ne doit pas publier d'événement ; un mouvement
accepté mais bloqué est au contraire un résultat de jeu observable.

Les IDs de messages restent dans leur espace original explicite : phrases
C4473, MESSAGE*, indices générés, inscriptions Seth. Ne pas inventer de texte
original, de variante ou de tirage pour remplir une lacune. Les tirages de
phrases déjà présents mais dont le résultat était jeté doivent être conservés
et transmis, sans ajouter de RANDOM.

## Sauvegarde et services

Le sink et son contexte ne sont jamais sérialisés. Le format moderne v1
reste inchangé. Après chargement, rattacher explicitement le nouveau service
avec `tera_game_set_presentation`. Ce rattachement ne rejoue aucun événement.
Pour observer la création ou l'arrivée initiale, fournir le sink dans le monde
avant `tera_game_create/start`. Les copies de sessions empruntent le même sink ;
ce dernier ne doit pas pointer vers une ancienne adresse de session déplacée.

La temporalité du monstre 15 et l'audio audible restent des blocs ultérieurs.
Ce contrat n'ajoute pas d'attente, de SDL, d'horloge ou de ressources au core.

## Valeurs transmises

| Événement | Données utiles | Point d'émission |
|---|---|---|
| MOVE / BLOCKED | commande, départ, arrivée, position précédente, mur | Décision de marche ; mur `~0u` sur surface sans mur |
| ZONE | ancien monde/position/OnAlfol, nouveau lieu dans l'en-tête | Entrée/sortie/niveau, voyage spatial ou retour Meduz |
| PLACE | ID de lieu original | Détection d'un lieu sur Amarande |
| MESSAGE | espace de ressource, ID/sélecteur, variante | Réponse C4473, dialogue, indice ou inscription |
| CHEST | commande, acteur, contenu d'origine, outcome, état ouvert, récompense | Résolution du coffre/ramassage, avant retour au routage |
| TRAP | type original - 0 gaz, 1 explosion, 2 cyanide, 3 lame, 4 déjoué - et acteur | Après décision, avant effets sur les personnages |
| ACTOR | raison, ID, HP/MP/statut résultants | Piège, soin ou dépôt radioactif, sans recalcul par SDL |
| INVENTORY | commande, sujet/cible, slot, ID d'objet avant retrait, outcome | Fin de l'opération, même annulée |
| SHOP | outcome original 1..6 ; acteur/objet/prix ou semaines d'hôpital | Résultat de chaque opération, même si le magasin reste ouvert |
| RECRUIT | outcome, négociateur, masques disponibles/recrutés | Résolution du recrutement |
| OBJECTIVE | index 0..3 | Passage du drapeau concerné à 1, une seule fois |
| RESIDENT | commande/choix, outcome Amarande, acteur, directions est/nord | Résultat des panneaux/cartes/fiches avant écrasement |
| ENDING | victoire ou condoléances, score déjà calculé | Entrée dans la fin ou les condoléances ; aucun score inventé |
| COMBAT | événement de combat existant, acteur, lettre, valeur | Relais du flux TeraCombatObserver existant |

L'en-tête donne toujours `source`, `world`, `position`, `on_alfol`. Un événement
MOVE décrit le pas avant les effets d'arrivée ; une suite ZONE peut le suivre.
Les effets ACTOR peuvent précéder le résultat SHOP/INVENTORY ; TRAP précède ses
effets et le résultat CHEST. Un effet de radiation photographie les dix fiches,
ce qui conserve aussi le cas historique D9B66=16 sans inventer une cible normale.
Les snapshots ne prétendent pas être une formule de dégâts ou une animation.

`TERA_TEXT_RESPONSE` conserve les IDs C4473 déjà nommés dans Amarande.
`TERA_TEXT_COMMAND_RESPONSE` conserve l'octet de commande des autres branches
CAFE8 et sa variante effectivement tirée ; sa correspondance avec les entrées
des fichiers est maintenant documentée et résolue dans TERA_ASSETS.md.
MESSAGE utilise ses IDs d'origine, HINT les IDs 1..38, SETH 100/101.
`TERA_TEXT_NO_VARIANT` signifie qu'aucune variante n'a été sélectionnée par
ce chemin ; ne pas transformer cette valeur en tirage supplémentaire.
Les bornes numériques ne sont pas des indices de tableaux universels.
CHEST utilise `actor=~0u` quand aucun personnage n'a été sélectionné (carte,
clé ou annulation). INVENTORY transmet un ID d'objet uniquement après une
lecture de slot/destinataire, jamais un objet arbitraire d'une fiche annulée.

## Vue de saisie

`TeraGameView.prompt` sélectionne le lecteur actif, y compris la création du
successeur, les combats imbriqués et le transfert d'inventaire d'un dialogue.
Les types sont COMMAND, MENU, CONFIRM, WORD, ACTOR, SLOT, PAUSE, NAME, GENDER,
SPLIT, BONUS, KEEP, GATE, DIGIT, SHOT et TERMINAL.
`source` et `command` distinguent les menus ; `actor/slot` donnent le contexte,
`selecting_target` distingue un destinataire d'inventaire. `input` contient
seulement la saisie courante, jamais le mot secret attendu.
`pool/remaining` sont les valeurs de répartition/bonus du core.
`confirm_key/accepts_enter` ne sont significatifs que pour CONFIRM/GATE :
les portes exigent O/U, les médias Alfol et le carburant exigent O.
Ces informations servent à afficher une invite, pas à court-circuiter
`tera_game_key` avec une validation supplémentaire côté frontend.

Après COMPLETE issu d'ENDING, la vue garde le score et le résultat final.
Les condoléances suivies d'une succession ne deviennent pas une victoire.

## Exemple d'utilisation autonome

```c
static void receive(void *context, const TeraPresentationEvent *event)
{
    /* Copier *event dans le journal de l'hôte ; ne pas rappeler le core. */
    (void)context;
    (void)event;
}
TeraPresentationSink sink = { receive, NULL };
/* Après génération, avant création : */
world.world.characters.combat.presentation = &sink;
tera_game_create(&game, &world, NULL, NULL);
/* Après chargement : */
tera_game_set_presentation(&game, &sink);
/* Pour chaque saisie : */
tera_game_key(&game, key);
tera_game_view(&game, &view);
```

## Validation et limites

`test_presentation_c` vérifie les refus atomiques/sans événement, les vues
sans mutation, le bouclage de surface, obstacles/portes O-U, entrée de zone,
achat/hôpital, retrait d'objet, coffre/récompense/piège/effets, phrases/indices,
recrutement, objectif non répété, victoire/score terminal, création et service
attaché pendant un combat. Les valeurs copiées survivent à l'écrasement des
unions. Les sauvegardes restent exactement v1 / 11 521 octets sur ces fixtures,
et le sink est absent après décodage, sans événement rejoué au rattachement.

Deux campagnes `campaign_presented_1986/12345` vont jusqu'à la victoire avec
le service attaché et les reprises actives. Elles conservent les mêmes compteurs,
digests et résultats que les campagnes existantes. Les quatre événements
OBJECTIVE sont reçus chacun une seule fois malgré les morts et reprises.
La graine 1986 produit 3 429 événements et 51 changements de zone ; la graine
12345 produit 3 101 événements et 35 changements de zone. Le masque des
objectifs observés vaut 15 pour les deux. Résultat global : **29 suites CTest
sur 29 réussies**, aucune exclue. Configuration séparée sans SDL : les trois
contrats de vue, présentation et sauvegarde passent aussi.
Ce n'est pas une certification exhaustive de toutes les branches originales.

Le diagnostic SDL consomme maintenant ce flux et la nouvelle invite. Il
conserve les 64 derniers événements, avec compteur visible des anciennes
entrées remplacées. Les textes/sprites et les sons audibles restent hors de
ce bloc. Les six images temporisées du monstre 15 restent à traiter selon
TERA_UNKNOWN.md ; leur calendrier RNG n'a pas été déplacé dans SDL.
