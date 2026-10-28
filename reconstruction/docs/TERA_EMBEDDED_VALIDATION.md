# Validation Original — 28 octobre 2026

CONFIRMÉ — DATA / tests C / OBS du frontend Windows.

Base : `master`, commit `badd59f`, branche `codex/original-embedded-resources`.
Worktree : `E:/dev/PYTHON/PROJETS/tera_remake/reconstruction/build/original-embedded`.
Les modifications Enhanced du répertoire principal n'ont pas été intégrées.
Cette livraison reste locale, sans publication GitHub.

## Résultats mesurés

| Vérification | Résultat |
|---|---|
| Reconstruction DATA | 15/15 fichiers identiques octet par octet et SHA-256 |
| Textes et profils | Sept tables CP437 avec fins inutilisées ; 22 × 6 valeurs signées identiques |
| Images | 82 sprites, 92168 pixels d'atlas, six fonds 320 × 200 |
| Masques | 82 PNG de masques identiques à la règle par octet nul |
| Chargeurs C compilés / externes | Textes, pixels, descripteurs, plans et monde généré identiques |
| Édition | JSON CP437 et pixel PNG modifiés répercutés ; RGBA rejeté |
| Reproductibilité | Deux générations identiques, également dans la copie sans DATA |
| C99 Release sans SDL | Compilation code 0 ; 34/34 suites CTest, aucune exclue |
| C99/SDL3 Windows x64 Release avec oracle Unicorn | Compilation code 0 ; 47/47 suites CTest, aucune exclue |
| Campagnes SDL 1986 et 12345 | Victoires ; 23 et 19 sauvegardes/rechargements F5/F9 |
| Campagnes BIN/C 1986 et 12345 | Comparaisons TERAMAIN réussies |
| Contrats embedded après ajout du SKIP si DATA absent | 2/2 réussis avec DATA présent |
| Archive extraite, SDL dummy et Windows, reprise séparée | Trois processus code 0 |

SHA-256 du C généré à partir des PNG/JSON, identique dans les deux arbres :
`75573e0c320f565c2b7b4b74c1561bf537375eef0038609a98d5c3667f6405f7`.
Les digests de campagne restent `2d0ca4bb` (1986, score 8) et `b246cd73`
(12345, score 10). Aucun résultat attendu n'a été ajusté. Le renderer des
campagnes SDL utilise les ressources compilées ; le pilote du core garde
les DATA comme oracle de préparation du monde.

## Commandes et journaux

Extraction/génération : [commandes complètes](TERA_EMBEDDED_RESOURCES.md).
Configuration Windows : CMake, `MinGW Makefiles`, GCC 13.2.0
`E:/msys64/ucrt64/bin/gcc.exe`, make du même répertoire,
`CMAKE_BUILD_TYPE=Release`, `TERA_BUILD_SDL3=ON`,
`SDL3_DIR=E:/msys64/ucrt64/lib/cmake/SDL3`,
`TERA_TEST_RESOURCE_DIR=E:/dev/PYTHON/PROJETS/tera_remake/data/compiled program`.
Oracle facultatif :
`TERA_UNICORN_ROOT=E:/dev/PYTHON/PROJETS/tera_remake/reconstruction/build/emulator_deps/unicorn`.
Le build core séparé configure `TERA_BUILD_SDL3=OFF`, sans oracle.

```powershell
cmake --build reconstruction/build/embedded-win64 -j 8
ctest --test-dir reconstruction/build/embedded-win64 --output-on-failure -j 6
cmake --build reconstruction/build/embedded-core -j 8
ctest --test-dir reconstruction/build/embedded-core --output-on-failure -j 6
```

Ces quatre commandes ont réussi (code 0). Journaux sous `reconstruction/build/` :
`compile-win64.log`, `ctest-win64.log`, `compile-core.log`, `ctest-core.log`,
`conversion-verify.log`, `pipeline-test.log` et `ctest-final-check.log`.

Une copie sous `build/converted-source/` contient seulement CMake, engine,
frontend, tests, générateur et ressources éditables. Aucun DATA, COM/overlay
ni C généré n'y a été copié. Configuration `TERA_REGENERATE_ASSETS=ON`,
`Python3_EXECUTABLE=E:/Python/Python310/python.exe`,
`TERA_TEST_RESOURCE_DIR=Z:/absent-original-data`, puis compilation de
`tera_sdl3` et `test_embedded_c` : code 0. Le test embarqué sans argument :
code 0. Journaux `configure-converted-only.log`, `compile-converted-only.log`.
Le C régénéré est identique. Le build ordinaire précédent utilise ce C sans
chercher Python.

## Paquet réellement extrait hors du dépôt

Archive :
`E:/dev/PYTHON/PROJETS/tera_remake/reconstruction/delivery-original-20261028/TERA-Original-Port-v0.2.0-embedded-windows-x64.zip`.

SHA-256 : `4b78152885ef2581379ee1c4cad0fae396b4d3658ef4ef9dba5f32b9d8b7e606`.

Commande exécutée, code 0 :

```powershell
./reconstruction/tools/test_release_autonomy.ps1 -Archive 'E:/dev/PYTHON/PROJETS/tera_remake/reconstruction/delivery-original-20261028/TERA-Original-Port-v0.2.0-embedded-windows-x64.zip' -Destination 'C:/Users/sshpc/.codex/visualizations/2026/10/28/01a18113-cf63-7f01-a18c-02eb8df34dea/TERA-isolated-final'
```

Destination neuve hors dépôt, données et builds. Liste autorisée vérifiée :
exe, SDL3.dll, notices et documentation uniquement. Aucun des quinze fichiers
séparés, COM/overlay, Pascal, Python ou ressource de développement.
Le programme n'écrit aucun fichier dans le paquet.

Trois lancements de l'exe extrait, depuis `unrelated-cwd/` :

1. `--smoke --save .../dummy.sav --capture .../dummy.bmp`, pilote dummy : code 0.
2. Même scénario, pilote **Windows** : code 0, capture inspectée.
3. Processus normal distinct `--load .../windows.sav --save .../windows.sav` :
   fenêtre SDL démarrée, fermeture ordinaire de sa fenêtre, code 0, stderr vide.

Les scénarios smoke ferment le titre, créent le personnage et marchent via
les événements SDL de production. Ils comparent l'état au core direct,
affichent un message sélectionné par le core, sauvegardent, se déplacent,
rechargent et comparent les octets sérialisés et le RNG. La capture contient
le paysage, le héros et le texte original.

L'environnement est vidé puis limité à SystemRoot, WINDIR, TEMP/TMP dans le
dossier isolé, PATH=`C:/WINDOWS/System32` et les trois pilotes SDL. Aucun chemin
du dépôt ni variable Python. Inventaire, arguments, environnements, stdout,
stderr et codes : `inventory-before.json` et `runs.json` sous la destination.
Les sauvegardes et captures restent dans son répertoire de travail.
Les imports PE requièrent seulement SDL3 et les bibliothèques système Windows/UCRT.

## Problèmes rencontrés et limites

La première compilation Release a révélé que `NDEBUG` supprimait des assertions
et préparatifs de tests historiques. Les cibles `test_*` gardent désormais les
assertions actives. Les premiers parcours ont ensuite échoué dans
`ntdll!RtlVirtualUnwind2` au `longjmp` de succession, confirmé par GDB.
Conserver les frame pointers n'a pas corrigé le problème. Les trois pilotes de
campagne MinGW x64 utilisent finalement `__USE_MINGW_SETJMP_NON_SEH`, option
prévue par le `setjmp.h` installé. Cela concerne seulement les tests C sans
nettoyage SEH. Le core et les règles du jeu restent inchangés.

L'autonomie est confirmée sur cette machine Windows x64, avec rendu logiciel
SDL Windows et scénario automatisé. Pas de test sur VM Windows vierge, autre
OS, autre matériel graphique ou installation sans UCRT. La sortie audio réelle
n'a pas été écoutée (audio dummy). Le test isolé n'est pas une partie entière
jouée manuellement ; les campagnes complètes sont automatisées et séparées.
Aucun pourcentage global de fidélité DOS n'est mesuré. Les écarts de présentation
déjà documentés dans le projet restent hors du périmètre de cette livraison.
