#include "character_transport.h"
#include "tera/inventory.h"
#include <stdio.h>
#include <string.h>

static void show(const TeraInventorySession *s)
{
    TeraCreationSession packet;
    memset(&packet,0,sizeof packet);packet.phase=TERA_CREATE_DONE;packet.state=s->state;
    tera_frontend_show_creation(&packet);
    printf("I %d %u %u %u %u %d\n",(int)s->phase,s->subject,s->slot,s->target,s->pending_item,(int)s->outcome);
    /* ABAD : seul l'abandon d'un compagnon laisse son indice dans D9B66.
     * Alfol relit ce global lors de la prochaine irradiation. */
    if(s->command=='A' && s->target=='C' && s->phase==TERA_INV_DONE && s->outcome==TERA_ITEM_DROPPED)
        printf("J %u\n",s->subject);
    fflush(stdout);
}

int main(void)
{
    TeraCharacterState state;
    TeraNameVariants variants;
    TeraInventorySession session;
    int mode,command,initialize,key,pending;
    if(!tera_frontend_read_int(&mode,0,1) || !tera_frontend_read_int(&command,0,255) ||
       !tera_frontend_read_characters(&state,&variants,&initialize) || initialize) return 2;
    if(mode) {
        if(command>9 || !tera_frontend_read_int(&pending,0,255)) return 2;
        memset(&session,0,sizeof session);session.state=state;session.phase=TERA_INV_DONE;
        session.subject=(unsigned)command;session.pending_item=(unsigned)pending;
        session.outcome=tera_inventory_acquire(&session.state,session.subject,&session.pending_item);
    } else if(!tera_inventory_start(&session,&state,(unsigned)command)) return 2;
    show(&session);
    while(session.phase<TERA_INV_DONE) {
        if(!tera_frontend_read_int(&key,0,255)) return 4;
        if(!tera_inventory_key(&session,(unsigned)key)) return 3;
        show(&session);
    }
    if(session.phase==TERA_INV_ERROR) {
        fputs("Dialogue de negociation non porte ici : N/D est disponible.\n",stderr);return 3;
    }
    return 0;
}
