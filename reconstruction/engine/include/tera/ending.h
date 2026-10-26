#ifndef TERA_ENDING_H
#define TERA_ENDING_H
#include "tera/exploration.h"
typedef struct {
    TeraExplorationState state;
    unsigned complete,victory,runtime_error;
    uint8_t score_real48[6];
    double score;
    char score_text[32];
} TeraEndingSession;
/** OVL00CB quand les quatre objectifs sont vrais. Le groupe disponible
 * recoit le score ; le groupe mort recoit seulement les condoleances.
 * Copie l'etat et consomme la musique avant la pause. Days-4000 reste un
 * Integer 16 bits ; denominateur nul signale l'erreur Pascal de division.
 * Aucun successeur apres acquittement ; aucune sauvegarde DOS ecrite. */
int tera_ending_start(TeraEndingSession *session,const TeraExplorationState *state);
int tera_ending_key(TeraEndingSession *session,unsigned key);
#endif
