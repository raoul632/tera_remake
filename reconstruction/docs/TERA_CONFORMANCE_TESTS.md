# Scénarios de conformité et commandes — 26 octobre 2026

Commandes depuis la racine `E:/dev/PYTHON/PROJETS/tera_remake`, PowerShell.
Les sorties locales se trouvent sous `reconstruction/build/port-audit/`.
Les bibliothèques et ressources utilisées étaient déjà installées ; rien
n'est téléchargé par ces procédures. Les originaux sont lus, jamais modifiés.

## A — Compilation et tests sans SDL ni Python

```powershell
$env:PATH='E:\msys64\ucrt64\bin;'+$env:PATH
cmake -S reconstruction -B reconstruction/build/port-audit/core -G 'MinGW Makefiles' -DCMAKE_C_COMPILER=E:/msys64/ucrt64/bin/gcc.exe -DCMAKE_MAKE_PROGRAM=E:/msys64/ucrt64/bin/mingw32-make.exe -DTERA_BUILD_SDL3=OFF
cmake --build reconstruction/build/port-audit/core -j 6
ctest --test-dir reconstruction/build/port-audit/core --output-on-failure
```

Exécuté : configuration 0, build 0, CTest 0, 32/32 sans exclusion ; même
résultat après correction. Aucun interpréteur lancé. Les tests techniques
valident les contrats et vecteurs ; leur présence n'est pas une preuve DOS
supplémentaire sans provenance de l'attente.

## B — SDL et oracle natif optionnel

```powershell
cmake -S reconstruction -B reconstruction/build/port-audit/sdl3 -G 'MinGW Makefiles' -DCMAKE_C_COMPILER=E:/msys64/ucrt64/bin/gcc.exe -DCMAKE_MAKE_PROGRAM=E:/msys64/ucrt64/bin/mingw32-make.exe -DTERA_BUILD_SDL3=ON -DSDL3_DIR=E:/msys64/ucrt64/lib/cmake/SDL3 -DTERA_UNICORN_ROOT=E:/dev/PYTHON/PROJETS/tera_remake/reconstruction/build/emulator_deps/unicorn
cmake --build reconstruction/build/port-audit/sdl3 -j 6
ctest --test-dir reconstruction/build/port-audit/sdl3 --output-on-failure
```

Avant nouveaux tests : 42/42, retour 0, 179,78 s, aucune exclusion. Après les
deux corrections : 43/43, retour 0, 193,17 s, aucune exclusion ; la cible
finale ajoute `sdl3_startup_randomize`. Relevé dans `final-ctest.log`.
La configuration vérifie les SHA256 du COM/overlay. Sans
TERA_UNICORN_ROOT, aucune dépendance à ces exécutables n'est introduite.
Les campagnes et les fichiers d'origine absents peuvent provoquer SKIP/77
dans d'autres environnements : vérifier les exclusions, pas seulement « 100% ».

Scénarios comparatifs natifs réellement exécutés :

| Suite | Entrées / oracle | Vérification | Résultat |
|---|---|---|---|
| binary_input_boundaries | Monstre 15 et touches de vol avant/aux/après échéances | BIN/C, RNG et état, 70 + 166 cas | Zéro écart |
| teramain_binary_1986 | Monde généré, mêmes 3 886 touches, difficulté 0 | 2 182 frontières et six octets score Real48 | Score 8, zéro écart |
| teramain_binary_12345 | Mêmes 3 257 touches, difficulté 0 | 1 808 frontières et score | Score 10, zéro écart |
| sdl3_campaign_1986 | Événements SDL, production, temps virtuel | 2 181 READY, RNG, rendu, 23 F5/F9 | Victoire, zéro écart |
| sdl3_campaign_12345 | Même principe | 1 807 READY, 19 F5/F9 | Victoire, zéro écart |

Le conducteur planifie avec les cartes ; il ne téléporte pas, ne prévalide pas
les objectifs, ne modifie pas les statistiques ou le RNG après génération,
et reprend après les vraies successions. Les mêmes commandes suffisent aux
quatre objectifs puis à la fin ; un état préconfiguré n'est pas utilisé pour
déclarer ces campagnes terminées. Une injection initiale du monde reste
explicitement présente dans l'oracle TERAMAIN (génération séparément testée).
Voir validation_teramain_dos.md pour les hooks, champs et frontières exacts.

## C — Nouveau banc historique séparé des tests techniques

```powershell
& E:/Python/Python310/python.exe reconstruction/tests/original_conformance/compare_original.py --build-dir reconstruction/build/port-audit/sdl3 --report reconstruction/build/port-audit/original-conformance.json
```

Exécuté, retour 0. Quatre horaires connus sont retournés par l'interruption
DOS de l'oracle ; les instructions originales 0F14 construisent la graine.
Le core reçoit les mêmes quatre nombres, via le mode Z ajouté au pont C.
Attentes exclusivement BIN : 00:00:00.00, 12:34:56.78, 23:59:59.99, 01:02:03.04.

Pour chacun des seeds 0, 23, 1986 et 0xffffffff, difficultés 0/5/9 :
OVL0000 après saisies et OVL0010 sont exécutés ; tous les champs du snapshot
sont comparés au pont `tera_world_bridge` fraîchement compilé, y compris les
7 700 octets de cartes, villes, noms, équipement, caractéristiques, indices,
mots et graines. Douze cas, aucun écart ; seed 23 impose deux tentatives de
génération aux trois difficultés. Aucune logique de jeu Python n'est oracle.
L'import de WorldRoutine réutilise uniquement le conducteur d'instructions
originales. Python/Unicorn restent facultatifs pour compiler/jouer.

## D — Autonomie

`sdl3_runtime_autonomy` a créé un nouveau paquet sous le nouveau build, sans
supprimer/déplacer les originaux. Le script
`tests/sdl3_runtime_smoke.cmake` copie EXE, SDL3.dll et les 15 données listées
dans le rapport, refuse COM/overlay, réduit PATH à System32, supprime les
variables Python et lance le smoke avec création/sauvegarde/reprise/rendu.

```powershell
ctest --test-dir reconstruction/build/port-audit/sdl3 --output-on-failure -R '^sdl3_runtime_autonomy$'
```

Test exécuté dans les suites complètes et dans la régression ciblée, retour 0.
L'inventaire exact est `runtime-inventory.json`, et les imports PE sont dans
`exe-imports.log`. Les sorties smoke.sav/BMP ne sont pas des prérequis.

## E — Référence DOSBox réelle, isolée

La copie `build/port-audit/dos-data/` contient les 17 fichiers du répertoire
original compilé. Aucun fichier du dépôt original n'est monté directement.
`build/port-audit/dosbox.conf` sauvegarde toute la configuration employée :
machine cga, core normal, cycles 3000, aspect false, scaler none, frameskip 0,
nosound true. Le lancement réellement utilisé était :

```powershell
Start-Process -FilePath 'C:\Program Files (x86)\DOSBox-0.74-3\DOSBox.exe' -ArgumentList '-conf','E:\dev\PYTHON\PROJETS\tera_remake\reconstruction\build\port-audit\dosbox.conf','-noconsole' -WindowStyle Hidden
```

La fenêtre a ensuite été activée avec l'outil Windows. Observé : démonstration
animée, sortie par Escape, choix d'un lecteur, G pour nouveau scénario,
code `ter` et Entrée, difficulté 0, génération puis nom vide. Le clavier
AZERTY demandait les chiffres décalés ; une capture immédiatement après une
touche ne suffisait pas toujours à observer la transition achevée.
Ctrl-F5 enregistre `dos-captures/tera_000.png` et `tera_001.png`.
Le second capture le nom vide en mode texte 640x400.

Pas de graine DOSBox imposée : RANDOMIZE utilise l'heure DOS. Ce scénario
est rejouable dans son déroulement, pas déterministe dans ses mondes.
Pour une comparaison déterministe, employer les bancs BIN contrôlés précédents.
Pas de parcours humain DOS jusqu'à la victoire, pas de test auditif.
`original-hashes.json` conserve les SHA256 des 17 fichiers avant les essais ;
relecture après essais : zéro fichier changé.

## F — Graphiques et régression issue d'une vraie capture

```powershell
& E:/Python/Python310/python.exe reconstruction/tools/audit_final_raster.py
ctest --test-dir reconstruction/build/port-audit/sdl3 --output-on-failure -R '^sdl3_scenes_interactions$'
& E:/Python/Python310/python.exe reconstruction/tools/render_port_audit_evidence.py --native-dir reconstruction/build/audit/final-raster --sdl-dir reconstruction/build/port-audit/sdl3 --output-dir reconstruction/build/port-audit/visual --dos-creation reconstruction/build/port-audit/dos-captures/tera_001.png
```

Exécution réelle : 458 rasters originaux régénérés ; scripts d'export Python
facultatifs avec Pillow. L'interpréteur est explicite : préfixer PATH avec
UCRT64 sélectionne sinon un autre Python sans Pillow (échec constaté d'un
premier export, corrigé sans installation). **Après régénération d'un header de vecteurs,
recompiler le test SDL avant CTest.** Cet audit l'a fait. Quatre buffers
SDL indexés sont comparés directement, 0 pixel différent par paire. Les
images côte à côte ne remplacent pas les buffers conservés.

`test_character_screens` possède aussi une assertion tirée de la capture DOS
réelle (SHA256 dans le code) sur 632x16 pixels RGB de titre/bordure. Avant
correction le test échoue (CTest 8), après il passe (0). Cette région exclut
le curseur clignotant ; le reste de l'écran n'est pas artificiellement aligné.
Résultats dans frame-red.log, frame-green.log et visual/creation-comparison.json.

## G — Erreurs et réserves

Exécutés avec vidéo dummy : difficulté 10, seed 4294967296, répertoire de
données absent, sauvegarde absente, smoke + load. Chaque appel retourne 1
avec message explicite ; `initialization-errors.json` conserve arguments,
sorties et codes. Ces refus ne sont pas des comportements historiques certifiés.

Scénarios restant à exécuter : parcours humains DOS/SDL avec saisies et délais
réels, mesure DOSBox synchronisée de l'achat, reprise DOS/SDL sur décor aléatoire,
capture de chaque écran de création/combat/commerce/fin à même état, sons
audibles, campagnes complètes à difficulté non nulle et davantage de graines.
Leur statut est NON TESTÉ ; aucun résultat n'est simulé.

## H — Poursuite vers 95–98 % : photographie d'achat et clavier

```powershell
& E:/Python/Python310/python.exe reconstruction/tests/original_conformance/compare_keyboard_purge.py
cmake --build reconstruction/build/port-audit/sdl3 -j 6
ctest --test-dir reconstruction/build/port-audit/sdl3 --output-on-failure -R '^sdl3_shop_presentation$'
```

Exécuté, retours 0. Le premier banc exécute C4210 du COM, avec son empreinte
vérifiée, et confirme la purge de 0/1/3/15 touches anciennes avant l'attente
d'une nouvelle. L'E/S clavier et les contrôles de runtime sont abstraits.
Ce banc ne mesure pas le délai DOSBox : DELAY(2000) est établi par OVL00BC.

Le test SDL exerce une vraie acquisition via les commandes du core dans une
fixture synthétique, puis vérifie la photo à 1999 ms, la restauration à 2000 ms,
la purge de KEY_DOWN/TEXT_INPUT, l'absence de modification de la session pendant
l'attente, un nouvel achat et la fermeture de fenêtre. Le rectangle restauré
est comparé en pixels au catalogue avant achat. Les trois captures BMP restent
dans le build ; `shop-test.log` et `keyboard-purge.log/json` conservent les preuves.
La suite complète mise à jour est enregistrée dans `fidelity-ctest.log`.
Résultat : 44/44, aucune exclusion, retour 0, 187,07 s. Les deux campagnes
SDL et les deux campagnes comparées au binaire passent toujours. Le build
jouable habituel a ensuite été reconstruit (`playable-build.log`) et les trois
vérifications horloge/commerce/autonomie passent (`playable-check.log`, 3/3).
