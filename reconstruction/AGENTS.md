# TERA — instructions de travail

## Séparation des contextes 3 et 4

Instruction explicite de l'utilisateur : le contexte 3 suit exclusivement
`TERA_SPEC -> CORE C -> TESTS C`. Il comprend le moteur C, les tests unitaires
et d'intégration C, et si nécessaire un petit exécutable C de diagnostic en
ligne de commande. Ne créer aucune interface temporaire Python, Pygame,
Tkinter ou autre frontend pour piloter ou afficher le jeu.

Le Python existant reste une référence ou un outil optionnel : il ne doit
pas être une dépendance obligatoire de la compilation ou des tests C.
Si une vérification révèle une API ou des tests insuffisants, corriger ces
éléments dans le core C. Un rendu C temporaire est exceptionnel et limité
au test d'une primitive indispensable, sans interface de jeu complète.

Toute interface graphique appartient au contexte 4 : moteur C puis frontend
SDL définitif. Cette instruction prévaut sur les anciennes orientations
concernant les interfaces de développement.

Tous les nouveaux travaux restent dans `reconstruction/`. Consulter le
dépôt parent comme référence en lecture seule. Ne pas modifier son Python,
ses documents, ses outils ou ses tests ; tout transfert doit être séparé
et explicitement demandé. Les correctifs Python de l'audit sont archivés
dans `patches/` et testés sur une copie sous `build/`.

Lire `docs/cadre_projet.md` : cadre fourni par l'utilisateur, prioritaire sur
les anciens documents. Objectif : prototype fonctionnellement fidèle,
moteur C portable et frontend moderne. Arrêter aux critères de fin du cadre.

- Python et notes historiques sont des travaux partiels, jamais une preuve
  suffisante. Vérifier la provenance avant conservation ou port.
- Classer les résultats CONFIRMÉ, PROBABLE, À VÉRIFIER, MANQUANT ou
  MODERNISABLE, avec sources BIN/PAS/DATA/MANUAL/OBS/PY/INFERENCE.
- Ne pas inventer de mécanique, de contenu, d'équilibrage ou de confort.
- Reverse engineering ciblé ; abstraire DOS/BIOS sans effet sur le gameplay.
- Voir `docs/audit_fidelite.md` et `docs/spec_combat_actions.md` pour l'audit
  en cours. Le blueprint, CLAUDE.md et le mapping anciens comportent des
  contradictions ; leurs mentions « fidèle » doivent être revérifiées.
- Ne pas redistribuer les originaux et leurs ressources : respecter les
  exclusions existantes de `.gitignore`.
- Documenter les fonctions : role, entrees/sorties, effets de bord et
  limites utiles. Utiliser des noms explicites. Commenter les intentions,
  la provenance et les particularites originales (ordre des tirages,
  arrondis, debordements), sans paraphraser chaque instruction.
