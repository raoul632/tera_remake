# Comparaisons visuelles — 26 octobre 2026

## Nature des images

Aucune image générative utilisée. Trois preuves distinctes : captures natives
Ctrl-F5 d'une vraie session DOSBox ; pixels B800 de routines originales
exécutées sous Unicorn ; surfaces relues après rendu SDL de production.
Les secondes ne sont pas présentées comme une session DOSBox complète.
Les images et les ressources historiques restent locales dans `build/`.

## Exécution DOS observée

DOSBox 0.74-3, machine=cga, core=normal, cycles=fixed 3000, 16 Mo,
output=surface, scaler=none, frameskip=0, aspect=false, mixer nosound=true.
Ce réglage est reproductible mais ne constitue pas une mesure de PC/XT 4,77 MHz.
La démonstration change de palette ; les écrans texte suivants sont en 640x400.

Deux captures brutes dans `build/port-audit/dos-captures/` :

| Fichier | Situation réellement capturée | Usage |
|---|---|---|
| tera_000.png | Démonstration, scène de temple | Observation historique uniquement ; pas de scène SDL identique préparée |
| tera_001.png | Premier écran de création, nom vide après génération | Comparaison de cadre, titre, palette et disposition |

Les graines des mondes DOSBox et SDL ne sont pas synchronisées. Cela n'affecte
pas le titre/cadre de l'invite initiale. Les caractéristiques et scènes
aléatoires ne sont pas comparées à partir de ces deux mondes indépendants.

## Création : divergence observée puis corrigée

Le premier écran SDL provenait de `test_character_screens`, données originales,
graine explicite 1986, difficulté 0. Les surfaces sont toutes deux 640x400 ;
aucune interpolation ni correction de ratio avant la comparaison.
Les pixels du cadre supérieur montraient des lignes simples en SDL contre
une ligne double et des coins mixtes en DOS. PAS confirme D5/CD/B8.
La correction de `text.c` est testée sur 632x16 pixels RGB incluant le titre.
Avant : empreinte cd5aea25 ; après et DOS : c3f17565. Voir AUD-002.

Le reste de l'écran diffère encore : texte résiduel « création du monde.. »
sur la ligne inférieure DOS, fond hors fenêtre et curseur clignotant. Le C
recompose son écran et affiche un souligné fixe. Pas d'identité globale revendiquée.
Les écrans de points, bonus, combat, ville et dialogue disposent de captures
SDL de tests, mais pas tous d'une paire DOSBox de même état dans cet audit.
Ils restent NON TESTÉS par capture complète, même lorsque leurs primitives
ou textes ont une autre preuve BIN/PAS/DATA.

Comparaison côte à côte : `build/port-audit/visual/creation-comparison.png`.
Les fichiers source DOS/SDL sont conservés, y compris `creation-before.bmp`.
La différence totale d'écran et celle de la bande figurent dans
`visual/creation-comparison.json` ; la bande doit avoir zéro pixel différent.

## Raster graphique natif

`audit_final_raster.py` a été réellement exécuté et a produit 458 buffers
de 64 000 indices dans `build/audit/final-raster/`. Il exécute les primitives
GRAPH, les blitters et les compositions du COM/overlay ; les écritures vidéo
B800 sont lues après exécution. Chargement de données, texte BIOS, clavier et
arrêt avant règles de combat/jour sont abstraits, comme détaillé dans
spec_final_comparison_c.md. Les écritures aléatoires du monolithe sont isolées.

Les tests SDL relisent 458 scènes en 320x200 et 640x480, avec les bandes
noires et le viewport, puis comparent leurs empreintes indexées. Ils couvrent
lignes, effacements, grands dessins, sprites aux bords, paysages, monstres
fixes, façades, arcs, perspectives/portes, ruines, fosses Meduz et portes Alfol.
Cette égalité n'inclut pas le texte BIOS abstrait ni chaque combinaison de scène.

Quatre buffers SDL exportés par ce test ont également été comparés directement,
octet par octet, aux buffers B800 régénérés :

| Index | Scène | Pixels différents / 64 000 | Comparaison locale |
|---|---|---:|---|
| 277 | Paysage Amarande | 0 | visual/277-comparison.png |
| 337 | Façade de lieu | 0 | visual/337-comparison.png |
| 340 | Première configuration de couloir | 0 | visual/340-comparison.png |
| 350 | Configuration de couloir | 0 | visual/350-comparison.png |

`visual/pixel-comparison.json` contient aussi les SHA256 des deux buffers pour
chaque paire. Les PNG côte à côte appliquent une même palette CGA cyan/magenta/
blanc pour rendre les indices lisibles. Ils prouvent la composition indexée,
pas la sélection physique de palette ni la réponse d'un écran CGA.
Les PNG individuels conservent les 320x200 pixels sans agrandissement.

## Ressources, palette, proportions, animation et son

Les 82 sprites, 92 168 pixels et six pages originales passent l'aller-retour
de décodage et lecture SDL. Les palettes de scènes et polices CP437 sont
présentes et testées ; l'écran de création donne une preuve OBS supplémentaire
limitée au premier cadre. Les vecteurs de 44 cartes, 245 images de tunnels,
sons et temps XT sont relus par les tests existants ; ces autres générateurs
de références n'ont pas tous été régénérés dans cet audit.

Le monstre 15 dispose d'un test d'interruption réel dans le smoke. Les
campagnes SDL utilisent le temps virtuel et des pilotes dummy ; elles ne
confirment pas la perception du mouvement, la synchronisation sonore ni les
latences d'une partie humaine. Aucun son DOSBox audible n'a été comparé.

AUD-003 est corrigé : photo d'achat pendant 2000 ms, puis retour automatique
au catalogue, avec purge clavier originale ; test de restauration en pixels.
Différences ouvertes : AUD-004 (démo absente), réserves AUD-005/006 (reprise,
transitions et rythme), autres délais de commerce non comparés.
Une comparaison d'images par primitive ne suffit pas à lever ces réserves.
