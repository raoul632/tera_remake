#ifndef TERA_SPACE_H
#define TERA_SPACE_H
#include "tera/creation.h"
typedef enum { TERA_SPACE_CONFIRM,TERA_SPACE_FUEL,TERA_SPACE_DIGIT,
    TERA_SPACE_SHOT,TERA_SPACE_DONE,TERA_SPACE_HALTED,TERA_SPACE_ERROR } TeraSpacePhase;
typedef struct {
    TeraCharacterState state;
    TeraSpacePhase phase;
    int16_t pirates,counter,backup_position;
    int pilot,returning,departed,alien_x,alien_y,hit;
    unsigned delay_ms,trials;
    unsigned input_ms,input_stage;
    int input_key; /* -1=aucune touche en attente ; FIFO reduite au premier. */
} TeraSpaceSession;
/** OVL00AE en jeu, pas sa demonstration : fuel puis pirates et saut.
 * Copie le roster, conserve RNG et compteurs 16 bits. Un key=0 a SHOT
 * signifie poll final sans touche : API de lecture semantique des bancs.
 * Pour des entrees brutes horodatees : tera_space_input_at ci-dessous.
 * HALTED est le HALT original, sans recreation du heros. */
int tera_space_start(TeraSpaceSession *session,const TeraCharacterState *state,int16_t pirates);
/** TERAMAIN : confirmation du spatioport et dix jets de pilotage.
 * returning=1 reprend les pirates restants, au moins quatre, depuis backup.
 * Le dernier pilote apte reussissant determine le nombre de pirates. */
int tera_space_travel_start(TeraSpaceSession *session,const TeraCharacterState *state,
                            int returning,int16_t pirates,int16_t backup);
int tera_space_key(TeraSpaceSession *session,unsigned key);
/** Evenement au temps virtuel elapsed_ms depuis le debut du tir courant ;
 * key=-1 avance sans touche, sinon octet 0..255. Temps monotone entre 0
 * et delay_ms+990. Purges a delay_ms et delay_ms+300, puis quatre tailles
 * 230/200/170/90 : premiere touche apres la seconde purge lue au poll final.
 * Un evenement exactement a une purge est efface ; au temps final accepte.
 * Aucun sommeil/tirage de plus. Resout l'echeance via tera_space_key ;
 * remet le temps a zero si un autre tir commence. Fournir le surplus de
 * temps separement. Refus sans mutation pour mauvais temps/phase/touche. */
int tera_space_input_at(TeraSpaceSession *session,unsigned elapsed_ms,int key);
#endif
