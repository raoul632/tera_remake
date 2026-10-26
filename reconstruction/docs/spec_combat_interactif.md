# Combat interactif terminé dans le moteur C

Statut **CONFIRMÉ BIN/PAS/DATA** pour les règles ci-dessous. Port :
`engine/src/interactive.c`, `engine/src/random.c`. Frontend de validation
jouable : `frontend/combat.py`, pont `frontend/combat_bridge.c`.
Les originaux et le Python historique restent inchangés.

## Périmètre livré

L'appelant fournit l'état des personnages, leur équipement, H, le mot
secret, les 22 lignes VALMON, le monde, les cartes et la graine persistante.
`tera_combat_start` initialise réellement la rencontre. `tera_combat_key`
fournit une touche et avance jusqu'à la prochaine saisie ou la fin.
Les menus ne reçoivent plus de résultat d'action prédéfini et les ripostes
ne reçoivent plus de défense prédéfinie. Le moteur appelle les formules
déjà validées et effectue lui-même chaque tirage au bon moment.

Le combat se termine par victoire, fuite ou groupe hors de combat.
L'état de sortie comprend HP, MP, maxima, états HS/EM, argent, expérience,
position, 9B72, AFD1 et la graine après le dernier tirage. Les cartes et
l'équipement ne sont pas consommés ou modifiés par ce bloc.
Le calendrier des tours de `spec_combat_boucle.md` reste conservé.

## Initialisation — C_838A / 95D9–9757

1. Remettre AFCC, victoire, fuite et disponibilité à zéro.
2. Normaliser les personnages actifs par C_7CC8. Si aucun n'est apte,
   revenir sans tirage, sans monstre initialisé et sans récompense.
3. Si l'argument agresseur vaut zéro, RANDOM(3) choisit l'annonce.
   Un autre ID 1..9 ne consomme pas ce tirage.
4. HP initiaux = `VALMON[type,1] + RANDOM(VALMON[type,6])`, addition 16 bits.
   Conserver séparément ces HP pour la récompense d'expérience.
5. Les résistances, catégorie de réaction et initiative viennent des
   colonnes 2..5. Le nom est lu dans NOMON, chaîne Pascal de 40 octets.

## Menu, confirmation, annulation — C_8790, C_81D7, C_4800

| Touche | Verbe VERBES | Type C_842A |
|---|---:|---:|
| T | 26, tirer | 2 |
| L | 27, lutter | 1 |
| J | 28, jeter un sort | 9 |
| D | 29, dissiper | 4 |
| A | 31, attaque mentale | 3 |
| P | 32, psalmodier | 5 |
| C | 33, crier | 6 |
| F | 6, fuir | relocation |

Les minuscules ASCII sont converties en majuscules à la lecture.
La confirmation lit **une seule touche** : O/o ou Entrée confirme ;
toute autre touche annule (y compris N, Échap et espace). Il n'y a pas
de boucle qui attendrait exclusivement O ou N.

Chaque personnage dispose de deux tentatives de menu : une première
touche inconnue ou annulation laisse recommencer ; une deuxième termine
son tour sans action. Une action confirmée, même inefficace, termine le
tour immédiatement. Deux touches inconnues constituent donc le passage
original ; aucun nouveau bouton de passage de tour n'a été ajouté.
La première tentative ne tire rien en cas d'annulation. C_4503 n'est
qu'un son/une temporisation, sans RANDOM.

## Mot — C_42AC et stockage A859

Après C confirmé, lire les lettres A..Z ; les autres caractères sont
ignorés, sauf retour arrière qui efface la dernière lettre, et Entrée
qui termine la saisie. La saisie termine aussi automatiquement dès que
le buffer contient dix lettres. Le stockage de combat n'en garde que
les **neuf premières**, puis les compare exactement à SecretWords[1].
Le mot peut être vide. Ni C annulé ni la saisie elle-même ne tire de nombre.
Après la saisie, C_842A(6) tire et résout le cri selon la catégorie du monstre.

## RANDOM et RANDOMIZE — 10DA/10E6 et 0F14

```
RANDOMIZE : seed = heure<<24 | minute<<16 | seconde<<8 | centième
RANDOM(n) : seed = (129 * seed + 0x361962E9) modulo 2^32
            résultat = (seed >> 17) modulo n
```

Le biais modulo et la sortie sur quinze bits sont originaux ; ne pas
substituer un générateur standard moderne. La graine reste dans l'état
du jeu entre rencontres. Une graine fournie permet un rejeu identique.
Le frontend adapte l'heure locale aux quatre octets DOS sans autre mélange.

Ordre des tirages conservé : annonce éventuelle (3), variance HP,
initiative (5), jets C_842A (15, puis éventuellement un deuxième 15),
phrase (3), puis si nécessaire cible (effectif), défense (15), poison
(11-difficulté). Les tours suivants suivent le même calendrier.
La fuite tire pendant son menu confirmé ; la récompense (50) vient en fin.

**C_4473 choisit une phrase avec RANDOM(3)** : même si le frontend change,
ce tirage doit rester. T sans arme de tir saute cette phrase et son tirage.
Les phrases viennent des entrées de même numéro de MESSAGE, MESSAGE2 ou
MESSAGE3 (enregistrements de 40 octets). AFCC conserve l'objet souligné,
sauf armure 21 ; C_8B5E et la sortie non-F de C_8CE3 le remettent à zéro.
Ces nettoyages sont comparés au binaire. Aucun retrait d'objet n'apparaît.

## Fuite et cartes — C_86E1

F annulé n'a aucun effet sur le lieu, les drapeaux ou la graine.
Après confirmation :

- Monde 0..10 : candidat = `monde*100 + RANDOM(90)+5`, répéter tant que
  **la septième colonne de la carte fournie** est nulle. Un rejet consomme
  un tirage ; il ne faut pas tirer parmi une liste préfiltrée.
  9B72 reçoit candidat-1 et Position reçoit le candidat accepté.
- Monde 11, OnAlfol vrai : `RANDOM(99)+1` ; sinon `3*RANDOM(255)`.
  9B72 reste inchangé dans ces deux branches.
- Fuite et AFD1 sont posés. Les personnages suivants n'ont plus de menu,
  mais le monstre peut encore riposter si le groupe avait l'initiative.
  Fin de tour, HS, maxima et test de récompense restent exécutés.

L'import du bloc original tSavedata utilise le tableau de 7 700 octets à
75EE et extrait `carte[6::7]`. Il ne devine pas le rôle général de cette
colonne. WorldId, OnAlfol, 9B72 et RandSeed sont hors de ce record : ils
sont fournis explicitement. Aucune carte de partie n'est fabriquée en
mode `--state` ou `--savedata`.

## Limites explicites de l'intégration

- **MODERNISABLE** : clavier BIOS, CGA, haut-parleur et délais sont
  remplacés par un frontend terminal. L'affichage CGA et les sons ne sont
  pas certifiés par ce lot ; les textes d'action sont lus dans les originaux.
- L'API accepte jusqu’à dix personnages aptes (quatre emplacements d’action), 22 types de monstre, difficulté 0..10,
  variance HP positive. Les données invalides sont rejetées sans mutation.
- Une carte de monde 0..10 dont toutes les cases 5..94 ont la colonne 7
  nulle ferait boucler le COM indéfiniment. Le moteur signale ERROR à la
  confirmation de F, sans fausse fuite ni tirage. Ce garde-fou de contrat
  concerne une entrée incohérente ; il ne change aucune carte admissible.
- Une session attend indéfiniment une touche sans tirage supplémentaire.
  La fermeture du frontend signale une interruption ; elle n'exporte pas
  une victoire fictive. La sauvegarde d'une session en cours n'est pas fournie.
- Le frontend réintègre les sorties dans un état JSON moderne. Il ne
  réécrit pas une disquette DOS. La création de partie, la génération des
  cartes et la boucle d'exploration du jeu complet restent des lots distincts.
- `--demo` utilise un profil et des cartes synthétiques annoncés comme
  banc manuel. Il sert à jouer ce bloc isolé, pas à créer une partie fidèle.

## Vérification exécutée

`tools/compare_interactive_binary.py` exécute C_838A jusqu'à sa sortie,
avec le menu, la confirmation, le mot et le **véritable RNG** du COM.
Les mêmes touches et graines sont envoyées au port C. Les états sont
comparés avant chaque lecture et à la fin, y compris AFCC.

Campagne : **720 rencontres, 4 934 frontières clavier, 770 cas de RNG,
aucun écart**,
22 monstres, 1..10 membres, les deux priorités, tous les choix, annulations,
mots vides/corrects/incorrects, retour arrière, troncature, les douze mondes,
les deux branches Alfol et 1 932 tirages de fuite rejetés.
718 rencontres atteignent une fin ; deux scénarios s'arrêtent volontairement
à la fin de leurs entrées et restent comparés jusqu'à cette frontière.
Les données originales restent locales et les rapports sous `build/audit/`.

Les lots monde/recrutement étendent la validation aux groupes >4 et au
drapeau AFCE partagé ; voir `spec_recrutement.md` pour le CASE de cible
absent et le calcul d’agilité avec un effectif supérieur à quatre.

## Révision lors des lots cité (11 et 12)

Les chiffres ci-dessus décrivent une campagne antérieure. Son oracle
abstrayait aussi des effets contenant RANDOM et initialisait SoundFlag=0.
Ce périmètre ne suffisait pas à certifier les tirages de présentation.
Les routines de couleur et de son sont désormais conservées dans le
port et exécutées dans l'oracle : flash J, riposte ≥15, sorts, coups et
musique de victoire. Voir `spec_navigation_cite.md` pour leurs séquences.

Campagne actuelle : **946 combats, 6 836 frontières, 770 vecteurs RNG,
aucun écart** ; 154 scénarios supplémentaires couvrent tous les types et
actions avec SoundFlag=1. 944 atteignent une fin, deux s'interrompent
volontairement à une frontière. Cette campagne remplace l'ancienne pour
le combat interactif. Les autres comparaisons numériques restent des
tests des formules, distincts de la chronologie complète des tirages.
