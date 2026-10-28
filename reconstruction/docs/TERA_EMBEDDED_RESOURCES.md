# Ressources Original intégrées — 28 octobre 2026

Chaîne locale : DATA → PNG/JSON éditables → C généré → compilation → exécutable.
Les droits et la provenance TERA / Loriciels 1986 restent inchangés. Aucune
publication automatique. Les ressources converties et générées sont exclues
de Git par `reconstruction/.gitignore` ; elles sont conservées localement.
Un clone du code seul doit recevoir ces ressources locales avant de construire
le frontend. Le core reste indépendant de SDL.

## Extraction reproductible

Depuis la racine du worktree Original, avec Python et Pillow disponibles :

```powershell
python reconstruction/tools/original_resources.py extract --source 'E:/dev/PYTHON/PROJETS/tera_remake/data/compiled program' --editable reconstruction/assets/original/editable
python reconstruction/tools/original_resources.py verify --source 'E:/dev/PYTHON/PROJETS/tera_remake/data/compiled program' --editable reconstruction/assets/original/editable
python reconstruction/tools/original_resources.py generate --editable reconstruction/assets/original/editable --output reconstruction/assets/original/generated/embedded.c
python reconstruction/tests/test_original_pipeline.py reconstruction/assets/original/editable
```

L'extraction refuse un dossier existant pour préserver les éditions. Seule
cette commande lit les quinze originaux. `verify` est un oracle optionnel qui
les relit. `generate` ne consulte que les PNG/JSON. Aucun COM, overlay ou Pascal
n'intervient dans cette chaîne. La génération est déterministe, sans horodatage
ni chemin absolu incorporé dans le C.

## Édition

- Sept JSON : enregistrements numérotés depuis 1, texte Unicode réencodable en
  CP437, taille fixe d'origine et octets de fin inutilisés explicités. Le texte
  est la valeur éditable, pas une copie hexadécimale. Un texte trop long,
  non CP437 ou contenant NUL est rejeté. Après changement de longueur, la fin
  inutilisée est tronquée/complétée sans changer la taille d'enregistrement.
- `VALMON.json` : 22 profils, chacun avec six entiers signés 16 bits. Les
  colonnes restent numérotées : aucun sens de statistique n'est inventé.
- Six PNG de fonds indexés 320 × 200, opaques. Le JSON associé conserve les
  deux fins de banque CGA de 192 octets, invisibles mais nécessaires à la
  reconstruction exacte des fichiers de référence.
- `sprites/001.png` à `082.png` : pixels indexés 0..3, ordre et dimensions
  documentés dans `sprites.json`, avec dimensions brutes, facteurs de stockage
  et offsets d'origine. Les variantes agrandies sont déjà stockées ; aucun
  rééchantillonnage. La troncature des hauteurs impaires suit le décodeur C.
- `masks/` : masques consultables dérivés, sans autorité sur le rendu. Un groupe
  de quatre indices tous nuls est transparent ; les noirs d'un groupe mixte
  restent opaques. Après édition d'un sprite le masque effectif est recalculé
  par le renderer C à partir des indices. Les images de masque d'extraction
  documentent la référence originale et ne sont pas des entrées de génération.

Conserver le mode PNG indexé et les quatre entrées de palette : noir,
cyan clair, magenta clair, blanc. Les conversions RGB/RGBA et la transparence
PNG sont refusées pour éviter une réinterprétation silencieuse. La palette PNG
sert à l'inspection ; les changements de palette en jeu restent ceux du code C.
Les tailles de sprites et l'organisation du manifeste sont fixes pour ce port.

## Compilation

Avec `embedded.c` déjà généré, le build ordinaire ne cherche ni Python ni DATA :

```powershell
cmake -S reconstruction -B reconstruction/build/core -DTERA_BUILD_SDL3=OFF
cmake --build reconstruction/build/core
cmake -S reconstruction -B reconstruction/build/sdl -DTERA_BUILD_SDL3=ON -DSDL3_DIR=E:/msys64/ucrt64/lib/cmake/SDL3
cmake --build reconstruction/build/sdl
```

Pour modifier les PNG/JSON et les régénérer automatiquement à la compilation,
configurer `-DTERA_REGENERATE_ASSETS=ON` et éventuellement
`-DPython3_EXECUTABLE=...`. Cette option requiert Python/Pillow uniquement sur
la machine de développement. Elle suit les PNG/JSON et le script dans les
dépendances CMake. Sans cette option, relancer `generate` après toute édition.

`tera_resources_load(NULL, ...)` et `tera_assets_world(..., NULL, ...)` lisent
les tableaux constants compilés via les décodeurs C existants. `--data DIR`
reste une surcharge externe explicite et stricte, sans repli caché. Aucun
fichier n'est extrait au lancement, aucune recherche dans le dépôt, le profil
utilisateur, une ancienne installation ou le réseau. Le core et le format de
sauvegarde ne changent pas.

## Packaging et preuves

`tools/package_release.ps1` produit une nouvelle archive sans écraser les
anciennes. Contenu autorisé : exe x64, SDL3.dll, documentation et notices,
avec empreintes des fichiers et manifeste de provenance des DATA intégrés.
Aucun dossier data, script, ressource de développement ou original séparé.
Les sauvegardes existantes ne sont pas touchées.

Le compte rendu `TERA_EMBEDDED_VALIDATION.md` distingue les vérifications
effectuées et les limites. Les comparaisons avec la référence et les tests de
campagne conservent leurs résultats attendus ; aucun digest n'est ajusté.
