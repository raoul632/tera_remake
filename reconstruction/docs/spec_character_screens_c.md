# Écrans de création et de personnages

CONFIRMÉ [PAS OVL_0022] : création en texte 80x25, cadre bleu/jaune,
nom, sexe, trois répartitions, six bonifications, puis fiche graphique et
validation. Seul N refuse la fiche. SDL transmet les touches sans filtrer
les valeurs, sans calcul de caractéristiques ni tirage supplémentaire.

CONFIRMÉ [BIN C4873/C49FD/C4A8B/C4D46] : fiche graphique 320x200,
caractéristiques en grille I/V/C, E/T/N, H/A/F, compétences non nulles,
groupe de quatre actifs dans l'ordre du core (y compris HS).
La fonte libre actuelle remplace la fonte DOS ; les espacements des invites
et la colonne d'équipement sont une approximation de présentation.

Le core fournit une photographie par valeur lors de H/I et de chaque fiche
H/S autorisée. H/I ne crée aucune pause : l'affichage reste visible jusqu'à
la prochaine commande, qui est transmise au core. H/S conserve ses vraies
lectures. Aucun navigateur de compagnons supplémentaire n'est ajouté.
Les limites HP/MP sont copiées après leur rafraîchissement existant ; rendre
un écran ne doit jamais appeler ce rafraîchissement.

CONFIRMÉ [SOURCE C / tests exécutés] : `tera_character_card` copie nom CP437,
caractéristiques, compétences, dix objets, HP/MP/maxima, état, clé et compteurs.
Le paramètre de combat facultatif sélectionne les valeurs d'un combat actif.
`TERA_PRESENT_CHARACTER` transporte cette valeur avant la fin de H/I et à
chaque étape H/S. Aucun de ces champs ne modifie le format de sauvegarde.

Validation : 33 suites CTest, dont `sdl3_character_screens` avec ressources
originales, refus/recréation, zéro et valeur excessive transmis au core,
bonus invalide sans mutation, H/I, pauses H/S (indices 3 et 8, HS inclus),
équipement aux emplacements 0 et 9 et sélection d'inventaire. Chaque capture
compare la session avant/après rendu. Les tests core seuls vérifient la
photographie du combat, sa durée de vie, les événements H/I et H/S et
l'égalité des sauvegardes avec/sans observation. Le smoke de l'application
exerce aussi le maintien puis l'effacement de la fiche H/I.

Captures locales dans `build/sdl3/creation-*.bmp` et `character-*.bmp`.
Limites du jalon initial : fonte de remplacement, libellés abrégés pour la
grille de 40 colonnes, espacement et décorations DOS approchés.
Le bloc menus ultérieur corrige H (décor, fiche seulement après I/S), les
cadres/bonifications et l'invite de validation ; voir
[audit et limites actuelles](spec_menus_sdl_c.md). L'historique/scroll des
répartitions et la disposition de l'inventaire restent approchés. Les lieux
ont depuis leurs compositions SDL partielles ; le combat est maintenant
présenté par le bloc 6/7, la fin reste un diagnostic. Aucun parcours
graphique jusqu'à la victoire n'est certifié.
