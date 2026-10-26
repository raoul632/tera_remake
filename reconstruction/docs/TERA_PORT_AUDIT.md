# Audit de conformité du port original — 26 octobre 2026

**Verdict global : VALIDATION INSUFFISANTE.** Les preuves sont solides sur les
cas exécutés, mais ne couvrent pas encore toute l'expérience observable demandée.
Deux divergences ont été démontrées et corrigées pendant cet audit. Aucun P0/P1
ouvert n'a été démontré dans les parcours testés ; cela ne prouve pas leur absence.
L'Enhanced Edition n'est pas commencée.

**Poursuite après clarification utilisateur :** la cible est désormais
95–98 % de fidélité de l'expérience, avec priorité aux différences perceptibles
en jeu ; voir [cible et priorités](TERA_FIDELITY_TARGET.md). Le verdict ci-dessus
reste celui de l'audit strict, pas un pourcentage mesuré. AUD-003 est maintenant
corrigé : photographie d'achat de 2000 ms, restauration du catalogue et purge
des touches tamponnées, avec preuve BIN et régression SDL dédiée. Les résultats
supplémentaires sont conservés dans `fidelity-ctest.log` et `keyboard-purge.json`.
Résultat de cette poursuite : **44/44 tests SDL/oracle, aucune exclusion,
retour 0 (187,07 s)**. La version jouable habituelle
`build/sdl3/runtime-only/` a également été reconstruite ; ses trois vérifications
ciblées (horloge, commerce, autonomie) passent. Aucun calcul de gameplay modifié.

## Périmètre et provenance

Travaux exclusivement dans `reconstruction/`, conformément à AGENTS.md.
Le Python historique, les originaux, le manuel et les documents parents restent
inchangés. L'arbre `reconstruction/` était déjà entièrement non suivi par Git au
début : son existence ne représente pas un ajout intégral de cet audit. Aucun commit.

Les statuts CONFIRMÉ, PROBABLE, NON TESTÉ, DIVERGENT, MANQUANT et NON VÉRIFIABLE
portent toujours sur une portée explicite. Une réussite CTest technique ne
constitue pas, seule, une preuve historique. Voir la [matrice](TERA_DOS_COMPARISON.md).

Documents examinés : CLAUDE.md, README et mapping parents ; cadre_projet.md,
audit_fidelite.md, TERA_UNKNOWN.md, TERA_RUNTIME_DEPENDENCIES.md,
TERA_ASSET_MANIFEST.md, les spécifications de génération, combat, menus,
sauvegarde, scènes, profil XT et campagnes. Les mentions anciennes « manquant »
pour cartes/aide/polices sont dépassées par le code actuel et les tests récents.
TERA_PROJECT_RULES.md, TERA_SPEC.md, TERA_RE_STATUS.md et TERA_C_STATUS.md ne
sont pas présents sous ces noms ; leurs remplaçants sont les documents ci-dessus.

## Environnement et résultats réellement exécutés

Windows NT 10.0.26300.0, PowerShell, x86-64 UCRT64 ; GCC 13.2.0 MSYS2,
CMake/CTest 3.28.2, SDL 3.2.8. Aucun téléchargement ni installation.
Compilateur explicitement choisi : `E:/msys64/ucrt64/bin/gcc.exe` ; le premier
GCC du PATH initial était le mingw32, d'où cette sélection explicite.
Unicorn 2.1.4 et Python 3.10 servent uniquement aux preuves optionnelles.

Les logs locaux sont dans `build/port-audit/` (exclus de Git), chemins relatifs
à `reconstruction/`. Commandes reproductibles : [procédures](TERA_CONFORMANCE_TESTS.md).

| Opération | Résultat / retour processus | Preuve locale |
|---|---|---|
| Configuration core, SDL OFF, oracle OFF | 0 | core-configure.log |
| Construction core propre | 0 | core-build.log |
| CTest core | 32/32, 0 échec, aucune exclusion ; retour 0 | core-ctest.log ; core-final-ctest.log après corrections |
| Configuration SDL ON + oracle natif | 0 | sdl3-configure.log |
| Construction SDL et oracle propre | 0 | sdl3-build.log |
| CTest initial SDL/oracle | 42/42, aucune exclusion ; retour 0 | sdl3-ctest.log ; full-ctest-details.log |
| Reconstruction après corrections | 0 | frame-green-build.log |
| CTest final SDL/oracle après les deux corrections | 43/43, aucune exclusion, retour 0 ; 193,17 s | final-ctest.log ; final-ctest-details.log |
| Nouvelle régression de bordure avant correction | retour CTest 8, échec attendu ; cd5aea25 au lieu de c3f17565 | frame-red.log |
| Même régression après correction | retour 0 | frame-green.log |
| Régression graine + deux campagnes SDL + autonomie | 4/4, retour 0 | regression-ctest.log |
| Régénération de l'oracle raster natif | 458 cas, retour 0 | native-raster.log ; build/audit/final-raster/ |
| Nouvelle comparaison BIN/C horloge et mondes | 4 horaires et 12 mondes complets, aucun écart, retour 0 | original-conformance.json et .log |
| Comparaison brute de quatre scènes BIN/SDL | 0 pixel différent, retour 0 | visual/pixel-comparison.json |
| Scènes SDL après régénération des références | 1/1, retour 0 | visual-ctest.log |
| Cas d'erreur de lancement | cinq refus explicites, retour 1 chacun | initialization-errors.json |

Les 43 tests finaux passent, y compris le nouveau test de démarrage, la
régression visuelle, les deux campagnes BIN/C, les deux campagnes SDL et
le paquet autonome reconstruit avec le nouvel exécutable.

## Architecture, API et ressources

CONFIRMÉ par CMake et construction indépendante : `tera_engine` est une
bibliothèque C99 sans SDL. Les modules présents couvrent création, monde,
recrutement, exploration, navigation, combat, objets/coffres, dialogues,
succession, commerce, espace, Alfol, Meduz, Amarande, fin et sauvegarde.
Les formats de présentation sont dans `tera_resources`, eux aussi sans SDL.
`tera_sdl3` lie ces deux bibliothèques et SDL3 ; `app.c` transmet les commandes
à `tera_game_key` et les entrées spatiales à `tera_game_space_input_at`.
Les calculs de prix/dégâts/progression/RNG résident dans le core.

La présentation n'est pas intégralement confinée au frontend : le core conserve
des tirages et événements visuels historiques pour préserver l'ordre RNG.
C'est une intention historique documentée, pas une seconde règle SDL.
Les vues et événements publics sont testés en lecture seule ; les tests de
campagne comparent les états persistants, pas seulement les phases affichées.
Aucune preuve exhaustive de cohérence de toutes les combinaisons d'API n'est revendiquée.

Les chargeurs contrôlent tailles et chaînes Pascal ; les 15 ressources sont
présentes. [Manifeste existant](TERA_ASSET_MANIFEST.md), vérifié par empreintes
recalculées dans `original-hashes.json`. TERA.COM fait 50 909 octets et TERA.000
84 992 octets. Les 30 fichiers PAS/INC décompilés présents restent des sources
partielles ; les attentes du nouveau banc proviennent des instructions BIN.

Initialisation : arguments invalides, ressources absentes, sauvegarde absente
et combinaison smoke/load sont effectivement refusés. Les chemins d'erreur
de `app.c` rejoignent le nettoyage ; fichiers fermés, surfaces détruites,
textures/audio/renderer/fenêtre libérés. Les allocations et fermetures sont
contrôlées dans les chemins inspectés. NON TESTÉ : injection systématique
d'échec d'allocation, détection instrumentée de fuites et épuisement des ressources.

## Autonomie et sauvegardes

CONFIRMÉ dans le paquet neuf `build/port-audit/sdl3/runtime-only/` : lancement
avec PATH réduit à System32, sans PYTHONHOME/PYTHONPATH, sans COM, overlay,
Pascal, outils RE ni extraction. Le smoke crée le héros, passe une flèche SDL
au core, compare les états, écrit/relit une sauvegarde et capture le rendu.
L'animation interrompable du monstre 15 est également exercée par ce smoke.
Pilote vidéo dummy : ce résultat ne valide pas l'affichage physique.

Fichiers distribuables requis : `tera_sdl3.exe`, `SDL3.dll`, puis `data/` avec
NOMPERS, VALMON, VERBES, OBJETS, NOMON, MESSAGE, MESSAGE2, MESSAGE3,
CRAPIM.IMA, ASTRON.IMA, BANNIERE.IMA, FACADE.IMA, KRONO.IMA, MAISONJ.IMA,
MAISONS.IMA. `smoke.sav` et `smoke.bmp` sont des sorties, pas des prérequis.
L'inventaire exact tailles/SHA256 est `runtime-inventory.json`.
Les imports PE de l'exécutable sont SDL3, KERNEL32 et les API UCRT Windows
(`exe-imports.log`) ; aucun import Unicorn ou Python. Les DLL système de SDL
restent fournies par Windows ; le test n'est pas un inventaire récursif des DLL système.

Sauvegardes modernes : contrats CRC/version/erreurs et reprises de campagne
passent. Deux parcours SDL effectuent respectivement 23 et 19 cycles F5/F9.
Le format DOS n'est pas importé/exporté ; la substitution par un format moderne
est autorisée par cadre_projet.md. Le rendu des détails aléatoires juste après
chargement garde une réserve distincte de la conservation de l'état logique.

## Référence DOS et parcours complet

DOSBox installé : `C:/Program Files (x86)/DOSBox-0.74-3/DOSBox.exe`.
Exécution réelle de TERA.COM observée : démonstration animée, choix du lecteur,
nouveau scénario, code `ter`, difficulté 0, génération, écran de création.
Les commandes répondent ; Ctrl-F5 produit des PNG natifs. Aucune victoire
manuelle DOS n'a été effectuée. Configuration et captures : voir les
[procédures](TERA_CONFORMANCE_TESTS.md) et le [comparatif visuel](TERA_VISUAL_COMPARISON.md).
La copie de référence seule est montée dans DOSBox. Les empreintes des
17 fichiers originaux ont été relues après les essais : aucune modification.

Deux parcours générés C atteignent les quatre objectifs et la victoire sans
téléportation ni tâches prévalidées. Le même conducteur fournit les touches
aux instructions TERAMAIN/overlays et au core : 7 143 touches, 3 990 frontières,
237 613 tirages originaux, huit successions, scores Real48 8 et 10 concordants.
L'oracle reçoit une fois le monde généré C ; sa génération n'est donc pas une
preuve indépendante dans ces campagnes. Le nouveau banc de 12 mondes couvre
séparément cette lacune, dont les régénérations de la graine 23.
Les campagnes SDL traversent le code de production avec horloge accélérée,
3 988 comparaisons READY et deux victoires. Ce sont de vrais parcours automatisés,
pas des parcours humains et pas un DOSBox complet. Les abstractions sont
décrites dans validation_teramain_dos.md et dans la matrice.

## Conclusion et conditions restantes

Sous-systèmes numériques confirmés sur les fixtures et parcours exécutés,
autonomie confirmée, ressources exactes présentes, compilation/tests réussis.
Les [anomalies](TERA_PORT_ISSUES.md) distinguent les corrections de cet audit,
les défauts antérieurs corrigés et les réserves non confirmées.

Restent notamment : comparaison plus large des écrans texte et de leurs
transitions, autres temporisations du commerce, décors après reprise,
parcours humain avec délais et entrées réels, difficulté non nulle en campagne
complète, son audible et synchronisation. Les tests ne suffisent donc pas
encore pour attribuer CONFORME ou CONFORME AVEC RÉSERVES selon la mission.
