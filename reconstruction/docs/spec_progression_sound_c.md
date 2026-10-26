# Sons de progression

CONFIRMÉ PAS/BIN : les quatre objectifs jouent C6496(9,8),
C6518(1,9,8), puis C6518(2,9,8). Le recrutement négocié joue
C6518(1,9,8), confirmé au résident B872..B87E.
Les anciens appels RNG muets sont remplacés, jamais doublés.

CONFIRMÉ BIN : C61D1(bound,delay,gap,count) joue count fréquences
10*RANDOM(bound), chacune pendant delay ms, puis un silence gap ms.
C68F4 (entrée Méduz) appelle cette routine pour bound=10..250 par
pas de 10, delay=10, gap=0, count=10.
C693B (refus Alfol) : C65B5(100), SOUND(165)/250 ms,
C65B5(100), C6179(250,20,5,15).

CONFIRMÉ PAS : pièges gazeux/aiguilles C6121(50,1500,10,20),
explosion C6D32 (huit couples de couleurs et C61D1(100,2,0,16)),
apparition des gardiens de Méduz C6221(3000,30,100).
Ces effets gardent leur position par rapport aux blessures et aux combats.

MODERNISABLE : synthèse SDL du haut-parleur ; les écritures sans DELAY
conservent la convention existante du lecteur audio, sans nouvelle promesse
de timbre ou de durée physique PC/XT.

## Vérification

Sept partitions supplémentaires sont comparées à SOUND/DELAY exécutés dans
TERA.COM (empreinte contrôlée) par `tools/audit_progression_sound.py`.
Les vecteurs indépendants conservés en C vérifient toutes les fréquences,
les délais, le nombre de notes et la graine finale :

| Partition | Notes | Délais explicites cumulés |
| --- | ---: | ---: |
| Objectif (trois mélodies) | 621 | 9315 ms |
| Recrutement négocié | 207 | 3105 ms |
| Entrée Méduz | 250 | 2500 ms |
| Refus Alfol | 248 | 561 ms |
| Piège, balayage | 146 | 1378 ms |
| Son d'une image d'explosion | 16 | 32 ms |
| Gardiens Méduz | 100 | 3000 ms |

Les durées du tableau excluent le coût matériel des instructions sans DELAY.
Les tests couvrent aussi objectif unique, mode muet, présence/absence de
lecteur, graine inchangée par le lecteur et passage en PCM SDL (9315 ms
pour l'objectif). Le WAV de diagnostic est généré dans le dossier de test.
Les pauses originales de 2000 ms sur Alfol sans son et de 1000 ms avant
le tunnel Méduz sans son sont également raccordées.

Validation : 37/37 suites CTest réussies, dont le paquet autonome reconstruit.
Comparaisons des états et RNG avec le programme original : Alfol 613 cas,
Méduz 326 cas plus 48 trajets, dialogues 466 cas, navigation 887 cas,
inventaire 632 cas : aucun écart. Ces bancs abstraient l'affichage/BIOS ;
la preuve sonore distincte est celle des sept partitions ci-dessus.
