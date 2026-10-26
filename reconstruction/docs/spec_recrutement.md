# Sollicitation et recrutement — lots 7 et 8

## Périmètre

**CONFIRMÉ — BIN/DATA** : `engine/src/recruitment.c` porte la recherche
C_A074, solliciter (verbe 25, commande S de AFEF, B215) et séduire
(verbe 14, commande S de AFE8, B6D4). O/o/Entrée confirme ; les autres
touches annulent. Le frontend terminal charge VERBES et les profils
générés ; les annonces de résultat sont une présentation moderne.

Un compagnon est disponible s'il est à la position courante, inactif
et d'état différent de HS. Aucun test HP supplémentaire n'est ajouté.
Solliciter cherche tous les disponibles et renseigne AFCE ; aucun tirage,
paiement ni activation. AFCE est partagé avec le combat : chaque premier
menu l'efface, une première annulation le positionne pour la seconde
tentative. Les exports conservent cette donnée entre interactions.

## Séduire et conséquences

Sans AFCE, une phrase consomme RANDOM(3), puis la commande termine.
Avec AFCE, la routine l'efface et compte 1 + les compagnons actifs, HS
compris. Si ce nombre est au moins quatre, elle termine sans jet.

Sinon, elle effectue **dix** jets C_8315(charme, négociation), y compris
pour les inactifs et les HS. Un actif non-HS réussit si son résultat est
positif **ou** si la position est celle du personnage 8. Le dernier
réussissant reste le négociateur retenu, sans interrompre les tirages.

Réussite : tous les disponibles rejoignent, XP += 5 avec débordement
Integer 16 bits. Échec : si argent > 249, retirer 250 et activer tous
les compagnons à cette position, **HS inclus** ; sinon aucun paiement.
AFCE reste effacé après ces branches. Une annulation préserve AFCE et
la graine. Le moteur ne remplace aucune de ces particularités.

## Plus de quatre membres

**CONFIRMÉ — BIN** : le contrôle précède l'activation de toutes les
recrues. Des compagnons co-localisés peuvent donc porter le groupe
au-delà de quatre membres. Le combat original compte tous les aptes,
mais mémorise seulement quatre IDs. Seuls ces quatre personnages
régénèrent, agissent et contribuent à la somme d'agilité ; celle-ci
est divisée par l'effectif total. La normalisation de fin de tour ne
porte que sur ces quatre IDs.

La cible de riposte est RANDOM(effectif total). Les résultats au-delà
des quatre emplacements ne lancent **aucune attaque**, aucun jet de
défense ou de poison. Le port reproduit ce CASE absent, sans nouveau
tirage ni plafond de recrutement ajouté. Les tests différentiels de
tour et de combat interactif couvrent désormais les groupes 5 à 10.

## Banc et limites

`frontend/recruitment.py --state ...` utilise la position enregistrée.
`--at` impose un lieu explicitement annoncé comme banc d'audit ; il
ne simule pas les déplacements ou la détection d'une rencontre.
`--request` sélectionne solliciter, séduire ou les deux, puis le frontend
exporte l'état. Un état de monde avec création du héros terminée est
requis. Le fichier source reste inchangé.

`tools/compare_recruitment_binary.py` exécute C_A074, AFEF/AFE8,
C_8315 et le RNG originaux : 696 scénarios, aucun écart. Il couvre
annulations, confirmations, groupe complet, HP/statuts, seuils d'argent,
lieu du personnage 8, négociation, paiement et débordements d'XP.
La présentation et le clavier BIOS sont abstraits.

Le lot 15 relie N/S aux dialogues de l'exploration et ajoute le calendrier
sonore de la séduction réussie : voir `spec_dialogues_succession.md`.
Le banc isolé décrit ci-dessus ne transporte pas le son et valide le cas
sans son. **MANQUANT** : autres lieux/interactions et rendu graphique.
