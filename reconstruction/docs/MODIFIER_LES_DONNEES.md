# Modifier les données de TERA Original

Les données s'éditent dans les sources de développement, puis il faut
régénérer et recompiler le jeu. Modifier un fichier à côté de l'exécutable
distribué ne change pas ses ressources intégrées.

## 1. Ouvrir les ressources

Le travail Original se trouve ici, séparé d'Enhanced :

```text
E:\dev\PYTHON\PROJETS\tera_remake\reconstruction\build\original-embedded
```

Dans ce worktree, ouvrir `reconstruction/assets/original/editable/`.
Faire une copie de ce dossier avant les éditions : il est ignoré par Git,
donc Git ne sauvegarde pas vos modifications de ressources.

| Fichier | Contenu à modifier |
|---|---|
| `OBJETS.json` | Noms des objets |
| `NOMON.json` | Noms des monstres |
| `NOMPERS.json` | Noms des personnages et variantes |
| `VERBES.json` | Libellés des verbes |
| `MESSAGE.json`, `MESSAGE2.json`, `MESSAGE3.json` | Messages et variantes |
| `VALMON.json` | Profils numériques des monstres |
| `sprites/001.png` à `sprites/082.png` | Images des sprites |
| `ASTRON.png`, `BANNIERE.png`, `FACADE.png`, `KRONO.png`, `MAISONJ.png`, `MAISONS.png` | Six images de fond |

## 2. Modifier un texte ou une valeur

Ouvrir le JSON dans un éditeur de texte et l'enregistrer en **UTF-8**.
Dans un enregistrement, changer uniquement le champ `text`, par exemple :

```json
"text": "cristal"
```

Conserver les `id`, leur ordre, le nombre d'enregistrements, `stride` et
`unused_tail`. La génération s'occupe de la longueur et de la fin inutilisée.
Utiliser des caractères représentables en CP437 : les accents français usuels
conviennent, les emoji et les guillemets typographiques ne conviennent pas.

Longueur maximale par texte : **15** caractères pour OBJETS, **9** pour
NOMPERS, **18** pour VERBES, **39** pour NOMON et les trois tables MESSAGE.
Une phrase plus courte reste préférable si sa place à l'écran est limitée.

Dans `VALMON.json`, changer les nombres du tableau `values` de l'enregistrement
voulu. Conserver exactement **six entiers par monstre**, entre -32768 et 32767,
et les 22 enregistrements. Les colonnes sont volontairement numérotées : ce
guide ne leur attribue pas de signification non vérifiée. Modifier ces valeurs
change les données de combat et peut donc changer l'équilibrage.

## 3. Modifier une image

Ouvrir le PNG dans un éditeur capable de conserver son **mode indexé**.
Garder les dimensions et les quatre indices de palette dans cet ordre :
0 noir, 1 cyan clair, 2 magenta clair, 3 blanc.

Ne pas convertir en RGB/RGBA, ajouter de transparence PNG, redimensionner,
réordonner la palette ou renommer les fichiers. Les fonds font 320 × 200 ;
chaque sprite garde sa propre taille. Ne pas modifier les JSON de dimensions.

Pour les sprites, la transparence dépend de groupes horizontaux de **quatre
pixels** : un groupe entièrement noir est ignoré ; dans un groupe contenant
une autre couleur, les pixels noirs sont opaques. Il n'y a pas de canal alpha
à peindre. Le dossier `masks/` sert à inspecter les masques de référence ;
modifier ses images n'a aucun effet sur le jeu. Le rendu recalcule le masque
effectif depuis les indices du sprite.

## 4. Régénérer, compiler et essayer

Ouvrir PowerShell à la racine du worktree indiqué plus haut :

```powershell
Set-Location 'E:\dev\PYTHON\PROJETS\tera_remake\reconstruction\build\original-embedded'

python reconstruction/tools/original_resources.py generate --editable reconstruction/assets/original/editable --output reconstruction/assets/original/generated/embedded.c
if ($LASTEXITCODE -ne 0) { throw 'Corriger les ressources avant de compiler.' }

cmake --build reconstruction/build/embedded-win64 --target tera_sdl3 -j 8
if ($LASTEXITCODE -ne 0) { throw 'La compilation a échoué.' }

./reconstruction/build/embedded-win64/tera_sdl3.exe --save ./reconstruction/build/essai-donnees.sav
```

Ces commandes utilisent le build Windows déjà configuré sur cette machine.
La génération nécessite Python avec Pillow ; le jeu compilé n'en a pas besoin.
Ne pas éditer `generated/embedded.c` à la main : il sera remplacé à la prochaine
génération. En cas d'erreur, lire le message du générateur : longueur de texte,
encodage, dimensions ou palette sont les premières choses à contrôler.

Créer une **nouvelle partie** pour vérifier les nouveaux noms du monde et
profils numériques : une ancienne sauvegarde conserve ses données logiques.
Le chemin de sauvegarde d'essai ci-dessus évite d'utiliser `tera.sav`.
Lancer sans `--data`, sinon le jeu utilisera les fichiers externes indiqués.

La commande `verify` compare aux originaux : elle doit signaler une différence
après une édition volontaire. Les tests de fidélité et campagnes peuvent aussi
échouer si les textes ou profils changent ; ne pas modifier leurs attentes
pour masquer ces écarts. Examiner visuellement le résultat dans le jeu.

Ne pas relancer l'extraction pour appliquer une édition. Ne pas toucher à
`data/compiled program/` ni modifier les empreintes des sources originales.
Pour revenir à l'état initial, restaurer votre copie du dossier `editable/`,
puis régénérer et recompiler.

Pour une autre machine, la configuration CMake et le détail du pipeline sont
décrits dans [TERA_EMBEDDED_RESOURCES.md](TERA_EMBEDDED_RESOURCES.md).
Ce guide concerne Original ; la chaîne doit être adaptée séparément pour Enhanced.
