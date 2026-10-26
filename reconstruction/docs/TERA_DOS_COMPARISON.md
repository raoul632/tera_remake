# Matrice DOS ↔ Core C / SDL3 — 26 octobre 2026

La colonne DOS distingue instructions originales exécutées (BIN) et observations
DOSBox (OBS). « CONFIRMÉ » n'est jamais étendu au-delà des cas nommés.
Les résultats réels détaillés sont sous `build/port-audit/` ; anciens documents
et Python seuls ne fournissent aucun nouveau résultat confirmé.

Preuves : E1 = full-ctest-details.log ; E2 = original-conformance.json ;
E3 = native-raster.log et tests SDL de scènes ; E4 = visual/pixel-comparison.json ;
E5 = captures DOSBox et test de création ; E6 = code source inspecté ;
E7 = initialization-errors.json ; E8 = core-final-ctest.log.
Les scénarios sont décrits dans [TERA_CONFORMANCE_TESTS.md](TERA_CONFORMANCE_TESTS.md).

| ID | Fonctionnalité | DOS attendu / réellement établi | C/SDL3 réellement vérifié | Résultat et portée | Preuve / gravité |
|---|---|---|---|---|---|
| A01 | Modules et architecture | Sans objet historique | Core indépendant, SDL lie le core | CONFIRMÉ technique | CMake, deux builds propres |
| A02 | Autonomie | COM référence seulement | Paquet neuf, PATH système, 15 données | CONFIRMÉ technique | E1, runtime-inventory.json |
| A03 | Initialisation en erreur | Compatibilité DOS non comparée | Cinq refus contrôlés, aucun crash observé | CONFIRMÉ technique ; historique NON TESTÉ | E7 |
| C01 | Dix personnages, caractéristiques, équipements | OVL0010 et état original exécutés | Douze mondes complets, roster inclus | CONFIRMÉ sur graines/difficultés E2 | E2 |
| C02 | Nom, sexe, points, bonus, validation | OVL0022 réel, mêmes touches pendant deux campagnes | Création et recréations concordantes | CONFIRMÉ pour ces chemins ; tous les refus NON TESTÉS dans ce nouvel audit | E1, character_creation_contracts |
| C03 | Écrans de création | OBS écran de nom, cadre D5/CD/B8 | Bande 632x16 comparée à la capture réelle | CONFIRMÉ bande après correction ; autres écrans NON TESTÉS contre captures | E5 ; P3 corrigé |
| C04 | Historique/scroll des répartitions | Séquence PAS, pas de nouvelle capture correspondante | Écrans synthétisent les valeurs courantes | PROBABLE divergence ; portée visuelle à comparer | spec_menus_sdl_c.md, E6 ; P2 potentiel |
| W01 | Dimensions et sept colonnes des cartes | 7 700 octets DOS produits par OVL0000 | Tous ces octets comparés sur 12 mondes | CONFIRMÉ fixtures | E2 |
| W02 | Murs, portes, secrets, escaliers | Génération native, même état RNG initial | Égalité complète des cartes | CONFIRMÉ fixtures, pas une conclusion statistique universelle | E2 |
| W03 | Monstres, coffres, pièges, clés | Population native ; champs complets | Douze populations identiques | CONFIRMÉ fixtures | E2 |
| W04 | Villes, PNJ, montagnes, indices, mots et H | OVL0010 réel | Placements/textes/roster comparés | CONFIRMÉ fixtures | E2 |
| W05 | Rejet et accessibilité | Le générateur original rejette la première carte, graine 23 | Deux essais, même monde final | CONFIRMÉ pour ce rejet ; preuve d'accessibilité universelle NON TESTÉE | E2 |
| R01 | Algorithme et ordre RNG | BIN ; 237 613 vrais tirages pendant campagnes | États RNG concordants aux frontières | CONFIRMÉ dans les cas exécutés | E1, E2 |
| R02 | Graine de nouvelle partie | RANDOMIZE après code scénario, BIN 0F14, quatre horaires exécutés | Avant : 1986 fixe ; après : horloge locale DOS | DIVERGENT avant, CONFIRMÉ après sur format et tests de l'adaptateur | E2 ; P1 corrigé |
| R03 | Distributions aléatoires | Biais modulo original, pas de campagne statistique | Formule identique et égalités déterministes | CONFIRMÉ formule/cas ; distributions empiriques NON TESTÉES | random.c, E1/E2 |
| M01 | Commandes, orientations, obstacles, portes | TERAMAIN et overlays, états DOS réels | Position, orientation, cartes et état concordants | CONFIRMÉ deux parcours | E1 |
| M02 | Limites des cartes | Quelques limites exécutées par contrats C | Tests techniques et parcours | NON TESTÉ exhaustivement contre DOS aux quatre bords de chaque monde | E8 |
| B01 | Initiative et choix des actions | Routines résidentes exécutées en campagne | Combat, réactions, tours et menus testés | CONFIRMÉ chemins de deux campagnes ; pas chaque combinaison | E1 et combat_* |
| B02 | Jets, seuils, résistances, équipement, dégâts | Calculs BIN, états HP/MP comparés | Égalité aux frontières stables | CONFIRMÉ cas exécutés ; tests source seuls n'étendent pas la portée | E1 |
| B03 | Ripostes, monstres, états, mort, succession | Huit successions DOS réelles | Mêmes survivants, états, ressources | CONFIRMÉ deux campagnes | E1 |
| B04 | Fuite et récompenses | Vrais calculs DOS, carte et argent comparés | 135 fuites, 82 victoires sur deux campagnes | CONFIRMÉ deux campagnes | E1 |
| B05 | Monstre 15 et tir spatial | 70 cas monstre, 166 cas horodatés BIN/C | Zéro écart ; smoke SDL interruption | CONFIRMÉ fixtures ; jouabilité humaine NON TESTÉE | E1 |
| I01 | Acquisition, capacité, utilisation, consommation | Routines BIN atteintes pendant parcours | États équipements et limites comparés | CONFIRMÉ chemins atteints ; inventaire exhaustif NON TESTÉ | E1, inventory_chest_contracts |
| I02 | Commerce/soins, prix, vente | Overlay 00BC exécuté ; prix et argent comparés | Achats/vente/hôpital et menus techniques | CONFIRMÉ chemins atteints | E1 |
| I03 | Photographie après achat | PAS C4EFE puis DELAY(2000), BIN C4210 purge avant lecture | Photo 2000 ms puis catalogue restauré ; touches anciennes purgées | CONFIRMÉ cycle sur test à temps contrôlé ; latence physique NON TESTÉE | sdl3_shop_presentation, keyboard-purge.json ; P2 corrigé |
| P01 | PNJ, dialogues, recrutement | Routines réelles et état comparé | Branches de campagne + contrats | CONFIRMÉ chemins exécutés ; toutes combinaisons NON TESTÉES | E1 |
| P02 | Événements et objets spéciaux | Objectifs réels sans injection après départ | Quatre tâches et compteurs concordants | CONFIRMÉ deux parcours | E1 |
| P03 | Victoire, défaite, score | Fin BIN, six octets Real48 comparés | Scores 8/10, fins et successions | CONFIRMÉ deux parcours ; division nulle hors parcours technique seulement | E1, ending_module_contracts |
| S01 | Sauvegarde moderne | Conteneur DOS abstrait dans l'oracle | Contrats CRC, 42 reprises SDL, état identique | CONFIRMÉ technique ; compatibilité disque DOS NON TESTÉE et non implémentée | E1/E8 ; substitution autorisée |
| S02 | Apparence juste après reprise | Redessin DOS après reprise non observé ici | Scenery remis à zéro dans load_game | PROBABLE divergence historique ; perte du cache SDL établie | E6, TERA_UNKNOWN.md ; P2 à comparer |
| V01 | Sprites et six fonds | DATA originaux, décodage sans perte | 82 sprites, 92 168 pixels, six pages | CONFIRMÉ données/aller-retour ; composition non implicite | E1 |
| V02 | Paysages, portes, couloirs, silhouettes | 458 rasters B800 originaux régénérés | Tests 320x200 et 640x480 ; quatre buffers comparés directement | CONFIRMÉ fixtures, textes abstraits dans cet oracle | E3/E4 |
| V03 | Cartes, tunnels, palette | Fixtures d'oracles natifs existantes | Tests C/SDL rejoués | CONFIRMÉ tests contre fixtures ; provenance de tous ces anciens vecteurs non régénérée ici | E1, spec_visual_xt_c.md |
| V04 | Polices et mode texte | Capture DOSBox CGA et tables CP437 | Bande de création identique après correction | CONFIRMÉ bande ; tous écrans texte NON TESTÉS | E5 |
| V05 | Démo et choix du scénario | OBS démonstration, choix lecteur/code/difficulté | Bannière puis création ; difficulté en CLI, code scénario absent | MANQUANT : démo et code scénario en SDL ; adaptation lecteur/sauvegarde autorisée | OBS/E6 ; P2 |
| V06 | Son et délais | Scores/fréquences BIN documentés ; DOSBox muet | Collecte PCM et temps virtuel testés | NON TESTÉ : fidélité audible, rythme et latence physiques | E1 ; pas de verdict auditif |
| H01 | Partie humaine complète | Aucune session DOSBox jusqu'à victoire ici | Aucun parcours humain SDL complet | NON TESTÉ | Captures s'arrêtent à la création |

## Couverture des lieux

| ID | Lieu | DOS | C/SDL3 | Résultat / preuve |
|---|---|---|---|---|
| L01 | Amarande | Overlay 00FB, états aux frontières, milliers d'actions | Exploration et retours dans deux campagnes | CONFIRMÉ accès/navigation/sorties/progression parcourus ; toutes scènes NON TESTÉES |
| L02 | Cité des Crânes et Arioch | 002E/0049/0069 exécutés | Traversée, combats, objectif et retour | CONFIRMÉ parcours E1 ; toutes configurations de portes non énumérées en parcours |
| L03 | Pyramide | 0142 réel | Accès, mot et transition vers Meduz | CONFIRMÉ chemins E1 ; façade fixtures E3 |
| L04 | Alfol surface et bâtiments | 0085 réel | Voyage, exploration, bureau, paiement, retour | CONFIRMÉ deux parcours E1 ; réactivité spatiale humaine NON TESTÉE |
| L05 | Meduz | 00A4 réel | Extension mentale, invocation, objectif, retour | CONFIRMÉ deux parcours E1 ; fosses fixtures E3 |
| L06 | Temple de Seth | 0145 réel | Indices, portes, objectif et sortie | CONFIRMÉ chemins E1 ; présentation texte complète NON TESTÉE contre capture DOS |
| L07 | Kronopolis et villes | 00BC réel, villes placées par OVL0010 | Commerce/soins dans deux parcours | CONFIRMÉ chemins E1/E2 ; chaque ville séparément NON TESTÉE |
| L08 | Mahalore, Balgence, Roche aux Goules, Porte des sables, Mortefontaine | 00EC/00F0/00F3 atteints selon parcours | Événements et compositions présents | CONFIRMÉ uniquement événements atteints E1 ; toutes interactions de chaque lieu NON TESTÉES |
| L09 | Spatioport | 00AE et 00CB réels | Carburant/pilote/vol/retour | CONFIRMÉ parcours et tests horodatés E1 ; son/latence réels NON TESTÉS |

## Limites méthodologiques

L'oracle global commence à DBAC après la génération, abstrait BIOS, rendu,
attentes, fichiers et certains sons ; l'overlay de sauvegarde 00D2 est ignoré.
Les conditions terminales sont comparées avant sauvegarde/restauration écran.
Les campagnes sont à difficulté 0 et son désactivé ; des tests ciblés utilisent
le son activé. Le conducteur lit les cartes pour planifier, sans modifier la
partie après initialisation. La génération a été comparée séparément en 0/5/9.
Aucune preuve de toutes les graines, interruptions ou stratégies n'est déduite.
