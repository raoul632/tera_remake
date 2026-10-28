# Dépendances Original — 28 octobre 2026

| Élément | Phase | Nécessaire au joueur |
|---|---|---|
| tera_sdl3.exe avec données intégrées | RUNTIME | Oui |
| SDL3.dll x64 | RUNTIME | Oui pour ce build dynamique |
| Bibliothèques système Windows / UCRT | RUNTIME | Oui, fournies par Windows testé |
| Sauvegarde moderne versionnée | RUNTIME | Seulement pour reprendre ; F5/F9 et --load inchangés |
| NOMPERS, VALMON, VERBES, OBJETS, NOMON, MESSAGE/2/3, sept IMA | EXTRACTION | Aucun fichier séparé requis ; contenu converti et intégré |
| PNG/JSON convertis | DEVELOPMENT / GENERATION | Non |
| embedded.c généré | BUILD-TIME | Non |
| Python + Pillow | EXTRACTION / GENERATION optionnelle | Non ; build ordinaire à partir de C généré sans Python |
| C99, CMake et outils de construction | BUILD-TIME | Non |
| SDL3 headers et import library | BUILD-TIME frontend | Non ; core compilable sans SDL |
| TERA.COM, TERA.000, Pascal, manuel DOS | REFERENCE-ONLY | Non |
| Unicorn, scripts d'audit, bridges historiques | DEVELOPMENT-ONLY | Non |
| Police CP437 C et tables de rendu existantes | BUILD-TIME, intégrées | Aucun fichier de police requis |

Aucun déploiement automatique des fichiers originaux au lancement. Le mode
externe --data est strictement explicite ; le mode par défaut lit la mémoire
constante de l'exécutable. Il ne consulte ni dépôt, ni ancienne installation,
ni dossier utilisateur, ni réseau.

Voir [la chaîne de génération](TERA_EMBEDDED_RESOURCES.md),
[les preuves exécutées](TERA_EMBEDDED_VALIDATION.md) et
[le manifeste de provenance](TERA_ASSET_MANIFEST.md).
