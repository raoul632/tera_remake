#include "character_transport.h"
#include <stdio.h>

int main(void)
{
    TeraCharacterState state;
    TeraNameVariants variants;
    TeraCreationSession session;
    int initialize, key;
    if (!tera_frontend_read_characters(&state, &variants, &initialize) ||
        (initialize && !tera_characters_initialize(&state, &variants)) ||
        !tera_creation_start(&session, &state)) {
        fputs("Etat de creation invalide\n", stderr);
        return 2;
    }
    tera_frontend_show_creation(&session);
    while (session.phase != TERA_CREATE_DONE) {
        if (!tera_frontend_read_int(&key, 0, 255)) return 4;
        tera_creation_key(&session, (unsigned)key);
        tera_frontend_show_creation(&session);
    }
    return 0;
}
