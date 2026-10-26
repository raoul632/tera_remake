# Ressources originales — intégration C / SDL3

16 octobre 2026. Ce bloc ajoute les chargeurs de ressources et leur raccordement
à la présentation. Il ne certifie pas encore les compositions de scènes,
les panneaux de personnages ou les menus complets du jeu original.

## Sources et niveau de preuve

**CONFIRMÉ — PAS/DATA** : `data/source code/OVL_00DB.PAS`, lecture de VALMON,
OBJETS, VERBES, MESSAGE/2/3, NOMPERS, NOMON et CRAPIM ; déclarations de
`TERA.PAS`. Le chargeur consomme exactement les 23 206 octets et les 82
enregistrements de CRAPIM. Les résultats Python antérieurs ne sont pas la
spécification du nouveau chargeur.

**CONFIRMÉ — PAS/DATA** : `TERAMAIN.PAS / C_DA21` charge la bannière et copie
0x4000 octets en mémoire vidéo. Les six pages IMA sont restituées avec
l'adressage CGA : octets 0..7999 pour les lignes paires, 8192..16191 pour
les impaires, quatre pixels 2 bits par octet, poids fort d'abord. Les zones
8000..8191 et 16192..16383 sont du padding, pas des lignes d'image.

**PROBABLE — DATA/inspection visuelle** : cette même interprétation des cinq
autres pages produit des images cohérentes : cockpit, façade, Kronopolis,
maison avec jardin et maison sans jardin. L'affectation exacte de chaque
image à chaque écran et sa palette ne sont pas toutes établies. Les notes
historiques qui affirmaient l'absence de tout chargeur de bannière étaient
incomplètes : C_DA21 constitue une source directe pour cette page.

**CONFIRMÉ — BIN capturé précédemment** : `build/audit/amarande_response.asm`
(C4473, 4473..4500) sélectionne MESSAGE, MESSAGE2 ou MESSAGE3 à partir du
RANDOM(3) du core. `amarande_resident.asm` / CAFE8 établit la correspondance
commandes/verbes, dont L/M/O/P -> 8/9/11/12. Les contrats combat et dialogues
confirment les IDs 26..33 et 22/23. Ces fichiers sont des preuves de
développement ; aucun ASM, Pascal, COM ou overlay n'est lu par le jeu.

## Formats, conversion et usage

| Source | Format | Représentation C résultante | Usage actuel |
|---|---|---|---|
| NOMPERS | 27 x 10 octets, longueur Pascal <= 9 | CP437 terminé ; base + deux variantes | Noms du monde/création via core ; table complète chargée |
| VALMON | 22 x 6 LE16 signés, 264 octets | Profils du core | Nouvelle partie/combats, chargeur existant conservé |
| VERBES | 56 x 19, longueur <= 18 | 56 chaînes CP437 | Table de libellés disponible pour les menus suivants |
| OBJETS | 25 x 16, longueur <= 15 | 25 chaînes CP437 | Noms d'objets dans les événements d'inventaire, achat et équipement |
| NOMON | 22 x 40, longueur <= 39 | 22 chaînes CP437 | Nom original du monstre actif |
| MESSAGE | 102 x 40 | 102 chaînes CP437 | Phrases choisies par le core et messages fixes |
| MESSAGE2 / MESSAGE3 | 33 x 40 chacun | Deux tables CP437 | Variantes 1/2 déjà choisies dans le core |
| CRAPIM.IMA | 82 enregistrements largeur/hauteur + paires de plans | 82 descripteurs et 92 168 pixels indexés | Textures SDL chargées, testées et disponibles ; composition des scènes à suivre |
| ASTRON/BANNIERE/FACADE/KRONO/MAISONJ/MAISONS.IMA | 16 384 octets chacun | Six images indexées 320 x 200 | Six textures ; BANNIERE au démarrage |

Aucune conversion préalable, PNG, JSON, script Python ou extraction du COM
n'est nécessaire. Les représentations modernes sont produites **en mémoire**.
Les captures BMP de vérification restent sous `build/`, exclu de Git ; les
originaux et leurs dérivés ne sont pas redistribués.

### CRAPIM et unités

Le loader suit les IF successifs de OVL_00DB : seuils 0x32, 0x3B, 0x4A,
0x4C et exceptions 0x51/0x52. Les facteurs déterminent la quantité de données
stockée : ne pas agrandir une seconde fois les pixels décodés. Les deux
plans sont entrelacés **octet par octet**, contrairement aux pages complètes.
Les lignes impaires du premier groupe suivent la troncature Pascal documentée,
sans ajout d'une ligne noire artificielle. Les offsets et dimensions brutes
restent dans les descripteurs, à côté des dimensions décodées en pixels.
Les coordonnées de placement X en octets vidéo devront être converties à la
primitive concernée lors du bloc scènes, pas universellement dans le loader.

### Couleurs, transparence et police

Les pixels originaux restent indexés 0..3 sans perte. L'affichage initial
utilise noir, cyan clair, magenta clair, blanc, une interprétation CGA
cohérente avec la bannière et les captures. **À VÉRIFIER** : correspondance
exacte PALETTE(0..3) et changements de palette selon scène.
**CONFIRMÉ — BIN C5051/C5E04/C60BB** : PutSprite, PutSprite_off et
Off2Screen_skip0 ignorent les octets CGA nuls. Chaque octet couvre quatre
pixels : un pixel noir d'un octet mixte reste opaque. Ce n'est ni un OR ni
une transparence de tous les pixels noirs. Les textures brutes d'inspection
restent opaques ; les textures de scène portent un alpha binaire par groupe
de quatre pixels, testé sur les 82 sprites. Les pages IMA restent opaques.

Un tampon SDL séparé reproduit les compositions hors écran d'Amarande,
de la Pyramide et des temples. Les remplissages noirs y effacent le tampon,
puis la superposition conserve le fond sous les groupes entièrement nuls.
Le sol est composé à son emplacement dans les séquences PAS, entre les
groupes fixes, avant la superposition sur le ciel ou les marches. PutSprite
aligne y sur une paire de lignes et ne dessine pas au-delà de y=191,
conformément aux tests y/2 et <96 du BIN. Pas de filtre ni d'interpolation.

Il n'existe pas de fichier de police séparé dans les ressources présentes.
**Police de remplacement explicitement déclarée** : tables 8x8 font8x8 dans
le domaine public, embarquées au code, attribution et URLs dans
`frontend/sdl3/vendor/font8x8/NOTICE.md`. Ce n'est pas une extraction de la
police TERA. La conversion CP437 -> Unicode/UTF-8 est complète ; le dessin
couvre ASCII et Latin-1 (tous les caractères effectivement présents dans les
sept tables locales, vérifiés par le test). Un caractère hors de ce jeu de
glyphes serait dessiné `?`, sans altérer la chaîne ni sa conversion UTF-8.
La police originale reste **MANQUANTE**, à traiter lors de la fidélité écran.

## Contrat des chargeurs et des références

`resources.c` / `encoding.c` constituent `tera_resources`, bibliothèque C99
sans SDL, indépendante des règles du core. Le core ne dépend pas de cette
bibliothèque. Les textes conservent leur numérotation originale à partir de 1.
Les accès invalides retournent NULL. Tailles exactes, longueurs, limites des
plans, dimensions et consommation complète sont vérifiées avant publication.
Les décodeurs ne modifient pas la sortie en cas de rejet. Le chargement du
paquet est atomique : toute erreur libère la nouvelle allocation.
Les zéros de padding ne sont pas affichés ; un NUL à l'intérieur d'une chaîne
Pascal déclarée non vide est rejeté pour éviter une troncature silencieuse.

`tera_resource_message` résout uniquement des sélecteurs produits par le core :
variants MESSAGE0/1/2, indices générés du monde, objets et phrases de combat.
Il ne tire aucun nombre, ne valide aucune action et ne calcule aucun résultat.
Les sélecteurs COMMAND_RESPONSE sont résolus par la table CAFE8 :
A/B/C/D/E/F/H/L/M/O/P/S -> 1/2/3/4/5/6/7/8/9/11/12/14.
Une valeur NO_VARIANT utilise MESSAGE ; une variante invalide est refusée.
Les inscriptions Seth 100/101 sont des textes résidents, absents des fichiers
externes : référence non résolue affichée explicitement, aucun texte fabriqué.

## Raccordement et lancement

Le frontend charge le paquet, crée les 88 textures, puis affiche BANNIERE
avant la création du héros. La touche qui ferme le titre est consommée par
la présentation : elle n'entre pas dans le nom. Le test vérifie que cette
étape ne modifie aucun octet de la session ni le RNG.
Les noms du core sont rendus en CP437, le combat affiche NOMON, et les deux
dernières références textuelles reçues sont résolues après la commande.
Les messages techniques restent visibles sur leur ligne propre. Le chargement
d'une partie efface les anciennes références pour ne pas lire les indices
d'un autre monde. La disposition reste diagnostique, pas un écran original
complet reconstitué. Aucun sprite de monstre n'est choisi par ressemblance.

Le dossier `--data` doit maintenant contenir les **15 fichiers** du tableau,
sans TERA.COM ni TERA.000. Les ressources de présentation sont aussi nécessaires
lors d'un `--load` ; le monde et les profils restent ceux de la sauvegarde,
aucune régénération n'est effectuée. La commande existante reste :

```powershell
./reconstruction/build/sdl3/tera_sdl3.exe --data './data/compiled program'
```

Pour un dossier de jeu local autonome, le test prépare
`reconstruction/build/sdl3/runtime-only/` : exe, SDL3.dll, data contenant
uniquement les 15 ressources autorisées. Pas d'outil ni de script au lancement.

## Validation exécutée et limites

**32 suites CTest sur 32 réussies**, aucune exclue, GCC C99 avec avertissements
traités comme erreurs. Les campagnes du core restent inchangées.

- `resource_decoder_contracts` : chaînes malformées, tailles tronquées ou
  surnuméraires, CP437/UTF-8, sorties inchangées au rejet, parité/padding CGA,
  plans entrelacés, troncature et seuils de sprites sur fixtures synthétiques.
- `original_resource_formats` : sept tables réelles, 82 enregistrements,
  consommation exacte de l'atlas, dimensions/offsets jalons dont le dernier
  offset 20324, 92 168 pixels, six pages,
  variantes/IDs invalides, références de messages/indices/combat.
- `sdl3_original_asset_pixels` : les 82 sprites et six pages sont rendus
  puis relus par SDL, comparaison **de chaque pixel RGBA** avec l'image
  indexée. Vérification des caractères réellement présents et captures
  inspectées visuellement : `original-title.bmp`, `original-sprites.bmp`,
  `original-backgrounds.bmp`, `original-texts.bmp` sous build/sdl3.
- `sdl3_runtime_autonomy` : pilote dummy/software, PATH système seul,
  titre original affiché/fermé sans toucher au jeu, création et déplacement
  via SDL, sauvegarde/reprise, puis phrase originale choisie par le core.
  L'état sérialisé et le RNG restent identiques à la même suite C directe.
- Configuration sans SDL : les cinq contrats sélectionnés de vue,
  présentation, sauvegarde, décodeurs et fichiers originaux passent.

La comparaison pixel SDL valide le transfert **décodeur -> texture -> rendu** ;
elle ne prouve pas à elle seule les couleurs ou compositions de tous les
écrans DOS. Les formats s'appuient sur PAS/DATA ; les captures permettent de
détecter des incohérences visuelles. Les compositions des lieux et interactions
sont désormais décrites dans [le bloc scènes](spec_scene_interactions_c.md).
Les détails graphiques encore absents, la police originale, les inscriptions
résidentes, le combat terrestre et l'audio restent à compléter.
