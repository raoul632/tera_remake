# Police, carte et tunnels : référence IBM PC/XT

Référence demandée : IBM PC/XT, 8088 à 4,77 MHz, CGA.

CONFIRMÉ (BIN C5495) : A744 est effacé à 5B80 pour chaque case,
avant la recherche des vingt villes. Il n'y a pas de résidu à conserver.
GRAPH inverse Y à son entrée puis lors du calcul CGA : les deux inversions
s’annulent et l’origine visible reste en haut, comme pour C4544 et le texte.
C4544 a une borne droite en octets exclusive, et traite les deux lignes de
chaque paire. Ces conventions doivent rester distinctes, même là où le
programme original les mélange pour les marqueurs Alfol.

CONFIRMÉ (BIN) : Circle utilise l'itération entière Newton de GRAPH, avec
symétries et clipping ; un cercle SDL de Bresenham n'est pas son oracle.
La comparaison doit exécuter le pilote original et lire B800, pas seulement
collecter les appels aux primitives. Les fixtures de comparaison sont
synthétiques et ne redistribuent pas les cartes originales.

CONFIRMÉ (référence matérielle) : police bitmap CP437 complète IBM BIOS en
graphique, CGA en texte, bit fort à gauche ; cadres en glyphes CP437. Voir
le fichier NOTICE des tables pour les sources et empreintes exactes.

La cadence doit tenir compte du travail de dessin des cercles et effacements
sur 8088, et conserver sa fraction de temps entre images/audio. Le PAS n'a
aucun DELAY ni synchronisation au rafraîchissement dans cette boucle :
une cadence constante de 16 ms ne reproduit pas la référence demandée.
Le modèle de cycles nominal ne doit pas être présenté comme une mesure
physique des attentes de bus/prélecture d'un exemplaire matériel.

## Implémentation et preuves

- `text.c` utilise les 256 glyphes BIOS ou CGA directement en CP437, sans
  passage Latin-1. Les 80 colonnes gardent le même viewport que les 40 :
  rendu logique 640 × 400 avec hauteur doublée des lignes 8 × 8. Les cadres
  de création et de fin utilisent les vrais caractères de bordure.
- `audit_cga_native.py` exécute le pilote GRAPH, les calculs de clipping et
  les écritures de B800. Les tests C comparent les empreintes de **44 cartes**,
  **6 cercles** et **245 images complètes de tunnel**. L'état initial A744
  est testé à 0 et à 1. Tous les pixels indexés correspondent dans ces cas.
- `audit_xt_timing.py` compte les coûts nominaux des instructions réellement
  exécutées, avec calcul d'adresse effective, transferts de mots sur bus
  8 bits, branches et décalages. Il génère les coûts des 100 rayons, des
  chemins de clipping, et des 16 variantes palette/fond/effacement/son.
  La préparation inclut les étoiles et un coût nominal de nettoyage CGA.
  Référence des coûts : [Intel, 8086 Family User's Manual (1979)](https://www.inf.pucrs.br/~calazans/undergrad/orgcomp_EC/mat_microproc/intel-8086_family_Users_Manual.pdf).
- Les échantillons audio et les repères vidéo partagent la même conversion
  entière cycles / 4 772 727 Hz. Le reste est conservé entre images ; aucun
  arrondi à la milliseconde ni délai fixe de 16 ms. La graine 1986 donne
  **21,017 s** en silence pour les 245 images avec ce profil nominal.
  Le compteur indépendant sur l'exécution de la séquence native entière
  donne **21,024 s** dans le même modèle : le test impose un écart inférieur
  à 0,1 % pour la factorisation des coûts employée par le lecteur.

Les tests vérifient les deux tables de 2 048 octets par empreinte indépendante,
les pixels d'un accent et d'une bordure, et la sélection d'image à ses bornes.
Ils passent dans le paquet autonome sans ROM, COM ou Python à l'exécution.

**Limite explicite du profil XT** : MUL/DIV prennent le haut de leur plage
nominale ; la contention CGA, la prélecture du 8088, le refresh DMA et le code
interne d'une ROM BIOS particulière ne sont pas simulés. Les 21,017 s sont
une durée modélisée reproductible, pas un chronométrage sur un PC physique.

Validation finale : **37/37 suites CTest réussies**, compilation avec les
avertissements traités en erreurs, captures SDL inspectées et paquet autonome
reconstruit. Aucun changement de règles, de graine ni de format de sauvegarde.
