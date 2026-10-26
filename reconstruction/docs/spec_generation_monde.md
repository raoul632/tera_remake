# Génération mondiale — lots 7 et 8

## Périmètre et preuves

**CONFIRMÉ — BIN/PAS/DATA** : `engine/src/world.c` porte OVL0000 après
la saisie du scénario/difficulté et RANDOMIZE, puis **OVL0010 entier**.
Le frontend fournit la graine initiale ; il conserve sa progression pour
OVL0022, sans réinitialisation entre cartes, personnages et héros.
Les empreintes, extraits et rapports restent sous `build/audit/`, ignoré
par Git. Les originaux et leurs ressources ne sont pas redistribués.

Les sept colonnes et 1 100 cases conservent le record DOS : quatre murs,
contenu, monstre et type de case. Villes, montagnes, positions particulières,
placements des neuf compagnons, 38 indices, quatre mots secrets et H
proviennent du générateur original. Les textes conservent CP437 et la
troncature Pascal ; le frontend décode seulement pour leur affichage.

## Calendrier du générateur

OVL0000 remet les cartes à zéro, trace les onze mondes (225 pas chacun)
et applique les raccords après chaque monde. La direction interdite
persiste d'un monde au suivant. La population parcourt 899 à 1 en ordre
décroissant ; les expressions Pascal évaluent leurs jets même lorsque
les autres conditions du AND sont fausses. La case 200 conserve la
variable monstre antérieure : aucune correction inventée.

Le contrôle des onze conditions obligatoires peut rejeter la génération
entière. Tous ses tirages sont consommés avant de recommencer. Les
marqueurs finaux, Alfol, clés, argent, énergie et drapeaux sont ensuite
initialisés. `max_draws` est une limite technique : son dépassement
interrompt sans modifier l'état fourni, et ne change pas les probabilités.

OVL0010 génère noms et caractéristiques, montagnes, vingt villes,
placements, indices/mots/H, placements complémentaires et population
des cases 900 à 999. Les villes refusent les doublons ; cela n'autorise
pas à imposer cette même politique aux compagnons.

## Anomalie de placement conservée

**CONFIRMÉ — BIN** : BA2F teste une collision avant d'écrire le résultat,
mais quitte tout de même la routine lorsque le candidat satisfait les
autres conditions. Deux compagnons peuvent donc partager leur position.
Pour les appels imbriqués BB28, le résultat conserve un mot de pile,
qui peut être négatif. Le C reçoit ce résidu via
`placement_result_residue` ; le pont fournit -211 (FF2D) pour le contexte
audité : pile DOS initiale FFFE, entrée d'overlay FFFC.

**À VÉRIFIER** : autres dispositions de pile DOS, notamment mémoire
insuffisante pour la pile initiale FFFE. Ce mot brut dépend du contexte,
pas uniquement de la graine. Le frontend ne relance pas un tirage pour
masquer l'anomalie. Les indices et positions produits sont comparés
exactement, y compris les valeurs inattendues.

## Utilisation et limites

`frontend/world.py` appelle les générateurs C puis la création interactive
du héros, et exporte `build/new_game.json`. `--no-create` exporte un état
d'audit avant le héros. Les cartes et le contexte sont préservés lors de
la création, du recrutement et du combat.

**MANQUANT** : boucle d'exploration, déplacements, déclenchement automatique
des rencontres, rendu graphique et parcours complet. Cet export est
un état généré, pas un conteneur de sauvegarde DOS certifié.

Validation reproductible : `tools/compare_world_binary.py` exécute les
instructions et le RNG originaux, avec BIOS/présentation abstraits. Il
compare cartes, profils, géographie, indices, mots, H, drapeaux et graine.
La graine 23 couvre une carte refusée puis régénérée. Le banc réutilise
aussi la mémoire issue des overlays originaux pour comparer la création
du héros après génération, y compris refus et recommencement.
