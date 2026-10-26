# Cadre général du projet TERA

## Objectif du projet

L'objectif est de reconstruire **TERA (Loriciels, 1986)** sous la forme d'un **prototype moderne jouable et fidèle à l'original**.

Le but n'est PAS de créer un remake amélioré, une nouvelle version du jeu ou une réinterprétation moderne.

Le résultat recherché est :

    TERA original
          ≈
    prototype reconstruit

du point de vue du joueur et des règles du jeu.

Nous voulons reproduire aussi précisément que possible :

- les règles ;
- les personnages ;
- les statistiques ;
- les compétences ;
- les déplacements ;
- les mondes ;
- les donjons ;
- les combats ;
- les monstres ;
- les objets ;
- les coffres ;
- les pièges ;
- les PNJ ;
- les villes ;
- les interactions ;
- les événements ;
- la progression ;
- les objectifs ;
- les conditions de victoire et de défaite ;
- le comportement observable du jeu original.

Une fois ce prototype fidèle et jouable obtenu, le travail est considéré comme terminé.

Ne pas prolonger automatiquement le projet vers un remake moderne.

---

# Ce projet n'est PAS un remake étendu

Ne jamais ajouter spontanément :

- nouvelles mécaniques ;
- nouveaux objets ;
- nouveaux monstres ;
- nouveaux PNJ ;
- nouvelles zones ;
- nouvelles quêtes ;
- nouvel équilibrage ;
- progression supplémentaire ;
- améliorations de gameplay ;
- fonctionnalités de confort ;
- comportements inventés ;
- contenu destiné à "compléter" ce qui semble manquer.

Si une mécanique originale n'est pas comprise :

    ne pas inventer

La classer explicitement :

    MANQUANT

ou :

    PROBABLE

et poursuivre le reverse engineering si cette mécanique est nécessaire au prototype.

---

# Fidélité avant amélioration

Si un comportement étrange est confirmé dans TERA original, il doit être conservé.

Ne pas le modifier simplement parce qu'il semble :

- mal conçu ;
- déséquilibré ;
- inefficace ;
- étrange ;
- peu ergonomique ;
- incohérent avec les standards modernes.

Cela inclut notamment :

- formules inhabituelles ;
- divisions entières ;
- probabilités étranges ;
- constantes d'équilibrage ;
- limitations ;
- comportements de déplacement ;
- règles de combat ;
- événements surprenants.

Le but n'est pas de corriger TERA.

Le but est :

    comprendre TERA
          ↓
    reproduire TERA

---

# Statut réel du Python actuel

Le remake Python actuel est **loin d'être fonctionnel et complet**.

Il ne doit PAS être considéré comme :

- une spécification du jeu ;
- une implémentation de référence ;
- une preuve de fidélité ;
- une base fiable pour un futur port C ;
- une reconstruction complète du moteur.

Le Python existant contient un mélange de :

- fonctions fidèlement reconstruites ;
- portions approchées ;
- hypothèses ;
- placeholders ;
- comportements provisoires ;
- fonctionnalités absentes ;
- simplifications ;
- modernisations.

Par conséquent :

    Python ≠ vérité originale

et :

    Python ≠ spécification fonctionnelle globale

Le Python doit être considéré uniquement comme :

    une reconstruction partielle
             +
    un dépôt des travaux précédents
             +
    un ensemble d'hypothèses déjà codées
             +
    un outil éventuel pour tester certaines fonctions isolées

---

# Vérification obligatoire du Python

Chaque fonction Python utilisée doit pouvoir répondre à la question :

    "D'où vient ce comportement ?"

Identifier la provenance :

- Pascal ?
- désassemblage x86 ?
- fichier de données ?
- manuel ?
- comportement observé ?
- hypothèse d'une session précédente ?

Une fonction Python qui ne possède pas de provenance suffisamment fiable ne doit jamais devenir automatiquement une référence.

Avant de conserver ou de porter une fonction Python :

    Python
       ↓
    provenance
       ↓
    validation
       ↓
    classification
       ↓
    conservation éventuelle

---

# Ordre de confiance des sources

Utiliser plusieurs sources lorsque cela est possible.

Les principales catégories de preuve sont :

    BIN      = TERA.COM / désassemblage
    PAS      = Pascal décompilé
    DATA     = données originales
    MANUAL   = manuel original
    OBS      = comportement observé dans TERA.COM
    PY       = Python existant
    INFERENCE = déduction

Le Python seul ne constitue jamais une preuve suffisante de fidélité.

Le Pascal lui-même n'est pas une source complète : les fichiers décompilés sont incomplets et plusieurs points d'entrée ou fonctions résidentes manquent.

Il faut donc souvent croiser :

    Pascal incomplet
          +
       TERA.COM
          +
       données
          +
       manuel
          ↓
    comportement reconstruit

---

# Classification obligatoire des résultats

Pour chaque fonction ou mécanique analysée, utiliser l'un des statuts suivants.

## CONFIRMÉ

Le comportement est suffisamment établi par les sources originales.

Il peut servir de base à la spécification puis au futur port C.

## PROBABLE

Les indices sont forts mais une partie de l'interprétation reste incertaine.

Ne pas présenter ce comportement comme une vérité définitive.

## À VÉRIFIER

Une implémentation ou une hypothèse existe mais elle n'est pas suffisamment démontrée.

## MANQUANT

Le mécanisme, la fonction ou le point d'entrée n'a pas encore été retrouvé ou compris.

## MODERNISABLE

La fonctionnalité originale dépend de DOS, du BIOS ou du matériel mais son implémentation exacte n'a pas besoin d'être reproduite.

Seul son effet fonctionnel doit être conservé.

---

# Reverse engineering ciblé

Le but n'est PAS de désassembler intégralement TERA.COM instruction par instruction.

Utiliser une stratégie ciblée :

    fonctionnalité inconnue
            ↓
    fonction / appel concerné
            ↓
    graphe d'appels
            ↓
    analyse des données
            ↓
    désassemblage ciblé
            ↓
    pseudo-code
            ↓
    comportement reconstruit

Si une mécanique est déjà suffisamment confirmée :

    ne pas la redésassembler sans raison

---

# Ne pas sur-analyser DOS / BIOS

Le but du reverse engineering est de comprendre **TERA**, pas de reconstruire complètement le fonctionnement de DOS ou du BIOS.

Ne pas perdre du temps à analyser en détail :

- interruptions BIOS standard ;
- gestion clavier bas niveau ;
- polling clavier ;
- gestion mémoire DOS standard ;
- allocation de segments standard ;
- fonctions DOS standards de fichiers ;
- routines CRT/Pascal runtime ;
- temporisations génériques ;
- initialisation vidéo standard ;
- wrappers système ;
- conventions d'appel sans logique métier ;
- manipulation technique de registres sans impact sur le jeu ;
- détails CGA déjà compris et documentés.

Lorsque leur comportement fonctionnel est évident, une abstraction est suffisante.

Exemple :

    INT 16h
       ↓
    "lit une touche"

est suffisant si aucun comportement particulier de TERA ne dépend de la manière précise dont le BIOS fournit cette touche.

De même :

    INT 21h
       ↓
    "ouvre et lit un fichier"

est suffisant lorsque le format du fichier et son utilisation dans le jeu sont déjà connus.

---

# Test de pertinence pour une routine système

À chaque routine DOS/BIOS/bas niveau rencontrée, poser la question :

    "Si je remplace cette routine par une implémentation moderne
     fonctionnellement équivalente, le comportement de TERA change-t-il ?"

Si :

    NON

alors :

    classer MODERNISABLE
    documenter brièvement son rôle
    continuer l'analyse

Si :

    OUI

alors analyser uniquement ce qui influence réellement le jeu.

---

# Exceptions : quand le bas niveau devient important

Une routine DOS/BIOS ou matérielle doit être analysée davantage si elle influence :

- le RNG ;
- les règles ;
- les statistiques ;
- les données ;
- une condition de gameplay ;
- une temporisation importante ;
- le comportement des entrées ;
- une protection ;
- une structure mémoire importante ;
- une mécanique propriétaire ;
- le rendu observable d'une manière non triviale ;
- la localisation d'un point d'entrée manquant.

Dans ce cas, analyser seulement la partie utile à la compréhension de TERA.

---

# Ce que signifie "fidèle"

Nous voulons une fidélité **fonctionnelle et observable**, pas une reproduction de la machine originale.

Nous voulons conserver :

    mêmes règles
    mêmes données utiles
    mêmes mécanismes
    mêmes événements
    mêmes résultats
    même progression
    même parcours
    même expérience fonctionnelle

Nous n'avons PAS besoin de reproduire :

    mêmes interruptions BIOS
    mêmes adresses mémoire
    mêmes segments
    même runtime Pascal
    mêmes fonctions DOS
    même organisation du binaire
    même code assembleur

si leur remplacement ne modifie pas le comportement du jeu.

---

# Modernisations autorisées

Certaines substitutions modernes sont explicitement autorisées.

Par exemple :

    clavier BIOS
          ↓
    système d'input moderne

    primitives CGA
          ↓
    SDL ou autre moteur graphique

    sauvegarde DOS
          ↓
    fichier moderne

    gestion mémoire DOS
          ↓
    structures C normales

La condition est :

    comportement du jeu inchangé

---

# Rendu

Le prototype doit tendre vers une représentation fidèle du jeu original.

Cependant, il n'est pas nécessaire de reproduire les limitations internes du matériel lorsqu'une implémentation moderne permet d'obtenir le même résultat observable.

Il est permis de remplacer :

    primitive graphique originale
             ↓
    primitive SDL équivalente

mais pas de modifier spontanément :

- composition des scènes ;
- sprites ;
- palette ;
- positions ;
- logique d'affichage ;
- animations ;
- informations présentées au joueur ;

lorsque ces éléments originaux ont été confirmés.

---

# Workflow cible du projet

Le workflow général doit rester :

    TERA.COM
       +
    Pascal incomplet
       +
    données originales
       +
    manuel
       +
    observations
          ↓
    REVERSE ENGINEERING
          ↓
    comportement confirmé
          ↓
    pseudo-code indépendant
          ↓
    spécification technique
          ↓
    moteur C portable
          ↓
    frontend moderne
          ↓
    prototype fidèle jouable

Le Python actuel peut être consulté pendant cette chaîne, mais il ne doit pas la remplacer.

---

# Définition de "terminé"

Le prototype est considéré comme suffisamment terminé lorsque :

1. le jeu peut être lancé ;
2. une nouvelle partie peut être créée ;
3. les personnages sont créés selon les règles originales ;
4. les principaux mondes et lieux sont accessibles ;
5. les déplacements fonctionnent correctement ;
6. les interactions essentielles existent ;
7. les monstres et rencontres fonctionnent ;
8. le combat fonctionne selon les règles reconstruites ;
9. les objets/coffres/pièges fonctionnent ;
10. les événements nécessaires à la progression fonctionnent ;
11. les objectifs originaux peuvent être accomplis ;
12. la condition de victoire peut être atteinte ;
13. le jeu peut être parcouru de bout en bout ;
14. aucune mécanique inventée n'est nécessaire pour terminer le jeu ;
15. les divergences restantes sont clairement documentées.

Lorsque ces critères sont atteints :

    STOP

Ne pas ajouter de fonctionnalités au-delà de l'original sans demande explicite.

---

# Principe directeur

À tout moment, préférer :

    comportement inconnu
          ↓
       MANQUANT

plutôt que :

    comportement inconnu
          ↓
    invention plausible

La priorité est :

    fidélité
      >
    complétude apparente
      >
    modernisation
      >
    embellissement