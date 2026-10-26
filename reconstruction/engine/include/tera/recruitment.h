#ifndef TERA_RECRUITMENT_H
#define TERA_RECRUITMENT_H
#include "tera/world.h"

typedef enum { TERA_RECRUIT_SOLICIT, TERA_RECRUIT_SEDUCE } TeraRecruitRequest;
typedef enum {
    TERA_RECRUIT_WAIT, TERA_RECRUIT_CANCELLED, TERA_RECRUIT_AVAILABLE,
    TERA_RECRUIT_NOBODY, TERA_RECRUIT_FULL, TERA_RECRUIT_NEGOTIATED,
    TERA_RECRUIT_PAID, TERA_RECRUIT_TOO_POOR, TERA_RECRUIT_NO_CONTEXT
} TeraRecruitOutcome;

typedef struct {
    TeraWorldState world;
    TeraRecruitRequest request;
    TeraRecruitOutcome outcome;
    int ready; /* AFCE, contexte original de sollicitation/information. */
    int negotiator; /* Dernier membre reussissant, -1 si aucun. */
    unsigned available_mask, joined_mask, rolls, initial_party_count;
} TeraRecruitSession;

/** C_A074 : present a Position, absent du groupe et etat !=HS.
 * Ne pas substituer HP>0 : les statuts sont mis a jour ailleurs.
 * Retour masque IDs1..9 disponibles, zero si NULL. */
unsigned tera_recruit_available(const TeraWorldState *world);
/** Commencer l'une des deux commandes originales S : solliciter dans le
 * dialogue (verbe25, C_B215), seduire hors dialogue (verbe14, C_B6D4).
 * ready fournit AFCE existant : le moteur n'ajoute pas une recherche
 * automatique de compagnon au moment de seduire. Aucun tirage au start.
 * Retour 0 sans mutation si NULL/request/ready invalides. */
int tera_recruit_start(TeraRecruitSession *session, const TeraWorldState *world,
                        TeraRecruitRequest request, int ready);
/** Une confirmation C_4800 : O/o/Entree confirme ; autre touche annule.
 * Solliciter : recherche les disponibles, sans RNG ni recrutement.
 * Seduire avec ready : compter 1+actifs1..9 (HS compris). Si <4,
 * tirer C_8315(charme,negociation) pour les DIX IDs, meme les inactifs/HS.
 * Dernier actif non-HS dont jet>0 OU Position==LocChars[8] reussit.
 * Succes : tous les disponibles rejoignent et XP+=5 (16 bits).
 * Echec : argent>249 => -250, TOUS les IDs1..9 a la position sont actives,
 * y compris HS. Sinon rien n'est paye. Ne pas normaliser ces particularites.
 * Sans ready : phrase RANDOM3, aucun autre effet.
 * Ce bloc de regles isole ne joue pas C6518 apres succes : le resident
 * dialogue ajoute ses tirages selon SoundFlag. rolls compte les jets
 * de negociation/phrase, pas ceux de cette melodie.
 * Retour 0 si NULL/touche>255/session finie, sans mutation. */
int tera_recruit_key(TeraRecruitSession *session, unsigned key);
#endif
