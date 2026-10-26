# Orchestration du moteur C — contexte 3

Le point d'entree commun est `engine/include/tera/game.h`, implemente dans
`engine/src/game.c`. Il relie les modules existants sans interface, processus
externe, fichiers, horloge ni dependance Python. La bibliotheque reste C99.

## Provenance et perimetre

Les raccordements suivent TERAMAIN et les contrats BIN/PAS/DATA exposes par
les specifications exploration/temps, navigation, dialogues/succession,
Kronopolis/Seth, spatioport/Alfol, Meduz/fin et Amarande secondaire.
Les anciens outils terminal ont servi a inventorier les raccordements ;
ils ne constituent pas une preuve de fidelite a eux seuls.

**CONFIRME — tests C d'integration** : le routage et les enchainements
enumeres ci-dessous fonctionnent directement dans la bibliotheque.
Les campagnes BIN des modules restent des preuves distinctes. Cette nouvelle
suite ne certifie ni une partie complete ni l'execution integrale de TERAMAIN
contre le binaire.

## Contrat de session

1. Fournir les ressources originales et generer le monde avec les fonctions
   de `world.h` (cartes puis placements/compagnons/mots/H).
2. Appeler `tera_game_create` sur ce monde pour la creation interactive du
   heros, ou `tera_game_start` pour reprendre un heros deja cree.
3. Fournir les lectures avec `tera_game_key`, un octet par appel.
4. Consulter `phase` et le module actif. `READY` attend une commande de jeu ;
   les phases de module attendent leur prochaine lecture originale.

La session appartient a l'appelant ; aucun etat global mutable supplementaire.
`state` contient la derniere operation terminee. Pendant une attente, les
donnees en cours sont dans `active.<module>`, y compris le combat imbrique.
La structure ne constitue pas un format de sauvegarde : elle contient une
union et des pointeurs d'observation, a ne pas ecrire brute sur disque.

`TeraGameContext` conserve les globals hors record : positions de retour de
Meduz et du spatioport, pirates restants, D9B66, variante de PNJ et commande
obligatoire apres arrivee mortelle. Pour une reprise a Meduz/Alfol, fournir
ce contexte explicitement. La position de retour de la Cite/Seth appartient
a `TeraExplorationState.city_backup_position`.

Les touches refusees ne modifient pas la session. Les erreurs techniques
des modules menent a `ERROR`, sans victoire, fuite ou succession fabriquee.
`COMPLETE`, `QUIT` et `HALTED` sont trois fins distinctes. Q n'est pas ajoute
comme commande du jeu : la sortie residente passe par H/Echap/confirmation.
L'arret d'un outil de diagnostic appartient a cet outil.

## Enchainements

- Amarande : mouvement, jour et combats, facade du lieu, reprise d'arrivee
  apres fuite. A74F/AFD1 utilise Position-- puis le vrai pas nord ; aucun
  deuxieme deplacement ou jour arbitraire. Le tirage PNJ est conserve.
- E : Kronopolis avec confirmation puis commerce ; spatioport avec pilote,
  ravitaillement et tirs ; Cite/Seth avec confirmation et arrivee.
- Pyramide/M : confirmation, mot de voyage, Meduz. Ses retours gardent sept
  jours et les tirages de transition, avant une eventuelle fin de partie.
- Alfol : atterrissage apres le vol, surface/batiments ; R au point zero
  reprend les pirates et le vol retour, puis la facade du spatioport.
- Navigation des overlays : portes, niveaux, coffres, invocations et combats
  restent delegues aux sessions existantes ; retour Cite/Seth puis facade.
- N/S et A/B : dialogues/recrutement/transfert et inventaire ; ensuite reprise
  du contexte appelant, dont le test de sortie de Meduz.
- Defaite : sortie de l'overlay avant condoleances, puis creation du
  successeur sans regeneration des cartes/objectifs. L'arrivee automatique
  du successeur est consommee par le C.
- Arrivee mortelle : Seth, batiments d'Alfol et Meduz autorisent encore une
  commande avant le test UNTIL. Une lecture de confirmation ne compte pas
  comme une nouvelle commande ; une commande refusee par l'API non plus.
- Quatre objectifs : fin au retour sur Amarande ; victoire/score ou
  condoleances sans successeur. Aucun score/musique anticipe dans un overlay.
- Destruction spatiale : `HALTED`, sans succession terrestre.

Les transitions automatiques ne prennent aucune touche supplementaire,
ne resement pas le RNG et n'ajoutent aucun tirage propre. Les seuls tirages
proviennent des modules originaux, y compris les facades et les sons abstraits.
Le callback de combat est transmis a chaque sous-session, sans modification.

## Validation directe, sans Python

```powershell
cmake -S reconstruction -B reconstruction/build/cmake
cmake --build reconstruction/build/cmake
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

`tests/test_game_c.c` couvre onze groupes de contrats sur des fixtures
synthetiques annoncees :

- marche/jour/facade, arrivee et fuite, et refus atomiques ;
- dialogue, achat, confirmations, annulations et sortie residente ;
- entree et sortie de Cite avec comparaison de l'etat complet ;
- sortie de Meduz et score differe ;
- commande obligatoire Seth, succession, conservation du monde ;
- aller-retour spatial/Alfol, jours, positions et HALT ;
- creation puis arrivee, et contexte de reprise invalide ;
- vrai combat imbrique : memes touches, etats, graine et evenements que la
  chaine directe de navigation/combat, puis effacement du monstre vaincu ;
- commandes residentes apres arrivees mortelles Alfol/Meduz ;
- voyage vers Meduz, refus du mot et erreur de donnees de monstre.
- arrivees mortelles reelles aux trois entrees, avant leur commande
  obligatoire ; retour immobile de Seth avec les tirages de facade.

Validation executee : compilation GCC C99 avec les avertissements du moteur
traites comme erreurs, **12 suites CTest sur 12 reussies**, sans lancement
de Python ni d'interface.

**A VERIFIER** : parcours integral d'un monde genere sans repositionnement,
comparaison globale des frontieres de TERAMAIN, interruptions clavier des
animations. **MANQUANT** : persistance C durable et format de reprise sur
disque (la structure de contexte est prete a etre serialisee explicitement).
Le conteneur DOS reste optionnel selon le cadre, qui autorise un fichier
moderne. Graphismes et audio audible appartiennent au contexte 4 / SDL.

Mise à jour du 16 octobre : les interruptions de clavier ont désormais leurs
contrats C et une comparaison native ciblée au binaire, voir
`spec_entrees_gameplay.md`. La sauvegarde explicite et la reprise READY sans
rejouer les transitions sont implémentées : voir `spec_sauvegarde_c.md`.
Les deux campagnes générées ont aussi leurs variantes avec reprises sur disque.
La comparaison globale de TERAMAIN est désormais exécutée sur les deux
campagnes, avec les véritables overlays DOS et le score Real48 : voir
`validation_teramain_dos.md` pour les résultats et limites.
