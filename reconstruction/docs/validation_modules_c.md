# Tests C directs des derniers modules — 16 octobre 2026

Six suites supplementaires sont enregistrees dans CTest. Elles appellent
directement la bibliotheque C : aucun processus Python, fichier de ressource,
emulateur ou frontend n'est requis pour les executer.

Les fixtures synthetiques communes sont explicites dans
`tests/module_test_support.h`. Elles isolent des regles et ne representent
ni une partie originale, ni des statistiques proposees pour le jeu.
Les attentes viennent des specifications BIN/PAS/DATA existantes ; les
vecteurs de score simples sont analytiques. Aucune nouvelle campagne de
comparaison au binaire n'a ete executee pour ce lot.

| Suite / fichier sous tests/ | Regles et cas limites verifies |
|---|---|
| commerce_module_contracts / test_commerce_c.c | Les 19 entrees du catalogue ; argent insuffisant/exact et debordement de money+1 ; inventaire plein et HS ; energie/cartes/cle sans slot ; slot vide sans tirage ; vente de l'objet 25 et conservation du prix precedent hors catalogue ; argent et jours signes ; hopital gratuit, restauration des dix personnages et statuts des inactifs preserves ; double ESC initial |
| space_module_contracts / test_space_c.c | Refus et absence de pilote, dix jets meme pour les inactifs, dernier pilote apte ; saisie du carburant et montant exact ; neuf directions via le calcul de cible ; tirs corrects/absents, compteur double, XP, energie, sept jours et retour ; essai obligatoire avec zero pirate ; HALT ; nombres de tirages avec et sans son |
| alfol_module_contracts / test_alfol_c.c | Bouclage de surface et jour ; rembarquement au point zero meme apres refus ; portes O/U et cle preservee ; sortie des batiments sans jour ajoute ; medias : O seul, seuil 1000, paiement repete mais objectif unique ; alias MP[6]/octets d'indice pour D9B66=16, preservation des octets voisins et refus du contexte 17 |
| meduz_module_contracts / test_meduz_c.c | Meditation : refus, gain unique pour tous les actifs HS inclus, octets 255 vers zero, inactifs preserves ; objectif conditionne au mot et a l'extension mentale, compteur signe et absence de recompense repetee ; jour devant un mur ; combat force 22 ; retour meme apres mort, sept jours signes ; retour arriere et terminaison a la dixieme lettre |
| amarande_module_contracts / test_amarande_c.c | Les dix facades sans son, nombres de tirages et absence de replay ; condition OR des PNJ et conservation d'A750 ; panneau avec division negative vers zero ; lectures distinctes de U ; halte, vitesse, aide, carte et son ; filtres d'invocation ; vrai combat de PNJ avec victoire marquant seulement HS |
| ending_module_contracts / test_ending_c.c | Chaque drapeau d'objectif requis independamment du compteur ; six vecteurs score/texte/Real48 dont score negatif, denominateur signe et troncature de 1/3 ; erreur 2 avant musique ; condoleances sans score ni division ; calendrier sonore identique entre les deux fins ; acquittement unique |

Les refus de touches hors octet et les appels apres fin sont controles
dans les modules concernes, avec comparaison de la session avant/apres.
Les tests d'orchestration existants restent complementaires : ils couvrent
les transitions entre ces modules, pas seulement leurs regles isolees.

## Execution et resultat

```powershell
cmake -S reconstruction -B reconstruction/build/cmake
cmake --build reconstruction/build/cmake
ctest --test-dir reconstruction/build/cmake --output-on-failure
```

Resultat execute : **18 suites sur 18 reussies**, dont les six nouvelles.
Le moteur et ces six executables compilent avec GCC C99,
`-Wall -Wextra -Werror -pedantic`. Leurs assertions restent actives meme
si NDEBUG est defini, afin de ne pas neutraliser ces contrats en Release.

**CONFIRME — tests C** : couverture directe des regles enumerees et absence
de regression dans les suites existantes. Aucun changement des regles du
moteur n'a ete necessaire dans ce lot ; les ajouts restent sous reconstruction/.

**A VERIFIER** : parcours integral depuis une generation reelle et comparaison
globale de TERAMAIN ; interruptions clavier des animations. Ces tests ne
certifient ni la couverture exhaustive des branches ni une partie complete.
La persistance durable reste un lot separe. Le rendu et l'audio audible
appartiennent au contexte 4 / SDL.
