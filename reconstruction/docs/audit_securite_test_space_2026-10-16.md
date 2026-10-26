# Audit statique de test_space_c — 16 octobre 2026

## Conclusion

Aucun comportement malveillant n'a été identifié dans les sources examinées,
la configuration de cette cible et les symboles externes des objets conservés.
**Cela ne certifie pas l'exécutable détecté et ne confirme pas un faux positif.**
Le fichier signalé est absent de son emplacement lors de l'audit. Son contenu,
ses imports PE et son empreinte ne peuvent donc pas être vérifiés. L'audit ne
justifie ni restauration, ni exclusion antivirus, ni reprise des exécutions.

## Alerte et périmètre

Informations fournies par l'utilisateur :

- Détection : `VHO:Backdoor.Win64.RShell.gen`, analyse heuristique / Protection Cloud.
- Processus associé : `E:\msys64\mingw32\bin\ctest.exe`.
- Objet : `E:\dev\PYTHON\PROJETS\tera_remake\reconstruction\build\cmake\test_space_c.exe`.
- MD5 annoncé : `00D80393A44F06F3ED56C619CC677F96`.

Ce MD5 est une information de l'alerte, pas une empreinte recalculée pendant
l'audit. L'absence du fichier est constatée ; sa mise en quarantaine effective
n'a pas été vérifiée dans l'antivirus.

Revue manuelle : `tests/test_space_c.c`, `tests/module_test_support.h`,
`engine/src/space.c`, `engine/src/random.c`, `engine/src/combat.c`,
`CMakeLists.txt`, métadonnées et dépendances de compilation disponibles.
Une recherche complémentaire d'appels réseau, processus, injection, persistance,
chargement dynamique et assembleur a porté sur les sources C du moteur et des
tests. Une recherche textuelle seule ne constitue pas une preuve d'innocuité.

## Résultats

### Sources et objets

Le test construit des états synthétiques et vérifie déplacements, coût du
voyage, refus, compteurs et consommation du générateur aléatoire. Les fonctions
spatiales manipulent les structures du jeu et des entrées temporelles virtuelles.
Le générateur est un calcul arithmétique déterministe. Aucun appel réseau,
lancement de commande ou mécanisme de persistance n'a été trouvé dans ce périmètre.

L'examen statique par `nm -u` donne :

| Objet | Symboles externes non résolus |
| --- | --- |
| test_space_c.c.obj | `___chkstk_ms`, `___main`, `__imp___assert`, `_memcmp`, `_memcpy`, `_memset`, `_tera_space_key`, `_tera_space_start`, `_tera_space_travel_start` |
| space.c.obj | `_memset`, `_strcmp`, `_tera_combat_base_roll`, `_tera_random_draw` |
| random.c.obj | Aucun |
| combat.c.obj | Aucun |

Ces symboles sont cohérents avec les sources examinées et la bibliothèque C.
Ils ne remplacent pas l'analyse des imports de l'exécutable manquant. Les objets
conservés ne prouvent pas à eux seuls leur correspondance avec le fichier détecté.

### Construction

La cible `test_space_c` contient un seul fichier de test et dépend de
`tera_engine`, archive statique du moteur C. Elle n'a pas de dépendance Python
ou Unicorn. Les autres exécutables déclarés dans CMake ne sont pas des
dépendances de cette cible.

Les fichiers de commande conservés montrent l'archivage de l'objet du test par
`ar`, puis une édition de liens par `gcc` avec `libtera_engine.a`, `-lm` et les
bibliothèques Windows par défaut de CMake/MinGW : `kernel32`, `user32`, `gdi32`,
`winspool`, `shell32`, `ole32`, `oleaut32`, `uuid`, `comdlg32`, `advapi32`.
Leur présence dans la ligne de commande ne prouve pas que des fonctions de
chacune sont importées ou utilisées par l'exécutable final.

Aucun hook CMake de téléchargement, commande personnalisée, lanceur de
compilateur ou étape pré/post-construction n'a été trouvé dans la configuration
examinée. Les options C sont `-Wall -Wextra -Werror -pedantic` ; les indicateurs
globaux de compilation et d'édition de liens du cache sont vides. Les dépendances
de l'objet du test désignent des en-têtes du projet et de la distribution MinGW.
Les variables `GCC_EXEC_PREFIX`, `COMPILER_PATH`, `LIBRARY_PATH`, `CPATH`,
`C_INCLUDE_PATH`, `LDFLAGS`, `CFLAGS` ne sont pas définies dans le shell d'audit.
L'environnement exact de la compilation passée n'est pas reconstitué.

### Chaîne d'outils et limites de provenance

Les outils résolus se trouvent sous `E:\msys64\mingw32\bin`. Les métadonnées
CMake indiquent GCC 13.2.0 et des pointeurs de 4 octets. `objdump -f` identifie
l'objet du test comme `pe-i386`, architecture i386. La différence avec le mot
`Win64` dans le libellé heuristique ne permet aucune conclusion sur la détection,
puisque l'exécutable lui-même n'est pas disponible.

Les signatures Authenticode de `gcc.exe`, `ld.exe` et `ctest.exe` sont absentes
(`NotSigned`). Cela ne prouve pas une compromission, mais ne fournit pas non plus
d'attestation par signature. La base locale pacman contient notamment des paquets
GCC/binutils UCRT x86_64 ; aucun paquet correspondant au compilateur i686 utilisé
n'a été trouvé par nom. L'origine et l'intégrité de cette installation mingw32
ne sont donc pas authentifiées par cet audit. Les empreintes collectées sont des
repères pour une comparaison ultérieure, pas des certificats de confiance.

`reconstruction/` est actuellement non suivi dans le dépôt parent. Aucun historique
Git authentifié ne permet de prouver la correspondance entre ces sources et les
octets du fichier de l'alerte. Aucun fichier suivi du dépôt historique n'a été modifié.

### Journal de test

Le dernier journal conservé indique 20 tests réussis et un échec immédiat du
test spatial, sans sortie. Il ne permet ni de diagnostiquer un malware ni de
prouver que des versions précédentes de cet exécutable n'ont jamais été lancées.
Aucun test n'a été relancé pendant cet audit.

## Preuves locales et suites

Les éléments suivants sont enregistrés sous
`reconstruction/build/audit/security-20261016/` :

- `sha256-manifest.json` : chemins absolus, tailles et SHA-256 de 24 fichiers
  sources, objets, archive, configuration, journal et outils ;
- `static-evidence.txt` : absence du fichier signalé, symboles externes, format
  de l'objet et commandes de construction conservées ;
- `LastTest.log` : copie du journal existant.

Aucun binaire du projet n'a été exécuté ou recompilé. Aucun fichier en quarantaine
n'a été restauré, aucune protection n'a été désactivée, aucune exclusion n'a été
ajoutée et aucun fichier ou hash du projet n'a été transmis à un service externe.
Les outils statiques locaux `nm` et `objdump` ont été exécutés pour lire les objets.

Pour lever l'incertitude, il manque l'analyse du fichier exact correspondant au
MD5 de l'alerte et un verdict de l'éditeur antivirus. Une éventuelle soumission
du fichier détenu par l'antivirus doit être décidée explicitement par l'utilisateur,
sans rétablissement automatique dans le dossier des exécutables. Le portail
[Kaspersky OpenTIP](https://opentip.kaspersky.com/howto/) décrit les recherches
par empreinte et les possibilités d'analyse. Aucun verdict individuel n'a été
obtenu dans le présent audit.

## Reconstruction ultérieure demandée par l'utilisateur

Après l'audit statique et les travaux de fidélité, l'utilisateur a explicitement
demandé de régénérer le test spatial et de le relancer. CMake a reconstruit
`test_space_c.exe` depuis les objets disponibles et la bibliothèque C actuelle.
Le fichier initial n'a pas été restauré depuis une quarantaine.

Empreintes de la nouvelle construction :

- MD5 : `2c4fe0c7e001c650ac8a76a6c4ccf4f2`.
- SHA-256 : `7feec9ab253dc3012700d87392efac582b98391964def54ae20d9a49b0c375b7`.

`space_module_contracts` et `input_gameplay_boundaries` passent. La régression
C complète passe ensuite : **24/24, sans exclusion**. Le banc natif
`binary_input_boundaries` passe également, dont les 166 cas spatiaux comparés
au binaire original. Aucun blocage d'exécution n'a été observé lors de cette
reprise ; aucune exclusion ou protection antivirus n'a été modifiée.

Les journaux et l'empreinte sont conservés sous
`build/audit/spatial-recheck-20261016/`. Ces résultats concernent le fichier
nouvellement construit, dont le MD5 diffère de celui de l'alerte. Ils ne
constituent pas une analyse rétrospective des octets du fichier initial.
