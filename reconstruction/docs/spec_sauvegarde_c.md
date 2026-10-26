# Sauvegarde et reprise C — contexte 3

## Statut et périmètre

**MODERNISABLE — cadre utilisateur** : le format de sauvegarde peut être moderne
si les effets fonctionnels restent identiques. Le format ci-dessous n'est pas
le conteneur DOS. **CONFIRMÉ — tests C** : sauvegarde/reprise à une frontière de
commande READY, sans changer les données persistantes, la graine, le calendrier,
les objectifs ou les commandes obligatoires d'un lieu. Aucune commande de jeu
supplémentaire, interface ou dépendance Python n'est ajoutée.

L'API est `engine/include/tera/save.h`, implémentée dans `engine/src/save.c`.
La reprise technique `tera_game_restore_ready` est distincte de
`tera_game_start` : celle-ci stabilise un monde et peut déclencher une arrivée,
un jour ou une succession ; la reprise restaure une frontière déjà stabilisée
et ne rejoue aucune de ces opérations.

## Format v1

Le fichier mesure **11 521 octets**, dont un en-tête de 20 octets et un payload
de 11 501 octets. Les nombres sont little-endian ; les entiers signés utilisent
le complément à deux. Aucun padding ni pointeur C n'est copié sur disque.

| Offset | Taille | Contenu |
| --- | --- | --- |
| 0 | 8 | Magic : `TERASAV` suivi de zéro |
| 8 | 4 | Version : 1 |
| 12 | 4 | Longueur du payload : 11 501 |
| 16 | 4 | CRC32 du payload, polynôme réfléchi EDB88320, initial/final FFFFFFFF |
| 20 | 11 501 | Champs décrits ci-dessous |

L'ordre exact des champs est la fonction `payload` de `save.c`, utilisée dans
les deux sens. Les int16_t occupent deux octets ; int et unsigned persistants
occupent quatre octets, sans dépendre de leur taille native. La graine occupe
quatre octets. Les tableaux d'octets et chaînes CP437 gardent leur taille fixe.
Toute modification de cet ordre, de la taille ou de l'interprétation nécessite
une nouvelle version et une décision explicite de migration.

Dans l'ordre sont encodés :

1. Les dix acteurs et leurs caractéristiques/compétences, les données de rencontre.
2. Les valeurs des monstres, équipements, aides, mot et colonne 7 des cartes,
   contexte de combat/exploration et graine.
3. Noms, clés, jours, niveaux, énergie et drapeau d'arrivée des personnages.
4. Cartes complètes, villes, placements, montagnes, positions cachées, indices,
   mots, cartes possédées, objectifs et globals du monde.
5. Globals d'exploration et contexte TERAMAIN, dont positions de retour, pirates,
   variante de PNJ et commande obligatoire.

Les valeurs signées originales, y compris les débordements, sont conservées.
La sérialisation n'invente pas de plafond de HP, argent, statistiques ou jours.
Les chaînes doivent être terminées ; les drapeaux et indices de routage sont
contrôlés. Les données des ressources restent locales ; les sauvegardes générées
en contiennent et ne sont pas des fichiers à distribuer avec le dépôt.

## Contrat d'utilisation

`tera_save_encode` produit un buffer canonique ; si un argument, l'état ou la
capacité est invalide, ni le buffer de destination ni sa taille ne sont publiés.
`tera_save_decode` vérifie magic, version, longueur exacte, CRC et données avant
de remplacer la session. Un rejet laisse la session entière inchangée. Le CRC
détecte des erreurs accidentelles et n'authentifie pas un fichier hostile.

Seule une session READY sans opération en cours est enregistrable. Combat,
création, confirmations, saisies, HALT, fin et erreur sont refusés. Une arrivée,
une succession ou une fin automatique qui reste à traiter n'est pas une frontière
READY valide. En revanche, l'attente d'une commande obligatoire après une arrivée
mortelle à Seth, dans les bâtiments d'Alfol ou sur Meduz est conservée.

La session restaurée ne contient pas l'ancienne union active, le dernier rapport
de mouvement ni les callbacks du processus précédent. Ces champs sont transitoires.
L'appelant fournit de nouveaux services d'entrée et d'observation ; la reprise ne
les appelle pas. Les futures touches reprennent le routage normal du moteur.

`tera_save_write` et `tera_save_read` utilisent des flux C binaires appartenant
à l'appelant. L'écriture vérifie `fwrite`, `fflush` et les erreurs du flux.
La lecture est bornée, exige un fichier unique sans suffixe et ne publie aucun
état partiel. Le test ferme et rouvre réellement un fichier avant reprise.

**Limite de stockage explicite** : ces fonctions ne publient pas atomiquement
un chemin et ne promettent pas une résistance à une coupure électrique. L'hôte
doit écrire dans un nouveau fichier temporaire, vérifier aussi sa fermeture,
puis publier/remplacer le fichier selon les garanties de sa plateforme. Une
erreur d'écriture peut laisser un flux incomplet. Cette responsabilité du futur
hôte SDL n'ajoute aucune règle de jeu ; le moteur reste portable C99.

## Validation

`save_resume_contracts` couvre :

- le format canonique, ses dimensions, les octets CP437 et valeurs extrêmes ;
- l'absence d'appel aux services et de tirage pendant sauvegarde/reprise ;
- chaque longueur tronquée, les suffixes, magic/version/longueur/CRC invalides ;
- des données sémantiques invalides avec CRC recalculé, refus sans mutation ;
- la capacité insuffisante et le refus des opérations en cours ;
- les douze identifiants de monde, les trois commandes obligatoires et les
  transitions automatiques à ne pas rejouer ;
- un fichier fermé/rouvert, une erreur d'écriture sur flux en lecture seule.

Les deux campagnes `campaign_resume_1986` et `campaign_resume_12345` sauvegardent
dans un flux temporaire, restaurent puis poursuivent les commandes réelles.
Chaque champ persistant est réencodé et comparé. Les callbacks sont reconnectés.
Le conducteur sauvegarde à la première frontière READY de chaque monde et
périodiquement ; il ne recharge jamais une partie antérieure après une défaite.

| Graine | Reprises | Mondes | Entrées | Successions | Trace finale | Score |
| --- | --- | --- | --- | --- | --- | --- |
| 1986 | 34 | 12 | 3 886 | 5 | 2d0ca4bb | 8 |
| 12345 | 30 | 12 | 3 257 | 3 | b246cd73 | 10 |

Valeurs actualisées après les corrections de fidélité détaillées dans
`validation_teramain_dos.md`. Les reprises gardent les nouvelles traces.

Ces traces sont identiques aux campagnes sans sauvegarde. Elles prouvent cette
intégration C pour les deux graines, difficulté zéro et son désactivé ; elles
ne constituent pas une nouvelle comparaison de la partie entière au DOS.

La suite dédiée passe également dans une configuration GCC UCRT64 séparée,
avec le même contrat de taille du format, en complément de GCC MinGW32.

```powershell
cmake --build reconstruction/build/cmake --target test_save_c test_campaign_c
ctest --test-dir reconstruction/build/cmake -R 'save_resume_contracts|campaign_resume' --output-on-failure
```
