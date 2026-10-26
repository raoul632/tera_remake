# Bloc de fidélité des menus SDL

Sources en lecture seule : PAS OVL_00BC et OVL_0022 ; BIN C4779,
C47B3, C4800, C81D7, CA709 et CAFE8 ; DATA VERBES et MESSAGE.

CONFIRMÉ : C4779 écrit en ligne 25 (y=192), C47B3 en lignes 24/25
(y=184/192). C4800 ajoute ` ? ` au verbe, sans afficher une nouvelle
liste de raccourcis. O et Entrée confirment ; le lecteur du core décide.

CONFIRMÉ : H affiche `halte` et la réponse 7 déjà choisie par le core,
sur le décor. Ce menu ne révèle pas la fiche du héros. H/I et H/S seuls
présentent les photographies correspondantes. La sous-commande H doit
être exposée dans la vue pour distinguer dormir, manger, sonder et quitter.
N affiche `Negocier` et MESSAGE 95, sans liste ajoutée.

CONFIRMÉ : Kronopolis affiche le texte commercial de l'overlay, aux
lignes 1,2,4,5,6,7,9 ; `[ESC -> sortir]` colonne 24, ligne 25.
La première lecture entre dans REPEAT même sur ESC ; les suivantes sortent.
Les achats affichent MESSAGE 72..90 en lignes 5..23, le groupe dans le
cadre supérieur, l'argent colonne 3 ligne 24 et ESC colonne 26 ligne 24.
Le choix du négociateur précède celui de l'article. La vente affiche les
objets 1..24 et leurs tarifs de base à droite (colonne 22), l'inventaire à
gauche, puis l'invite de sélection en ligne 25. Les montants négociés sont
calculés exclusivement par le core, jamais par le frontend.

CONFIRMÉ : la création comporte les cadres texte (2,2,78,23) et
(20,4,60,20), trois répartitions et le pavé de bonification I/V/C,
E/T/N, H/A/F sur trois lignes. Seul N refuse la fiche finale. Aucune
restriction aux chiffres 1..pool-1 n'est ajoutée au lecteur original.

À VÉRIFIER : identité des glyphes DOS et détails de palette. La fonte libre
reste une substitution visible. Le combat et les compléments des scènes
sont raccordés ; carte/aide restent manquants. Ce bloc ne certifie donc
pas une identité graphique globale avec une capture DOS.

Périmètre de l'audit des menus :

| Groupe | État du rendu après ce bloc |
| --- | --- |
| Kronopolis, achat A–S et vente 1..24 | Textes/positions PAS/DATA repris ; conservation exacte des fonds d'écran de retour encore à vérifier |
| H, dormir, manger, accélérer, sonder, quitter | Routage et verbes confirmés ; aucune fiche anticipée ni liste ajoutée |
| N, interroger, acheter, vendre, donner, solliciter | Titre et invite MESSAGE 95 repris ; choix et confirmations du core conservés |
| Création | Nom/sexe, cadre, grille bonus et validation corrigés ; historique/scroll des répartitions encore approchés |
| Inventaire et fiches | Lecteurs confirmés, mise en page et quelques invites encore approchées |
| Portes, Meduz et Alfol | Lecteurs confirmés ; invitation médias et méditation corrigées, autres aides SDL encore approchées |
| Combat terrestre | Bloc 6/7 et compléments de silhouettes intégrés : voir spec_combat_media_sdl_c.md |
| Aide, carte, fin | Présentation originale MANQUANTE ; diagnostic ou marqueur explicite |

Mise à jour du 26 octobre 2026 : la photographie d'équipement après achat
respecte désormais DELAY(2000), puis le catalogue est restauré. Les touches
arrivées pendant le délai sont purgées comme C4210, dont la boucle originale
a été exécutée séparément ; une nouvelle touche est ensuite acceptée.
Le test `sdl3_shop_presentation` compare aussi les pixels du catalogue restauré
et vérifie que le core/RNG ne changent pas pendant l'attente. Les campagnes
smoke accélèrent toujours le temps ; le nouveau test exerce le mode temporisé.
Les autres délais du commerce restent à comparer. Voir AUD-003 dans
TERA_PORT_ISSUES.md pour la portée exacte de la preuve.

Validation : contrats de vue sans mutation ; parcours SDL commerce
et création, comparés à la même séquence de touches sans rendu ; captures
des écrans et comparaison des pixels du catalogue avec les textes DATA.
Les parcours incluent le premier ESC ignoré, l'ESC ignoré au choix du
négociateur, achat/vente/hôpital, H/D, H/M et H/ESC refusés. Le catalogue
est contrôlé sur toute sa région intérieure, hors bordure du groupe et
bordure de la ligne d'argent ; l'oracle ne certifie pas la fonte DOS.

Résultat du 16 octobre 2026 : compilation stricte réussie, **35/35 suites
CTest SDL/core réussies**, paquet runtime autonome reconstruit et testé.
Compilation sans SDL des contrats de vue et commerce, puis **2/2 tests
ciblés réussis**. Captures inspectées : shop-menu, shop-catalogue,
shop-sell, creation-bonus, halte-menu et halte-dormir dans build/sdl3/.
