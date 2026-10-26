# Spatioport, voyage spatial et Alfol — lots 19 et 20

## Sources et portée

**CONFIRMÉ — BIN/PAS/DATA** : TERAMAIN E222..E3EA, OVL00AE
(`TERA.000[AE00:BC00]`), OVL0085 (`[8500:A400]`), C8315, CA0DD,
C7EF6/C7DBD et les routines de rendu/son résidentes. Les captures ciblées
et empreintes sont produites par `tools/audit_space_alfol_binary.py` dans
`build/audit/`, exclu de Git. Aucun original n'est redistribué.

Les règles sont dans `engine/src/space.c` et `alfol.c`. Les ponts C
suspendent aux lectures et combats ; `frontend/space.py` et `alfol.py`
présentent ces lectures dans la boucle d'exploration. Aucun tirage, prix,
dégât ou résultat de pilotage n'est calculé en Python.

## Spatioport et vol

Au spatioport, lieu 1 d'Amarande, **E**, puis O/Entrée confirme le départ.
Le programme consomme dix jets C8315(colonne 4, compétence 4), même pour
les inactifs et HS. Le dernier actif non HS dont le jet est positif pilote.
Son résultat détermine `max(4, (150-jet) DIV 10 + difficulté)`, avec les
Integer signés 16 bits. Sans pilote, le groupe reste sur Amarande. Le refus
ou l'échec consomme encore le RANDOM(3) de la réponse générique E.

Le ravitaillement accepte **O seulement**, puis un chiffre 0..9 ; ESC et
les lettres redemandent le chiffre. Coût et énergie ajoutée valent
`50*chiffre`. L'achat utilise `coût < money+1` signé, débordement à 32767
compris. Entrée au premier prompt continue sans ravitailler.

Chaque pirate apparaît dans l'une des neuf directions du pavé :

| | Gauche | Centre | Droite |
|---|---:|---:|---:|
| Haut | 7 | 8 | 9 |
| Milieu | 4 | 5 | 6 |
| Bas | 1 | 2 | 3 |

Le compteur commence au nombre de pirates. Tir correct : pirates −1,
compteur −1, expérience +3 ; tir manqué ou absent : énergie −5 et
explosion. Le compteur baisse ensuite encore d'une unité dans les deux cas.
La boucle fait au moins un essai. Si énergie <5, bruit de destruction,
quatre explosions et **HALT**, sans condoléances ni nouveau héros. Sinon,
fin lorsque compteur <1 : quatre explosions, étoiles, énergie −25 si >25,
sinon énergie=5, puis sept jours supplémentaires.

Les 24 étoiles consomment chacune R200/R90 ; chaque explosion consomme
huit paires R4/R16 et, avec son, seize R100 par image. Les ondes
déterministes C666B/C6721/C6221 n'ajoutent aucun RANDOM. La destruction
avec son consomme aussi 10 000 R255. Cet ordre fait partie du gameplay.

**MODERNISABLE — terminal** : la cible est affichée en texte. En TTY,
délai R10×100 ms, purges initiales, point 300 ms, puis quatre tailles
230/200/170/90 ms et un poll final. En pipe, une touche par poll final,
`0` pour aucune touche ; EOF interrompt sans exporter. La comparaison
binaire impose les mêmes touches au dernier poll et laisse les purges
précédentes vides. Elle ne certifie pas l'ordonnancement réel du terminal.

## Alfol : surface et bâtiments

L'arrivée place le groupe en 0, WorldId=11, OnAlfol vrai, puis effectue
un rendu et un jour. Sur cette surface de cent positions, 8/2 ajoutent
ou retirent 1 ; 6/4 ajoutent ou retirent 10 ; débordement circulaire 0..99.
Chaque déplacement effectif rend son décor puis appelle CA0DD, rencontres
et reprise après fuite comprises. Le rendu conserve ses tirages, même si
les graphismes sont abstraits. La fuite de surface utilise R99+1.

En position `alfol_position`, **E**, puis O/Entrée entre dans les bâtiments :
position 900, précédente 899, WorldId=9. Navigation relative 2468, portes O,
clé U selon le monde 9. Le passage en 899 est encore rendu et actualisé
avant de sortir. La sortie redessine la ville sans jour supplémentaire,
puis replace le groupe en `alfol_position`, WorldId=11, OnAlfol toujours vrai.

Les salles proposent les PNJ présents (dont les identifiants 7/9), le
bureau des médias et les dépôts. N/S et A/B passent par les modules
résidents déjà portés. I lit le mot, puis l'invocation générique est
refusée par OnAlfol. Les monstres de salle sont choisis par R3+12, même si
la carte stockait un autre identifiant non nul ; la victoire efface ce
monstre dans la carte, puis le jour est exécuté.

Le **bureau des médias** demande O seulement : argent >999 ⇒ −1000.
Si `tasks_done[1]` était faux, il devient vrai et le compteur augmente
d'une unité, avec les trois mélodies originales. Revenir et payer retire
encore 1000, sans nouvel objectif. Argent insuffisant conserve le butin et
joue son bruit original si le son est actif.

Un **dépôt radioactif** consomme dix-huit R3 et, avec son, le bruit
C62B7(110,10). Il retire un HP à chaque actif dont HP>0. Le COM appelle
ensuite UpdateHSState avec **D9B66**, au lieu de l'indice local : le port
garde cette faute. Le jour CA0DD préserve D9B66 ; il vaut 16 après le dessin
de l'atterrissage ou de la surface, l'identifiant du
dernier compagnon abandonné après A/C, 9 après le scan d'invocation ou de
négociation. Cette conservation a été revérifiée lors du port des événements
d'Amarande ; la comparaison Alfol contrôle maintenant cet indice à chaque lecture.

Pour l'indice 16, C7CC8 lit en réalité MP[6] et écrit dans le premier texte
d'indice : si MP[6]<1 et les trois octets ne sont pas déjà `02 48 53`,
il met MP[6] à zéro et écrit ces octets aux caractères 17..19 du texte
(indices C). Ces alias DOS sont reproduits explicitement, sans accès C
hors tableau ; les octets latents du texte sont conservés dans le snapshot.
Les autres indices hors 0..9/16 sont refusés comme contexte non couvert.

**R** demande confirmation sur la surface. Particularité confirmée : au
point zéro, même un refus sort de l'overlay, car UNTIL ignore la réponse.
Ailleurs, R conserve le groupe sur Alfol. Si le groupe est disponible,
le retour reprend les pirates restants, au moins quatre, restaure la
position du spatioport, puis désactive OnAlfol après un vol réussi.
La mort au sol revient aux condoléances/successeur sans vol retour.

Comme chez Seth, une arrivée mortelle attend encore une première commande
(REPEAT). Le snapshot conserve cette attente ; Q ne la consomme pas.

## État moderne et limites

`space_pirates`, `space_backup_position`, `on_alfol`, `alfol_status_actor`,
`alfol_entry_pending`, les objectifs, cartes et éventuels octets d'indice
sont conservés à l'export/rechargement. `program_halted` conserve le HALT
spatial et interdit une reprise artificielle de la partie.
Q est un export technique aux frontières de commandes, distinct d'une
sauvegarde DOS. Un dialogue ou combat interrompu ne publie pas d'export.

Meduz/monde 10 et la fin victorieuse sont maintenant portés dans les
[lots 21/22](spec_meduz_fin.md).
**MANQUANT** : autres lieux/événements d'Amarande, conteneur de sauvegarde
DOS et validation d'un parcours complet.
**MODERNISABLE** : graphismes, sons audibles et présentation des écrans.
**À VÉRIFIER** : ergonomie et timing réels du combat spatial en terminal,
interruptions clavier des autres animations. Ces lots ne déclarent pas
le jeu entier terminé.
