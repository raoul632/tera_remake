# Audit d'autonomie — 16 octobre 2026

Périmètre : `engine/`, sa cible `tera_engine`, puis `frontend/sdl3/`.
Les documents TERA_PROJECT_RULES.md, TERA_SPEC.md et TERA_C_STATUS.md demandés
n'existent pas dans ce dépôt. Les références applicables sont AGENTS.md,
cadre_projet.md, spec_orchestration_c.md, les spec_*.md et validation_parcours_c.md.

| Élément | Type | Nécessaire au runtime | Rôle | Peut être supprimé ? |
|---|---|---|---|---|
| TERA.COM | REFERENCE-ONLY | Non | Comparaisons historiques et RE | Oui du paquet jouable |
| TERA.000 et overlays DOS | REFERENCE-ONLY | Non | Oracle optionnel des tests BIN | Oui du paquet jouable |
| Python, frontend historique, bridges JSON | DEVELOPMENT-ONLY | Non | Anciens outils, extraction et comparaisons | Oui du paquet jouable |
| Scripts d'extraction/conversion | DEVELOPMENT-ONLY | Non | Préparation optionnelle de ressources | Oui du paquet jouable |
| DOSBox, Unicorn, BIOS, Turbo Pascal, x86 | DEVELOPMENT-ONLY | Non | Oracle historique optionnel | Oui du paquet jouable |
| ASM, Pascal décompilé, outils RE et documentation | REFERENCE-ONLY | Non | Provenance des règles | Oui du paquet jouable |
| Compilateur C99, CMake, make | BUILD-TIME | Non | Compiler le core et le frontend | Oui du paquet jouable |
| Headers et import library SDL3 >= 3.2 | BUILD-TIME | Non | Compiler uniquement le frontend | Oui du paquet jouable |
| SDL3.dll / bibliothèque SDL3 | RUNTIME | Oui pour SDL, non pour le core | Fenêtre, rendu, entrées | Non pour le frontend dynamique |
| Bibliothèques système / runtime C | RUNTIME | Oui selon plateforme | C standard, mathématiques, OS | Non selon linkage |
| NOMPERS | RUNTIME | Oui, création et paquet de présentation | 270 octets : 27 chaînes Pascal de 10 octets | Non pour le paquet actuel |
| VALMON | RUNTIME | Oui à la création | 264 octets : 22 x 6 entiers signés LE16 | Non pour une nouvelle partie |
| MESSAGE, MESSAGE2, MESSAGE3, NOMON, OBJETS, VERBES | RUNTIME | Oui depuis le bloc ressources | Textes et noms originaux | Non pour le frontend actuel |
| Les sept *.IMA, dont CRAPIM.IMA et BANNIERE.IMA | RUNTIME | Oui depuis le bloc ressources | 82 sprites et six pages ; titre au démarrage | Non pour le paquet complet actuel |
| Tables font8x8 du domaine public | BUILD-TIME | Embarquées dans l'exécutable | Police de remplacement pour les accents, original non retrouvé | Pas de fichier de police au runtime |
| Sauvegarde moderne | RUNTIME | Seulement pour reprendre | Format versionné/CRC du core, ressources logiques incluses | Oui pour créer une partie neuve |
| build/, dumps, captures, JSON intermédiaires, ressources générées | DEVELOPMENT-ONLY | Non | Preuves et sorties locales | Oui du paquet jouable |

## Résultat de l'inspection avant SDL

**CONFIRMÉ — SOURCE C** : `engine/` n'inclut que ses propres headers et
stdint/stddef/string/math/stdio/limits. Aucune inclusion SDL, appel DOS/BIOS,
exécution de processus, chargement COM, émulation ou import Python.
Les seules E/S du core sont `tera_save_read/write` sur un `FILE*` fourni par
l'hôte. Les données de jeu sont fournies aux fonctions C sous forme de structures.
Les constantes historiques (mots 16 bits, RNG Pascal, résidu de placement -211)
reproduisent des effets logiques documentés ; elles n'exigent aucun runtime DOS.

Le CMake existant peut lire TERA.COM/TERA.000 **seulement** si l'utilisateur
active explicitement TERA_UNICORN_ROOT pour les tests historiques. Ce banc
ne doit pas être activé pour distribuer le jeu. Les tests de campagne C ordinaires
lisent seulement NOMPERS et VALMON. Python n'est requis ni à la compilation
ni à l'exécution du core et du nouveau frontend.

## Contrat du chargeur autonome initial

Le chargeur de monde lit exclusivement NOMPERS et VALMON depuis le dossier explicite
`--data`. Validation des tailles exactes, longueurs Pascal <= 9, décodage
LE16 indépendant du packing. Aucune extraction au lancement. Les ressources
originales restent en lecture seule et ne sont pas ajoutées au dépôt.
Les cartes, placements, compagnons, mots et H sont ensuite produits par le core.
Les autres données graphiques/textuelles sont maintenant lues par le chargeur
de présentation séparé, avec leurs formats documentés dans TERA_ASSETS.md,
sans jamais charger le COM.

Architecture minimale : assets.c (fichiers) -> API core (génération/création),
input.c (événements SDL -> octets de commande), app.c (cycle de vie/timing),
renderer.c (vue publique en lecture seule -> diagnostic SDL 2D).
Le frontend ne calcule ni déplacement, ni RNG, ni dégâts, ni progression.
Les sauvegardes passent par save.h ; le format n'appartient pas à SDL.

## Vérification après raccordement SDL

Le test `sdl3_runtime_autonomy` passe dans un dossier contenant seulement
tera_sdl3.exe, SDL3.dll et data/ avec les 15 ressources listées dans
TERA_ASSET_MANIFEST.md, avec PATH système seul. La première preuve antérieure
à l'intégration graphique utilisait seulement NOMPERS et VALMON.
Il crée un héros, déplace via événement SDL, vérifie l'état attendu par
comparaison au core direct, sauvegarde/charge et rend une capture. Le smoke
avec fenêtre Windows passe également. Les imports PE ne contiennent que
SDL3 et les DLL système Windows/UCRT. Depuis l'intégration des ressources,
le paquet local contient les 15 fichiers de données originaux autorisés ;
les scripts de conversion et images extraites restent inutiles au lancement.
**32 suites CTest sur 32 passent**, dont les textures originales et deux
campagnes observées avec reprise. Une configuration sans SDL compile et passe
les cinq contrats de vue, présentation, sauvegarde et ressources. Le service de présentation est facultatif,
sans bibliothèque supplémentaire, absent du format de sauvegarde v1.
Ces preuves ne certifient pas encore le rendu/audio fidèles : voir
TERA_SDL3_STATUS.md et TERA_UNKNOWN.md.
