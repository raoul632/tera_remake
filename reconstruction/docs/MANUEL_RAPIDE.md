# TERA — mon premier essai

Ce guide est fait pour quelqu’un qui découvre complètement TERA.
Ton premier objectif : créer un personnage, marcher, comprendre les écrans
et essayer quelques actions. Tu n’as pas besoin de terminer le jeu pour le tester.

## 1. Lancer le jeu

Ouvre ce dossier dans l’Explorateur Windows :

`E:\dev\PYTHON\PROJETS\tera_remake\reconstruction\build\sdl3\runtime-only`

Double-clique sur **tera_sdl3.exe**. Laisse le dossier **data** et le fichier
**SDL3.dll** à côté de lui.

- Clique dans la fenêtre du jeu pour lui donner le clavier.
- Appuie sur **Entrée** pour passer l’image de présentation.
- **F11** permet de passer en plein écran et de revenir en fenêtre.

Le jeu se commande au clavier. La souris ne sert pas à déplacer le personnage.

## 2. Créer ton premier personnage

Pour ton premier essai, tu peux suivre cette recette :

1. Au nom, écris **ALIX**, puis appuie sur **Entrée**.
2. À la question « H(omme) ou F(emme) ? », appuie sur **H** ou **F**.
3. Le jeu demande trois répartitions de points. Pour commencer simplement,
   réponds **1** à chacune des trois questions, en attendant la question suivante.
4. À l’écran des bonifications, appuie successivement sur
   **F**, **F**, **E**, **E**, **T**, **T**. Cela utilise les six points supplémentaires.
5. À « gardes-tu ce personnage ? », appuie sur **O**, la lettre O de « oui ».
   **N** recommencerait la création.

Cette recette permet de découvrir le jeu ; ce n’est pas une recherche du meilleur personnage.
Tu peux utiliser ton propre nom et d’autres répartitions lors d’un prochain essai.

Sur un clavier AZERTY, les chiffres de la rangée supérieure demandent
habituellement **Majuscule**. Tu peux aussi utiliser le pavé numérique.

## 3. Comprendre ce que tu vois

Tu débutes dans **Amarande**, le monde extérieur. Tu regardes un paysage
à travers les yeux de ton personnage : ce n’est pas une vue de dessus.

Lis surtout les **messages et questions en bas de l’écran**. Ils indiquent
ce qui vient de se passer et ce que le jeu attend de toi.

Quelques mots utiles :

| Mot affiché | Ce que cela signifie |
|---|---|
| PV | Points de vie : la santé du personnage |
| PM | Réserve utilisée par certaines actions mentales ou magiques |
| Ducats | Ton argent |
| OK | État normal du personnage |
| A–D | Lettres servant à choisir un membre du groupe |
| 0–9 | Numéros servant notamment à choisir un emplacement d’inventaire |

Tu commences avec ton héros. D’autres personnages peuvent rejoindre le groupe.
Le héros neuf ne possède pas d’arme : l’action « tirer » n’est donc pas un bon
premier choix au combat.

## 4. Les règles du clavier

**Une touche à la fois, puis regarde ce qui change.**

Quand ce guide écrit « H → I », cela veut dire : appuie sur H, relâche,
puis appuie sur I. Ce n’est pas un raccourci à maintenir ensemble.

- **O** signifie oui. C’est une lettre, pas le chiffre zéro.
- **N** signifie non lorsqu’une question oui/non est affichée.
- **Entrée** valide le nom, les mots et de nombreuses confirmations.
- **Échap** ne permet pas toujours de revenir en arrière : certains écrans
  attendent obligatoirement une lettre de personnage ou un chiffre.
- Pendant une animation ou un message temporisé, attends avant d’appuyer.
  Les touches tapées trop tôt peuvent être ignorées.

Une même lettre a plusieurs rôles : **N** ouvre la négociation en exploration,
mais peut refuser une confirmation. **L** sert à lire en exploration et à lutter
au combat. Le contexte affiché fait foi.

## 5. Ton premier parcours, en dix minutes

### A. Regarder ta fiche

Quand le paysage apparaît, appuie sur **H**, puis **I**.
Tu vois la fiche de ton héros : caractéristiques, santé et équipement.
La prochaine commande masquera cette fiche ; une flèche fera aussi un déplacement.

### B. Faire une première sauvegarde

Au paysage, hors combat et hors menu, appuie sur **F5**.
Cela crée ton point de reprise. **F5 remplace la sauvegarde précédente.**
À partir de la version 0.1.1, « Partie sauvegardée » apparaît quelques secondes
en bas de l’écran après une réussite. Si l’action est indisponible ou échoue,
un message l’indique. La version 0.1.0 n’affichait pas cette confirmation.

### C. Marcher un peu

Utilise les **quatre flèches**, une pression à la fois.
Sur Amarande : haut = nord, bas = sud, gauche = ouest, droite = est.

Fais quelques pas et regarde les changements de décor et les messages.
Si un obstacle bloque le passage, essaie une autre direction.
Une rencontre peut interrompre la marche : c’est une partie normale du jeu.

Les cartes sont générées au démarrage : tu n’auras pas forcément les mêmes
lieux au même endroit qu’un autre joueur. Il n’y a donc pas de chemin fixe
« trois pas à gauche » à suivre pour ce premier essai.

### D. Essayer l’aide et la carte

- **?** ou **+** : aide des commandes ; une touche permet de revenir.
- **H → C** : demander la carte. Certaines cartes doivent d’abord être obtenues.

Si la carte n’apparaît pas, lis la réponse du jeu : cela ne signifie pas
forcément que la commande est en panne.

### E. Essayer une interaction

Quand tu rencontres quelqu’un, essaie **N → I → O** :
négocier, interroger, confirmer.

Pour repérer les compagnons disponibles, essaie **N → S → O**.
Pour tenter ensuite de les recruter, utilise **S → O**, directement en exploration.
La réussite dépend de la situation et peut coûter 250 ducats.

Si tu trouves un lieu dans lequel on peut entrer, essaie **E**, puis réponds
à la confirmation affichée. Une commande refusée dans un lieu inadapté
n’est pas nécessairement un bug.

### Le menu de négociation, en pratique

Appuie sur **N**, puis choisis une action, puis confirme avec **O** :

| Séquence | Ce que tu demandes |
|---|---|
| N → I → O | Interroger : demander des informations ou des indices |
| N → S → O | Solliciter : vérifier si des compagnons sont disponibles ici |
| N → D → O | Donner : poursuivre avec les choix de personnage et d’objet affichés |

Les lettres A (acheter) et V (vendre) existent aussi dans ce menu, mais
le vrai catalogue commercial se trouve dans le commerce de Kronopolis :
**E → O**, puis **A → O** ou **V → O**.

Après N → S → O, si quelqu’un est disponible, **S → O** en exploration
tente le recrutement. Cette seconde commande signifie « séduire » dans le jeu.
Ce n’est pas le même S que dans le menu N. Un échec de persuasion peut
entraîner un paiement automatique de 250 ducats si tu en as assez.
Le groupe peut normalement compter jusqu’à quatre membres.

Pour annuler une confirmation, réponds **N**. Pour sortir du menu de choix
juste après N, utilise **Échap**. Lis la réponse en bas de l’écran avant de marcher.

## 6. Si un combat commence

Le nom suivi de **→** indique qui doit agir. Tu ne dois pas cliquer sur le monstre.

Pour un premier essai :

1. Appuie sur **L**, pour lutter.
2. Confirme avec **O** ou **Entrée**.
3. Attends le résultat et regarde les PV.
4. Si le combat continue, recommence quand le jeu demande une nouvelle action.

Pour tenter de partir : **F**, puis **O**. La fuite peut échouer.
Les caractéristiques, les compétences, l’équipement et l’ennemi influencent le résultat.

Autres actions, à découvrir plus tard :

| Touche | Action |
|---|---|
| T | Tirer |
| J | Jeter un sort |
| D | Dissiper |
| A | Attaque mentale |
| P | Psalmodier |
| C | Crier : saisir le mot demandé, puis Entrée |

Si le groupe est vaincu, le jeu peut proposer de créer un successeur.
Ce n’est pas forcément un plantage. Pour ton test, tu peux aussi reprendre
la sauvegarde quand tu retrouves l’exploration.

## 7. Si tu trouves Kronopolis : essayer la boutique

1. **E**, puis **O** : entrer dans le commerce.
2. **A**, puis **O** : choisir l’achat et confirmer.
3. **A** : choisir le premier personnage du groupe.
4. Choisis la lettre d’un article du catalogue, si tu as assez d’argent.

Après un achat d’équipement, sa liste apparaît pendant **deux secondes**,
puis le catalogue revient. Attends ce retour avant la commande suivante.

Pour sortir : **Échap** quitte la liste d’achat, puis **Échap** quitte le menu
commercial. La première attente du menu peut demander deux Échap.
Si le jeu demande encore « qui effectue la transaction ? », choisis d’abord **A**.

Dans ce commerce, **V** permet de vendre et **H** mène à l’hôpital ;
réponds aux confirmations affichées. Les soins font passer du temps dans le jeu.

## 8. Sauvegarder, reprendre et quitter

| Touche | Utilité |
|---|---|
| F5 | Sauvegarder lorsque le jeu attend une commande d’exploration |
| F9 | Revenir à la dernière sauvegarde, dans cette même situation |
| F11 | Plein écran / fenêtre |

**F5 et F9 ne fonctionnent pas pendant la création, un menu, un combat
ou une animation.** Attends de retrouver l’exploration.

Pour quitter tranquillement : sauvegarde avec **F5**, puis ferme la fenêtre.
Sur Amarande, tu peux aussi utiliser **H → Échap → O**.
Quitter par ce menu ne remplace pas la sauvegarde manuelle F5.

**Pour reprendre après avoir fermé le jeu :** relance le même exécutable.
Cette version démarre directement une nouvelle création de personnage.
Termine cette création rapidement, puis, dès que le paysage est visible,
appuie sur **F9 pour retrouver ta partie sauvegardée**.
Ne fais pas F5 avant F9 : tu remplacerais ton ancienne sauvegarde par la nouvelle partie.

## 9. Quand tu te sens à l’aise

- **H → D → O** : essayer de dormir.
- **H → M → O** : essayer de manger.
- **O** près d’un coffre : tenter de l’ouvrir ; suivre les demandes.
- **P** : prendre un contenu accessible ; suivre les choix affichés.
- **B** : boire une potion ; choisir le personnage puis l’emplacement de l’objet.
- **L** : lire une inscription, quand le lieu le permet.

Dans les souterrains, les déplacements deviennent relatifs à ton orientation.
Ne suppose pas que « haut » signifie toujours le nord. Pour certaines portes,
le jeu attend **O** pour ouvrir ou **U** pour déverrouiller : lis la question.

Le but final est de remplir quatre objectifs, liés à la Cité des Crânes,
au temple de Seth, à Alfol et à Meduz. Les voyages, mots et objets nécessaires
se découvrent progressivement. Garde les indices et les mots importants sur papier.
Le voyage spatial vient plus tard : il demande un pilote, du carburant et
des tirs avec les chiffres 1–9 du pavé numérique. Tu peux l’ignorer pour ce premier test.

## 10. Quoi me signaler pendant ton test ?

Ton premier test est déjà utile si tu as réussi à :

- [ ] Créer ton personnage et atteindre le paysage.
- [ ] Ouvrir sa fiche avec H → I.
- [ ] Faire quelques déplacements.
- [ ] Sauvegarder, te déplacer, puis retrouver ta position avec F9.
- [ ] Essayer une rencontre, un combat ou une boutique si tu en trouves.

Si quelque chose paraît bloqué, note **ce qui est écrit à l’écran**, **les touches
que tu viens d’appuyer** et **ce que tu pensais obtenir**. Une capture d’écran
avec ces informations permettra de distinguer une règle du jeu d’un problème du portage.
