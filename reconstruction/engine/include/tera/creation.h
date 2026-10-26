#ifndef TERA_CREATION_H
#define TERA_CREATION_H
#include "tera/interactive.h"

/* Les noms restent en octets CP437, neuf caracteres + terminateur.
 * L'encodage d'affichage appartient au frontend, pas aux regles. */
typedef struct {
    TeraCombatGame combat;
    char names[10][10];
    int16_t keys[10], days, level, dummy_level, energy;
    int arrival_pending; /* A74F, distinct d'AFD1 de la fuite. */
} TeraCharacterState;

/* Copie la fiche courante sans recalcul ni effet RNG. live peut etre NULL,
 * ou le combat actif dont HP/MP/equipement doivent etre presentes. */
int tera_character_card(const TeraCharacterState *state,const TeraCombatGame *live,
    unsigned actor,TeraCharacterCard *card);
typedef struct { char names[2][10][10]; } TeraNameVariants;

/** OVL_0010:C597..C845 : effacer equipement/stats/skills, choisir les noms
 * des neuf compagnons, specialites puis tirer leurs 81 caracteristiques.
 * variants contient CharName_bis[0/1][0..9], noms de base dans state.names.
 * RANDOM(3)=0 conserve le nom courant ; 1/2 choisit la variante.
 * Les equipements deterministes de la FIN de l'overlay sont appliques aussi
 * (aucun tirage) ; le calendrier geographique situe entre ces deux blocs
 * reste a la charge de l'appelant. Aucun monde/indice/mot n'est fabrique.
 * Retour 0 si noms non termines/NULL, sans mutation. HP/MP non recalcules
 * ici : l'overlay de creation du heros les initialise apres validation. */
int tera_characters_initialize(TeraCharacterState *state,
                               const TeraNameVariants *variants);

typedef enum {
    TERA_CREATE_NAME, TERA_CREATE_GENDER, TERA_CREATE_SPLIT,
    TERA_CREATE_BONUS, TERA_CREATE_KEEP, TERA_CREATE_DONE
} TeraCreationPhase;
typedef struct {
    TeraCharacterState state;
    TeraCreationPhase phase;
    unsigned split, bonuses_left, name_length, attempts;
    char name_buffer[11];
    unsigned gender; /* Caractere lu : seul F change le bonus. */
} TeraCreationSession;

/** OVL_0022 : commencer a la saisie du nom, sans tirage ni effacement
 * du roster fourni. Permet la recreation apres defaite : equipement,
 * compagnons et cartes sont conserves comme dans cet overlay.
 * Retour 0 sans mutation pour pointeurs/noms invalides. */
int tera_creation_start(TeraCreationSession *session, const TeraCharacterState *state);
/** Une touche C_4277, minuscules ASCII majuscules. Nom : saisie C_42AC,
 * lettres seulement, backspace/Entree, dix lettres puis stockage de neuf.
 * Genre : F => charme+2, toute autre touche => force+2, APRES les repartitions.
 * Repartition : un chiffre 0..9 ; trop grand => RANDOM(primaire-1)+1,
 * zero est accepte. Bonus : six lettres valides F/I/N/E/V/T/A/C/H ; pas
 * de plafond invente. KEEP : N recommence (sans resemer), toute autre
 * touche accepte. L'attente ne consomme aucun tirage.
 * Fin : restaurer les dix HP/MP et OK, exclure les compagnons, remettre
 * skills heros a zero, argent >=500, XP/niveau=0, jours=7300, monde11,
 * position1000, cle heros0 et A74F=1. Equipement preserve.
 * Retour 0 pour NULL, touche >255 ou session terminee, sans mutation. */
int tera_creation_key(TeraCreationSession *session, unsigned key);
/** Reintegration atomique de la creation dans un etat de jeu existant.
 * Ne remplace pas cartes, H, mots, donnees monstres ou contexte Alfol
 * de destination ; ne reintegre que personnages et champs modifies par
 * OVL_0022, dont la graine. Retour 0 si session non terminee/NULL.
 * Les champs hors TeraCombatGame (jours, cles, niveau, A74F) restent
 * disponibles dans session.state pour la future boucle generale. */
int tera_creation_apply_combat(const TeraCreationSession *session, TeraCombatGame *game);
#endif
