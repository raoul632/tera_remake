# Création du héros et initialisation des compagnons — lot 6

Audit du 14 octobre 2026. **CONFIRMÉ BIN/PAS/DATA + exécution différentielle**
pour les règles décrites ici. Le Python historique n'est pas un oracle.
Moteur : `engine/src/creation.c`, API `tera/creation.h`. Frontend terminal :
`frontend/creation.py`, transport `frontend/creation_bridge.c`.

## Sources et périmètre

- COM SHA256 `22fb836aa93f46dc2437e8b9586415d13552bfac35e8ac6647ba6157fd9b1a37`.
- TERA.000 SHA256 `539d95781f9296d40ecd05698eb98255835a45cb03e60b49cf581e3ada7038f4`.
- OVL_0010 : octets `TERA.000[0x1000:0x2200]`, origine BA21.
  Bloc personnages C597–C845, équipement CB91–CBC1.
- OVL_0022 : octets `TERA.000[0x2200:0x2E00]`, origine BA21,
  véritable entrée BA28, corps BBDF–C54C. Les octets BA21–BA27 sont le
  descripteur/chargeur overlay ; le banc n'exécute pas ce chargeur DOS.
- PAS : OVL_0010, OVL_0022, OVL_00DB, TERAMAIN et record tSavedata.
- DATA : NOMPERS, 27 chaînes Pascal de dix octets chacune, texte CP437.

L'overlay 0010 alterne création de personnages et génération géographique.
Ce lot porte ses régions personnages, **pas l'overlay géographique entier**.
L'équipement final, déterministe et sans tirage, peut être appliqué avec
le roster. Le monde, les placements, indices, aides H et mots secrets
restent à générer dans le lot mondial. Leur calendrier RNG intermédiaire
n'est pas remplacé par des valeurs prétendument originales.

L'overlay de création du héros 0022 est porté entièrement pour ses règles,
ses menus et ses effets sur l'état. Il peut être utilisé après initialisation
d'un monde ou après la mort du héros, sans effacer ce monde.

## Neuf compagnons — CONFIRMÉ

`tera_characters_initialize` reçoit les noms déjà chargés et leurs variantes.
OVL_00DB lit d'abord les neuf noms de base de NOMPERS, puis les neuf variantes
du groupe 0 et les neuf du groupe 1. L'ID 0 est le héros, hors de ces groupes.

1. Effacer tous les équipements, caractéristiques et compétences.
2. IDs 1..9 : RANDOM(3) dans l'ordre des IDs. 0 conserve le nom courant,
   1 copie la variante du groupe 0, 2 copie celle du groupe 1.
3. État OK pour tous ; seul le héros est actif. Compétence `[id][id]=3`.
4. Pour chaque ID 1..9, tirer neuf valeurs `2+RANDOM(6)` (colonnes 1..9),
   puis écraser la caractéristique `[id][id]` par 9. Le tirage écrasé
   demeure consommé. Total : **90 tirages**, neuf noms puis 81 caractéristiques.
5. Les exceptions suivantes écrasent une valeur par 7 et ajoutent une
   compétence au niveau 1, sans modifier la spécialité principale :

| Compagnon | Caractéristique et compétence supplémentaires |
|---:|---|
| 3 | Volonté, colonne 5 |
| 6 | Intelligence/magie, colonne 2 |
| 8 | Volonté, colonne 5 |
| 7 | Habileté/tir, colonne 9 |
| 4 | Habileté/tir, colonne 9 |

Les équipements de fin d'overlay sont personnels :

| ID | Objets dans les emplacements 0 et 1 |
|---:|---|
| 1 | 13 épée, 20 cuirasse |
| 4 | 19 cuir, 17 revolver |
| 7 | 19 cuir, 17 revolver |
| 9 | 21 armure spatiale, 18 désintégrateur |

Le héros et les autres compagnons n'ont pas d'équipement ajouté par ce bloc.
L'initialisation du roster ne remplit pas encore leurs HP/MP ; cela vient
après validation du héros. Elle ne recrute aucun compagnon : recrutement,
lieux de rencontre et conditions de dialogue restent dans les interactions.

## Création du héros — CONFIRMÉ

`tera_creation_start` copie le roster fourni et attend le nom, sans tirage.
`tera_creation_key` reçoit une touche et avance jusqu'à la prochaine saisie.
L'attente peut durer indéfiniment sans changer la graine.

### Nom et genre

La lecture du nom est la même C_42AC que celle du mot de combat : lettres
ASCII A..Z, minuscules converties, retour arrière, Entrée, arrêt automatique
à dix lettres puis stockage des neuf premières. Les chiffres/espaces et
autres touches sont ignorés. Le nom vide est accepté par le binaire.

Le genre lit une touche, sans validation H/F. **Seul F** choisit le bonus
féminin ; toute autre touche choisit le bonus masculin. Le genre n'est pas
un champ persistant du record original. Son effet statistique suffit au moteur.

### Qualités principales et répartition

```
Force = RANDOM(5)+5
Intelligence = RANDOM(3)+3 si Force>7, sinon RANDOM(6)+4
Chance = 18-Force-Intelligence
```

Ces trois valeurs totalisent 18 avant les bonus. Le joueur répartit :

| Primaire | Valeur saisie | Reste calculé |
|---|---|---|
| Force | Endurance | Agilité = Force-Endurance |
| Intelligence | Volonté | Charme = Intelligence-Volonté |
| Chance | Vitalité | Habileté = Chance-Vitalité |

C_4418 lit un chiffre 0..9 ; une autre touche redemande sans tirage.
Malgré l'invite « 1 à primaire-1 », **zéro est accepté**. Une valeur supérieure
à primaire-1 est remplacée par `RANDOM(primaire-1)+1`, et ne redemande pas
au joueur. Ce remplacement consomme un tirage à son emplacement exact.

Après les trois répartitions : F ajoute 2 au Charme ; les autres genres
ajoutent 2 à la Force. Ces bonus n'augmentent pas les qualités secondaires
déjà calculées. Ne pas recalculer les sommes après ces bonifications.

### Six points de bonus, puis refus/acceptation

Six touches valides attribuent chacune un point : F Force, I Intelligence,
N Chance, E Endurance, V Volonté, T Vitalité, A Agilité, C Charme, H Habileté.
Une touche inconnue ne consomme aucun point ni tirage. Il n'y a **aucun
plafond à 11** dans ce bloc : plusieurs points peuvent aller à la même qualité.

La dernière invite est différente d'une confirmation de combat : **seul N
refuse le personnage**. Toute autre touche l'accepte, même Entrée, espace
ou Échap. N revient au nom puis aux tirages ; la graine reste avancée.
Les noms, équipement, compagnons et caractéristiques présentes ne sont
pas effacés à ce retour. Les nouvelles saisies les remplacent au fur et à mesure.

## État final et recréation après défaite — CONFIRMÉ

OVL_0022 appelle C_4CF9 pour les dix personnages : recalcul des maxima puis
HP/MP remplis aux maxima, tous les états deviennent OK, même les absents.
HP max = Force+Endurance+Habileté ; MP max = Intelligence+Volonté.

- Argent ramené à 500 **uniquement s'il est inférieur à 500**, comparaison signée.
- IDs 1..9 exclus du groupe ; le flag actif du héros n'est pas réécrit ici.
- Toutes les compétences du héros, **colonne 0 comprise**, sont mises à zéro.
- XP = 0, niveau = 0, dummyLevel = 1, jours = 7 300.
- A745 = vrai, Position = 1 000, WorldId = 11, A74F = vrai, Keys[0] = 0.
- Équipement et compétences des compagnons conservés.
- **L'équipement du héros est conservé lors d'une recréation** : c'est
  l'initialisation 0010 d'une partie neuve qui l'efface, pas 0022.
- OnAlfol n'est pas modifié par 0022 ; TERAMAIN le remet à faux après son retour.

Le moteur conserve cartes, H, mots, données monstres et contexte hôte.
`tera_creation_apply_combat` réintègre personnages et champs modifiés par
la création dans un `TeraCombatGame` existant, sans remplacer ses cartes.
Les champs jours, clés, niveau et A74F restent dans `TeraCharacterState`
pour la future boucle générale. Ils ne sont pas inventés par le combat.

## Frontends et utilisation

```powershell
python reconstruction/frontend/creation.py --seed 1986
python reconstruction/frontend/combat.py --demo --characters reconstruction/build/characters.json --monster 7
```

Le premier programme crée les neuf compagnons puis permet la création
interactive du héros. L'export `build/characters.json` est un roster moderne
marqué `characters_only`, **sans cartes ou mots secrets fabriqués**.
Sans `--seed`, l'heure locale est encodée selon RANDOMIZE. Cette graine est
une graine d'entrée au bloc personnages ; elle ne prétend pas rejouer la
génération mondiale antérieure et intermédiaire d'une partie entière.

Le deuxième programme utilise ces personnages dans un **contexte de combat
de test annoncé**, ou accepte `--state` pour préserver un véritable état
mondial fourni. Le héros conserve ses compétences nulles et son équipement
initial vide ; il n'est pas remplacé par le profil puissant de `--demo`.
Par défaut la graine du roster est conservée ; `--seed` l'écrase explicitement.

Recréation : `creation.py --roster CHEMIN_ROSTER`. Les compagnons ne sont
pas rerandomisés, les équipements sont conservés et le fichier d'entrée
n'est pas modifié. Une interruption n'exporte pas un roster terminé.

**MODERNISABLE** : écrans texte/CGA, cadres, couleurs, sons et délais.
Le frontend terminal ne certifie pas leur reproduction graphique.
**MANQUANT hors de ce lot** : generation mondiale, placement des compagnons,
indices/mots/H, recrutement, exploration et création d'une partie complète.

## Validation exécutée

`tools/compare_creation_binary.py` charge les originaux en mémoire et
exécute les instructions des régions personnages 0010, puis celles de 0022.
Les mêmes touches et graines vont au C. Aucun RANDOM ni résultat de
caractéristique n'est injecté. Comparaison des noms, statistiques,
compétences, équipements, groupe, HP/MP/maxima, états, clés, compteurs,
position, monde, disponibilité, A74F et graine à chaque lecture et à la fin.

**662 scénarios, 11 128 frontières clavier, 56 222 appels au RNG original,
aucun écart.** 564 créations terminées ; 98 traces partielles explicitement
arrêtées à la fin de leurs entrées. Cas : graines extrêmes, toutes les
touches de bonus, chiffres 0..9, hors limites, noms tronqués/vides/effacés,
genres atypiques, refus et reprises, renaissance, argent signé et profils
extrêmes. Les valeurs extrêmes testent l'arithmétique, sans affirmer leur
atteignabilité dans une partie.

Les hooks remplacent uniquement touches BIOS, affichages texte/graphique,
cadres, couleurs, sons et délais. C_4418, C_42AC, C_4C6C/C_4CF9 et le RNG
sont réellement exécutés. La limite d'instructions sans frontière attendue
fait échouer le banc. Les deux régions 0010 sont testées avec une frontière
géographique annoncée ; ce banc ne certifie pas les placements mondiaux.

Commandes : `python reconstruction/tools/audit_creation_binary.py`,
`python reconstruction/tools/compare_creation_binary.py`,
`python reconstruction/tests/test_creation_frontend.py`, puis CTest.
Preuves et rapports restent sous `build/audit/`, ignorés par Git.
