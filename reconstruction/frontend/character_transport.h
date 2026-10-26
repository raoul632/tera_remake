#ifndef TERA_CHARACTER_TRANSPORT_H
#define TERA_CHARACTER_TRANSPORT_H
#include "tera/creation.h"
/* Protocole prive decimal partage creation/recrutement. Verification des
 * bornes AVANT toute mutation de jeu ; pas de structs natives sur le fil. */
int tera_frontend_read_int(int *value,int low,int high);
int tera_frontend_read_characters(TeraCharacterState *state,TeraNameVariants *variants,int *initialize);
void tera_frontend_show_creation(const TeraCreationSession *session);
#endif
