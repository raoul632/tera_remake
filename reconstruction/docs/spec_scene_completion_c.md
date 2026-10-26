# Compléments des scènes — sources et contrat

CONFIRMÉ PAS/BIN : OVL00FB/BD0C dessine un disque (60,40,r8) en
direction 6 ; C6C4F compose sept paires de sprites 4B/4C avec les 35
tirages déjà exécutés. Le premier sprite utilise 12*i+R4-2,50+2*R4,
le second 6+12*i+R4-2,74+2*R4, i=0..6. Ne pas retirer ces valeurs.

CONFIRMÉ PAS/BIN : C6DC5 est le gratte-ciel (x en octets, base y,
hauteur, largeur en octets, couleur, fenêtres). C70A0 compose trois
rangées de trois immeubles, C7136 ajoute trois groupes de sprites et le
ciel. OVL0085/C30D compose horizon, montagnes, neuf immeubles, ruines,
39 sprites de sol et avant-plan. Le BIN confirme C70A0 dans cette scène ;
C6F17 est une autre rangée paramétrable.
Meduz emploie C6DC5(38,164,140,4,2,0) comme catalyseur.

CONFIRMÉ PAS : OVL0142 (Pyramide), OVL00F0/00EC (temples), OVL00F3
(Cité) décrivent les décorations dont les tirages sont déjà exécutés dans
commerce.c. OVL0085 décrit les déchets radioactifs et le graphique du
bureau. OVL00A4 fournit étoiles et cinq sprites du combat mental.

Contrat : les valeurs graphiques déjà tirées sont transmises par un
événement SCENERY, identifié par monde, position et famille. Le frontend
conserve ces détails seulement pour la scène correspondante. Il ne tire
aucun nombre et ne régénère rien après chargement. Les paysages Amarande
conservent directement leurs 35 valeurs de montagnes dans TeraLandscape.
Les sauvegardes et les résultats de jeu ne changent pas. Les figures
déterministes sont dessinées dans SDL à partir des coordonnées originales.

La comparaison des primitives BIN certifie compositions et paramètres.
L'identité pixel de la fonte, des ellipses et du tracé SDL/CGA reste
À VÉRIFIER ; aucune nouvelle décoration n'est inventée.

## Bloc livré

| Scène | Compléments | Comparatif original |
| --- | --- | --- |
| Amarande | Lune à l'est, 14 sprites de montagnes, inscriptions des lieux 10..19 | CONFIRMÉ PAS/BIN ; arrondis du disque et remplissage du monolithe À VÉRIFIER |
| Spatioport | 16 étoiles, neuf immeubles avec fenêtres et quinze vaisseaux décoratifs | CONFIRMÉ BIN C7136, 1 388 primitives comparées |
| Cité | Créneaux inclinés, tours, fenêtres, porche, sprites 31/37/38 | CONFIRMÉ PAS ; porche/remplissages SDL À VÉRIFIER au pixel près |
| Pyramide/temples | Lune/étoiles, 26 ou neuf sprites de sol, ordre des avant-plans | CONFIRMÉ PAS ; rasterisation du disque MODERNISABLE |
| Façades | Portraits C7A96/C7B05 et PNJ de l'ermitage | CONFIRMÉ BIN/PAS ; un portrait comparé aux 32 primitives BIN |
| Souterrains | Perspectives des portes et panneaux, cadres, autel et monolithe, motifs aléatoires | Contours PAS ; remplissages SDL À VÉRIFIER |
| Alfol | Surface modulo 23, montagnes, immeubles, arrivée, déchets, bureau avec graphique et personnage | CONFIRMÉ PAS/BIN ; 18 hauteurs et huit valeurs du graphique conservées |
| Meduz | Étoiles avant fosses/catalyseur, catalyseur, cinq sprites du combat mental | CONFIRMÉ PAS/BIN ; quatre primitives du catalyseur comparées |
| Spatial | 24 étoiles, rayon blanc double, 30 ms sans son / 36 ms avec son | CONFIRMÉ PAS ; neuf directions, son actif/inactif et horloge virtuelle testés |

Les événements SCENERY ne sont pas affichés dans le journal de texte.
Le cache est effacé au chargement ; les éléments fixes restent dessinés,
sans retirer de hasard pour retrouver les détails perdus. La sortie des
bâtiments d'Alfol rattache l'image à la destination sans avancer la mutation
de WorldId/Position. Le retour de Meduz capture les nouveaux tirages de
la Pyramide. Les conditions des portraits de façade suivent TERAMAIN,
distinctement de la disponibilité des personnages au recrutement.

Les palettes 2/3 suivent les CASE PAS : palette 2 pour Cité, Seth, ermitage,
abbaye, souterrains et Meduz ; palette 3 pour les autres scènes. Les douze
couleurs des quatre palettes sont testées. La recoloration concerne la
zone logique, en conservant les bandes extérieures de la fenêtre.

Validation : **37/37 suites CTest**, paquet autonome reconstruit. Les tests
SDL confrontent quatre composants aux primitives natives exécutées,
contrôlent les trois vues d'Alfol et le dépôt, les 89 valeurs du spatioport
et les 35 valeurs de montagnes, sans mutation des sessions. L'outil
`tools/audit_scene_binary.py` est optionnel ; ses vecteurs C sont inclus
dans les tests, aucun COM/Python n'est nécessaire pour jouer.

Audit séparé des règles/RNG originales exécutées : Amarande 854,
navigation 887, Alfol 613, Meduz 326 et voyages Meduz 48 scénarios :
**2 728 scénarios, zéro écart**. Cet audit abstrait la vidéo et complète
la comparaison des primitives ; il ne certifie pas tous les écrans DOS.
Les campagnes et la sauvegarde de 11 521 octets restent inchangées.

Hors de ce bloc statique : tunnels de transition Meduz, carte/aide,
textes résidents de Seth, écran de fin et fonte originale. Le parcours
humain complet et les arrondis CGA restent À VÉRIFIER.
