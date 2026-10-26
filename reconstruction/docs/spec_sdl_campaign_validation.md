# Validation de campagne SDL

Le conducteur C existant parcourt les douze mondes sans téléportation ni
modification de statistiques. Son origine et ses compteurs sont décrits dans
les tests de campagne et le comparatif binaire. Le même parcours doit maintenant
traverser les fonctions de production du frontend, par la file SDL : caractères,
Entrée, flèches, sauvegarde F5 et restauration F9.

À chaque entrée, comparer phase, position, monde et graine avec le conducteur
direct. À chaque frontière READY, comparer tous les octets persistants. Dessiner
chaque état et vérifier les erreurs SDL. Des sauvegardes régulières doivent
passer par le vrai fichier du frontend, puis préserver ces mêmes octets.

CONFIRMÉ après exécution : intégration C/SDL sur les parcours testés, avec
ressources DATA originales. La preuve BIN du moteur et les références de pixels
restent celles des audits existants ; cette campagne ne crée pas une nouvelle
comparaison de chaque écran avec DOS.

MODERNISABLE : pilote vidéo/audio dummy pour la régression. Les longues attentes
sont accélérées, sans changer le RNG. Les temporisations et le PCM sont couverts
séparément par sdl3_battle_media. À VÉRIFIER : appréciation humaine sur écran et
haut-parleurs réels, notamment réactivité et timbre.

## Résultats du 17 octobre 2026

| Graine | Entrées / rendus | États READY identiques | F5/F9 | Victoires en combat | Fuites | Successions | Score final |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1986 | 3886 | 2181 | 23 | 36 | 83 | 5 | 8 |
| 12345 | 3257 | 1807 | 19 | 46 | 52 | 3 | 10 |

Les deux parcours atteignent COMPLETE/victoire, les quatre objectifs et les
douze mondes. Aucun écart des octets persistants ni de la phase/graine comparées.
Le frontend collecte respectivement 6404 et 6015 événements. Tous les rendus et
les allocations de média vérifiés réussissent. Les captures finales sont dans
`build/sdl3/sdl-campaign-1986.bmp` et `sdl-campaign-12345.bmp`.

Les deux nouvelles suites réussissent, ainsi que les 37 suites existantes :
**39/39**. Journal conservé localement dans
`build/audit/sdl-campaign-validation.log`.

Le premier essai a détecté une erreur du conducteur de test : le tir spatial
était envoyé avant les purges puis comparé au résultat immédiat du moteur.
Le test utilise désormais une horloge virtuelle : touche à échéance moins
1 ms, résolution à échéance via `shot_input`, sans modifier le frontend livré.
Il ne mesure donc pas la réaction d'un joueur humain.

Reproduction, après compilation SDL :

```powershell
ctest --test-dir build/sdl3 -R sdl3_campaign -j 2 --output-on-failure
```

Le test inclut les contrôleurs statiques de `app.c` dans son propre binaire ;
il n'en réécrit ni le mapping clavier, ni le rendu, ni F5/F9. Les deux points
d'observation conditionnels du conducteur C sont absents de son exécution
ordinaire. Aucun mode de triche ou nouvelle option de jeu n'est ajouté.

## Parcours illustré

Le mode optionnel `TERA_CAMPAIGN_GALLERY` enregistre des captures BMP du
framebuffer SDL avant `SDL_RenderPresent`, avec un index CSV chronologique.
Il observe les états et la séquence de tunnel réellement émise, sans entrée
supplémentaire ni mutation de l'état du moteur. L'horloge reste accélérée.

Le parcours illustré de graine 1986 a été rejoué jusqu'à la victoire :
35 captures, mêmes 3886 entrées et digest `2d0ca4bb`, 2181 comparaisons d'état
et 23 cycles F5/F9 réussis. Les images sont des observations du remake SDL
(OBS/DATA), pas des captures d'une exécution DOS supplémentaire.

```powershell
New-Item -ItemType Directory -Force build/parcours-illustre
$env:SDL_VIDEODRIVER='dummy'
$env:SDL_RENDER_DRIVER='software'
$env:SDL_AUDIODRIVER='dummy'
$env:TERA_CAMPAIGN_GALLERY=(Join-Path $PWD 'build/parcours-illustre')
./build/sdl3/test_campaign_sdl3.exe '../data/compiled program' 1986 build/parcours-illustre/partie.sav
./tools/export_campaign_gallery.ps1 -Directory build/parcours-illustre
Remove-Item Env:TERA_CAMPAIGN_GALLERY
```

L'export PowerShell Windows facultatif convertit les BMP en PNG sans perte et
crée `index.html` (35 images entières) et `apercu.png` (12 miniatures).
Il n'est requis ni pour le jeu, ni pour les tests C. Les artefacts restent
locaux sous `build/`, avec les ressources historiques exclues de la diffusion.
