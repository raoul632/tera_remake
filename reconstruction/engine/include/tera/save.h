#ifndef TERA_SAVE_H
#define TERA_SAVE_H
#include <stddef.h>
#include <stdio.h>
#include "tera/game.h"

/* Format moderne autorise par le cadre ; aucune compatibilite avec le
 * conteneur DOS n'est revendiquee. La limite borne les lectures de fichiers
 * malformes et ne depend ni du packing C, ni de la taille des pointeurs. */
#define TERA_SAVE_CAPACITY 20000u
typedef enum {
    TERA_SAVE_OK, TERA_SAVE_ARGUMENT, TERA_SAVE_STATE,
    TERA_SAVE_FORMAT, TERA_SAVE_CHECKSUM, TERA_SAVE_IO, TERA_SAVE_BUFFER
} TeraSaveResult;

/** Encoder seulement une frontiere READY. Aucun effet sur le jeu/RNG.
 * buffer et size sont publies uniquement au succes. Aucun service runtime,
 * padding de structure ou union de session n'est serialise. */
TeraSaveResult tera_save_encode(const TeraGameSession *session,
    unsigned char *buffer,size_t capacity,size_t *size);
/** Valider entierement avant de remplacer session. Rebranche les services
 * fournis, sans les appeler. Taille exacte, version et CRC32 obligatoires.
 * CRC32 detecte les alterations accidentelles, pas une falsification. */
TeraSaveResult tera_save_decode(TeraGameSession *session,
    const unsigned char *buffer,size_t size,const TeraInputSource *input,
    TeraCombatObserver observer,void *context);
/** Flux binaire ouvert par l'hote. Ecriture depuis la position courante,
 * fflush verifie ; lecture d'un fichier unique depuis sa position courante,
 * EOF exige. Ne ferme pas le flux. L'hote gere publication atomique du
 * fichier, fermeture et synchronisation physique selon sa plateforme.
 * En cas d'echec d'ecriture, le flux peut contenir un fichier incomplet. */
TeraSaveResult tera_save_write(FILE *file,const TeraGameSession *session);
TeraSaveResult tera_save_read(FILE *file,TeraGameSession *session,
    const TeraInputSource *input,TeraCombatObserver observer,void *context);
#endif
