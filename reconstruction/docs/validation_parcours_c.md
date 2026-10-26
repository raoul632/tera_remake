# Parcours complets en C — 16 octobre 2026

**CONFIRME — tests C d'integration** : deux parties generees atteignent les
quatre objectifs et la fin victorieuse avec score via l'API commune `game.h`.
Le conducteur est `tests/test_campaign_c.c`, sans interface ni Python.

## Conditions et provenance

Les deux graines fixes sont **1986** et **12345**, difficulte **0**, son
desactive. Les cartes, rencontres, positions, compagnons, equipements
initiaux, mots et H proviennent des generateurs C existants. Le contexte de
placement DOS est le residu -211 documente pour SP=FFFE ; aucun rejet ou
reechantillonnage supplementaire de graine n'est effectue par le banc.

Les donnees originales locales sont lues en C, sans les incorporer aux
sources ni les modifier :

| Ressource | Taille | SHA256 du fichier utilise |
|---|---:|---|
| NOMPERS | 270 | a128f6b8a0c4984239be373297f584a9c653b7e9e7ae53892e026173f7a3dd2f |
| VALMON | 264 | 902516819b2bb3cf7454cdd74b555bd23a6b9f74d6edc89f3ade7ebcc742e3e9 |

Apres l'initialisation, les donnees de partie ne sont modifiees que par
`tera_game_create` et `tera_game_key`. Le conducteur ne change jamais les
positions, statistiques, HP/MP, argent, cartes, clefs, equipements, objectifs
ou graine. Il ne fabrique pas de resultat de combat et ne recharge aucun
snapshot apres un echec.

Le planificateur connait les cartes et mots generes : c'est un banc
d'integration omniscient, pas une simulation de decouverte humaine des
enigmes. Il calcule ses chemins a partir des murs/montagnes et choisit les
actions de combat selon leur moyenne sur tous les facteurs possibles.
Il ne lit ni ne choisit le prochain tirage de la partie. Les mots sont
fournis aux lecteurs originaux, avec leurs confirmations et validations.

## Parcours execute

1. Generation cartes/placements puis creation du heros avec les vraies
   lectures ; arrivee automatique sans repositionnement.
2. Marche vers les compagnons 8, 4 et 3, negociation/seduction, achats
   d'equipement a Kronopolis. Les soins passent par halte, autels et hopital.
3. Cite : portail, clefs obtenues dans les coffres et leurs pieges, niveaux
   1 a 7, combat d'Arioch et retour par les sentinelles 99 puis 0.
4. Seth : entree, portes avec la cle obtenue en Cite, chemin vers l'autel,
   mot genere, combat et retour sur Amarande.
5. Recherche d'argent dans les coffres generes, depart spatial avec pilote,
   carburant et tirs ; surface d'Alfol, batiments, paiement au bureau des
   medias, sortie puis vol retour.
6. Pyramide : voyage par le mot, combats de Meduz, meditation au catalyseur,
   invocation au sanctuaire, retour avec sept jours, puis fin et score.

Une defaite declenche les vraies condoleances et la creation d'un successeur.
Le conducteur recommence son plan depuis l'etat nouvellement publie : il
recrute a nouveau et reprend les objectifs manquants. Aucun retour a une
ancienne graine ni regeneration du monde. Les limites de commandes, de
soins et de successions sont des gardes du test ; les depasser fait echouer
le test au lieu de modifier le gameplay ou d'annoncer une victoire.

## Resultats executes et traces de regression

| Graine | Tirages de generation | Commandes de jeu | Lectures totales | Combats gagnes | Fuites observees | Successions | Score | Digest |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| 1986 | 17 730 | 2 181 | 3 886 | 36 | 83 | 5 | 8 | 2d0ca4bb |
| 12345 | 17 933 | 1 807 | 3 257 | 46 | 52 | 3 | 10 | b246cd73 |

Table mise à jour après les deux corrections BIN du 16 octobre : tirages des
inscriptions 104..113 et exclusion du monstre 13. Les parcours sont désormais
comparés globalement au DOS : voir `validation_teramain_dos.md`.

Chaque generation accepte sa premiere tentative. Une commande de jeu est
une lecture en phase READY ; les lectures totales incluent les confirmations,
mots, creation et combats. Les fuites sont des evenements du combat : plusieurs
personnages peuvent en produire pendant un meme tour original.

Le banc exige : visite des douze identifiants de monde, quatre drapeaux
d'objectif obtenus, compteur coherent a chaque lecture, aucun objectif perdu,
fin COMPLETE, victoire vraie et absence d'erreur du score. Il verifie aussi
les compteurs et le digest de chaque graine. Le digest 32 bits combine les
touches, phases et graines publiees ; il sert de regression deterministe du
port C courant, pas d'empreinte cryptographique ni de trace de l'original DOS.

**20 suites CTest sur 20 passent**, dont les deux campagnes effectivement
executees ici avec les ressources locales presentes. Le moteur n'a pas
necessite de modification pour faire aboutir ces parcours.

## Reproduction

```powershell
cmake -S reconstruction -B reconstruction/build/cmake
cmake --build reconstruction/build/cmake
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

Pour les seules campagnes et leur sortie detaillee :

```powershell
ctest --test-dir reconstruction/build/cmake -R campaign_generated -V
```

`TERA_TEST_RESOURCE_DIR` est un chemin CMake configurable vers les ressources
locales. Si NOMPERS ou VALMON manque, les campagnes renvoient **77 / SKIP**,
jamais un succes invente. Une ressource presente de taille incorrecte fait
echouer le test. Les autres suites C restent autonomes sans ces fichiers.
La console n'affiche que les jalons et compteurs, sans construire d'interface.

## Limites restantes

Cette preuve remplace le manque « aucun parcours integral sur monde genere »
pour ces **deux graines et cette configuration**. Elle ne certifie pas toutes
les generations, toutes les difficultes, une partie sans defaite, le mode
sonore actif, la decouverte des indices par un joueur ou le timing clavier.
Les suites de modules couvrent separement plusieurs de ces variantes.
La comparaison integrale de TERAMAIN au binaire reste distincte des campagnes
BIN locales deja executees. Sauvegarde/reprise durable et interruptions
d'animations restent a traiter ; SDL appartient au contexte 4.

Mise à jour : `spec_sauvegarde_c.md` décrit deux nouvelles variantes de ce même
parcours, avec 34 et 30 sauvegardes/reprises sur les douze mondes, traces et
scores inchangés. `spec_entrees_gameplay.md` décrit les nouveaux contrats de
clavier et 236 comparaisons natives C/BIN ciblées, sans écart. Ces ajouts
étaient des preuves distinctes. La comparaison globale des deux parties est
maintenant réalisée et décrite dans `validation_teramain_dos.md`.
