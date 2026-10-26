# Dialogues résidents et succession — lots 15 et 16

## Sources et périmètre

**CONFIRMÉ — BIN/PAS/DATA** : TERA.COM CAFE8/CAFEF, C4473,
C6073, CB6D4 ; OVL00CB branche défaite ; OVL0022 ; boucle TERAMAIN
relogée DBAC..DBE2 et E790..E7C0. Les fichiers MESSAGE/MESSAGE2/MESSAGE3
fournissent les réponses, les indices proviennent du monde généré.
`tools/audit_dialogue_lifecycle_binary.py` capture ces régions et les
empreintes sous `build/audit/`, exclu de Git. Aucun original redistribué.

## Négociation N et séduction S

**CONFIRMÉ — BIN** : N ouvre le menu I/A/V/D/S. Un choix inconnu ou ESC
sort ; O/Entrée confirme un choix, toute autre touche l'annule. Aucun jour
n'est ajouté. Les touches invalides du transfert gardent les règles CAD08.

- **I, informations** : réinitialise AFCE. Si A750 est vrai, le dernier
  ID1..9 dont ExtraLoc égale Position fournit les indices 2×ID−1 et 2×ID.
  Sans correspondant, afficher « ... ». Puis rechercher les compagnons
  présents, inactifs et non-HS ; leur présence active AFCE. RANDOM(2)=0
  ajoute les indices 2×RANDOM(9)+1 et le suivant ; sinon afficher MESSAGE46.
  Aucun compagnon n'est recruté par cette action.
- **A/V, acheter/vendre** : réponses C4473 des trois fichiers MESSAGE*,
  ID22/23, avec RANDOM(3). Aucune transaction dans ce résident ; les
  boutiques et leurs transactions appartiennent aux autres lieux.
- **D, donner** : confirmation déjà consommée, source A-D, slot 0..9,
  cible A-D ; délègue au transfert audité, sans deuxième confirmation.
- **S dans N, solliciter** : recherche les compagnons disponibles et fixe
  AFCE, sans tirage ni recrutement. **S direct, séduire** réutilise la
  négociation/recrutement audités, en conservant AFCE entre les commandes.

**CONFIRMÉ — BIN** : A750 est un contexte de dessin, distinct de la
faculté mentale 9418. C607D le met à vrai. Les présentations de PNJ
d'Amarande, compagnons 1/2/6 de la cité et monstres des rencontres le
propagent au moteur et au JSON. N/I lit ce contexte, sans le remplacer
par une déduction de proximité ou par le drapeau mental.

Une séduction négociée réussie joue C6518 si SoundFlag : ses tirages
RANDOM sont conservés après les dix jets de négociation. L'ancien banc
isolé de recrutement ne transportait pas SoundFlag ; ses résultats
restent une validation du cas sans son, pas de cette mélodie. Le nouveau
banc de dialogue couvre la commande entière avec son.

## Mort, condoléances et successeur

**CONFIRMÉ — BIN/PAS** : après perte de PartyAvailable, la cité revient
à la surface avec la position de sauvegarde. OVL00CB affiche les
condoléances. Si le son est actif, C6496, C6518, C6518, C6496 consomment
successivement leur calendrier RANDOM original. Une touche acquitte la
pause ; le matériel BIOS/vidéo et les délais sont abstraits.

TERAMAIN termine si les quatre objectifs sont déjà accomplis, même
avec un groupe mort : pas de successeur dans ce cas. Sinon il rappelle
OVL0022 pour créer le héros, sans OVL0000/0010 ni RANDOMIZE.

OVL0022 conserve les équipements de tous les personnages, leurs profils
compagnons, les cartes, lieux, mots, indices, aides, clés des compagnons,
cartes acquises et objectifs. Il recrée le profil/nom du héros, remet ses
compétences à zéro, restaure les maxima/HP/MP et états OK de tous,
désactive les compagnons, remet XP/niveau à zéro, DummyLevel à 1,
Days à 7300 et l'argent à au moins 500. La clé du héros devient zéro.
Le flag actif du héros conserve le comportement d'OVL0022.

Le héros repart à Position=1000, WorldId=11 et A74F vrai. TERAMAIN
efface A748 après la création. Le frontend exécute ensuite l'arrivée
automatique déjà auditée, avec le calendrier et les rencontres habituels.
Il ne restaure pas l'ancienne graine et ne génère pas un autre monde.

## Validation et limites

`compare_dialogue_binary.py` : **466 cas, 1 280 frontières, 80 905 appels
au RNG original, zéro écart**, y compris arrivée sur PNJ → N/I, contextes,
indices effectivement lus, réponses A/V, annulations, sollicitation,
séduction, paiement et transferts. Les indices et variantes de réponse
sont observés aux adresses de chaînes originales, sans oracle de règles
Python. L'état et la graine sont comparés à chaque touche et au retour.

Les campagnes de navigation (**887 cas**), exploration (**854 cas**) et
entrée/retour (**51 cas**) sont relancées sans écart avec A750 inclus.
La navigation couvre aussi les sprites de compagnons et l'invocation
d'Arioch, dont la présentation appelle C6073 comme les autres monstres.

`compare_lifecycle_binary.py` : **120 cas, 1 911 frontières, 75 092 appels
au RNG original, zéro écart**. Exécute les deux overlays et les branches
TERAMAIN, avec les seize combinaisons d'objectifs, son actif/coupé,
interruption, recréation refusée puis acceptée, argent et profils variés.
Le chargement DOS, la pause BIOS et les cadres texte sont abstraits ;
ni les créations, ni la condition de fin, ni la musique/RNG ne sont simulés.

**11 suites C et 33 tests frontend passent** : dialogues reliés,
contexte partagé, annulations, réponses sans transaction, décès en cité,
successeur, conservation du monde/butin/progression, arrivée,
export/rechargement et interruption sans publication. Le fichier source
reste inchangé. Q exporte seulement une opération terminée.

**MANQUANT** : transactions et événements des autres lieux, mondes 9/10,
Alfol/Meduz, voyages, autres objectifs et fin victorieuse avec score.
La succession est reliée à Amarande et à la cité déjà portées ; elle
ne déclare pas jouables les lieux non portés. **MODERNISABLE** : affichage
graphique et audio audible encore abstraits. **À VÉRIFIER** : animations
interrompues au clavier. Une partie entière reste à valider.
