# Anomalies du port original — audit du 26 octobre 2026

Les priorités décrivent l'impact ; les statuts décrivent la preuve. Un risque
non testé n'est pas une anomalie démontrée. Aucun P0 ouvert démontré dans cet
audit, aucun P1 ouvert démontré après correction ; cela reste limité aux essais.

## AUD-001 — P1, DIVERGENT corrigé : graine SDL fixe

Original : OVL0000, après READ du code scénario, appelle RANDOMIZE. La routine
BIN 0F14 demande l'heure DOS par INT21/2C, écrit DX au mot 01FC et CX au mot
01FE. Quatre appels originaux exécutés dans le nouveau banc confirment les
graines, dont 23:59:59.99 -> 0x173b3b63 et 12:34:56.78 -> 0x0c22384e.

Avant : `app.c` initialisait `seed=1986` puis la fournissait systématiquement
à la génération en l'absence de `--seed`. Toute nouvelle partie par défaut
recommençait donc avec la même séquence. Cause : valeur de diagnostic devenue
valeur de lancement normale ; le core disposait déjà du format d'horloge correct.

Correction ciblée SDL : `startup_seed` lit l'heure locale SDL, tronque les
nanosecondes aux centièmes et appelle `tera_random_clock_seed`. Une graine
explicite reste prioritaire, y compris zéro. `--smoke` conserve 1986. Une
reprise ne lit pas l'horloge et conserve le RNG enregistré. Un échec d'horloge
refuse le lancement au lieu de substituer une graine inventée.

Tests : `sdl3_startup_randomize` couvre horaires, centièmes, graine zéro/max,
smoke, erreurs d'horloge/conversion et seconde hors plage DOS. Le nouveau
`original_conformance/compare_original.py` exécute les vraies instructions
RANDOMIZE et compare le core. Deux campagnes SDL et l'autonomie repassent.
Limite : le moment de lecture n'est pas le même que dans les menus DOS
absents ; l'égalité de la graine réelle entre deux lancements distincts
n'est pas revendiquée. Aucun algorithme ni ordre de tirage du core modifié.

## AUD-002 — P3, DIVERGENT corrigé : haut des cadres texte

Preuve OBS : `build/port-audit/dos-captures/tera_001.png`, capture native DOSBox
640x400 au premier écran de nom, SHA256
`1d9f7867721dfc5d256b8e3249ef991df305f3608562579ac6411a4b627012c3`.
Preuve PAS : OVL0022/00CB INNER_BA2F écrit D5, CD répété, B8 en haut,
B3 sur les côtés, C0/C4/D9 en bas. Le code C utilisait DA/C4/BF en haut.

Correction : trois glyphes de `tera_text_box` modifiés ; dimensions, couleurs,
contenu et logique du core inchangés. La même primitive sert aux cadres
création/bonification/succession issus de la même routine historique.

Régression : bande RGB de 632x16 contenant titre et bordure, hors curseur.
Empreinte FNV attendue de la capture DOS : c3f17565. Avant : cd5aea25,
assertion échouée, CTest 8 (`frame-red.log`). Après : identité, retour 0
(`frame-green.log`). Le reste de l'écran n'est pas déclaré identique.

## AUD-003 — P2, DIVERGENT corrigé : durée de l'équipement acheté

Preuve PAS OVL00BC, lignes 69–78 : acquisition, C4EFE, cadre, DELAY(2000),
restauration du catalogue. Preuve C : `interactions.c::shop` dessine l'équipement
depuis le dernier événement SHOP ; `app.c` conserve cet événement jusqu'à
une autre commande, sans échéance de deux secondes. L'ancien
spec_menus_sdl_c.md reconnaissait déjà cet écart.

Cause : photographie représentée comme résultat persistant de commande.
Correction lors de la poursuite vers la cible utilisateur de 95–98 % :
`app.c` maintient la photographie pendant 2000 ms, écarte les commandes
pendant cette attente, restaure le catalogue puis purge les touches anciennes.
La fermeture reste disponible. Le core et le RNG ne changent pas pendant le délai.

Nouvelle preuve BIN : ReadChar, à 427F, appelle C4210 ; sa boucle 4218..4231
consomme les touches présentes, puis 4234 attend une nouvelle touche. Le banc
`tests/original_conformance/compare_keyboard_purge.py` exécute cette boucle
originale avec 0, 1, 3 et 15 touches tamponnées : toutes sont jetées, la touche
nouvelle n'est pas consommée par la purge. Seules les E/S clavier et les
vérifications du runtime sont abstraites ; rapport `keyboard-purge.json`.

`sdl3_shop_presentation` utilise le vrai core et le renderer avec une horloge
contrôlée : achat réel dans une fixture synthétique, photo encore visible à
1999 ms, restauration à 2000 ms, état/RNG inchangés pendant l'attente, purge
des événements KEY_DOWN/TEXT_INPUT, nouvel achat après le délai et fermeture.
Le rectangle restauré est comparé en pixels au catalogue avant achat ; les
captures sont `shop-preview-catalog/visible/restored.bmp`. Le mode smoke
continue d'accélérer les attentes ; ce test dédié exerce le mode temporisé.
Pas de mesure DOSBox synchronisée des deux secondes ni de latence physique
revendiquée. Les autres délais de commerce (services, soins) restent à comparer.

## AUD-004 — P2, MANQUANT : séquence de démonstration et code scénario SDL

OBS DOSBox : démonstration animée, choix du lecteur, génération/reprise,
code de trois lettres, choix de difficulté. SOURCE SDL : bannière puis création,
difficulté via CLI, sauvegarde moderne, pas de démonstration ou de code scénario.
Les adaptations du lecteur et du format de sauvegarde sont autorisées par le
cadre projet. L'absence de démonstration reste un écart de présentation original.
Pas de blocage de progression démontré ; aucune réintroduction de la gestion
physique des disquettes n'est demandée par cette anomalie.

## AUD-005 — P2 potentiel, PROBABLE : décors après chargement

Établi côté C : `load_game` remet `textures.scenery` à zéro ; les détails
aléatoires de décor sont des événements transitoires, absents du snapshot.
TERA_UNKNOWN.md décrit leur absence jusqu'au pas suivant. Le RNG, les cartes
et les règles restent conservés par les tests de reprise.
NON TESTÉ ici contre une reprise DOS réelle : le rendu attendu exact à cet
instant. Ne pas étiqueter cette réserve DIVERGENT historique sans cette preuve.
Ne pas ajouter des tirages de rattrapage qui modifieraient le jeu.

## AUD-006 — P2 potentiel, NON TESTÉ : transitions texte et rythme réel

Les répartitions de création sont redessinées depuis l'état courant ; leur
historique/scroll, les restitutions de fonds, les dialogues complets et les
transitions ne disposent pas tous de nouvelles captures DOS correspondantes.
Les pilotes dummy et horloges virtuelles ne confirment ni réactivité du tir
humain, ni timbre PC speaker, ni cadence mesurée sur PC/XT physique.
Ce sont des limites de validation, pas des bugs chiffrés inventés.

## Divergences historiques retrouvées dans les documents

Déjà corrigées avant cet audit, et régressions globales repassées :

- Cité, contenus 104..113 : 96 tirages d'inscription omis. `navigation.c`
  conserve désormais ces tirages ; source BIN et diagnostic décrits dans
  validation_teramain_dos.md.
- Rencontres : l'ensemble Pascal [12..14,16] avait été lu comme 12,14,16,
  laissant passer 13. `exploration.c` exclut maintenant 13 ; mêmes références.
- Blitters, clipping, masques CGA par octet, arcs et ruines face à l'ouest :
  corrections antérieures dans spec_final_comparison_c.md. Les 458 références
  ont été régénérées ici, puis les tests de rendu repassés.

Ces corrections antérieures ne sont pas comptées comme de nouveaux correctifs.
Les anciennes mentions « cartes/aide/police manquantes » ne décrivent plus
l'implémentation actuelle ; voir spec_visual_xt_c.md et le code testé.
