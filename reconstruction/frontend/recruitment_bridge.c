#include "character_transport.h"
#include "tera/recruitment.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    TeraWorldState world;
    TeraNameVariants variants;
    TeraRecruitSession session;
    TeraCreationSession packet;
    int request,ready,key,initialize,value;
    unsigned id;
    memset(&world,0,sizeof world);
    if (!tera_frontend_read_int(&request,0,1) || !tera_frontend_read_int(&ready,0,1) ||
        !tera_frontend_read_int(&key,0,255) ||
        !tera_frontend_read_characters(&world.characters,&variants,&initialize) || initialize) return 2;
    for (id=1;id<10;++id) {
        if (!tera_frontend_read_int(&value,-32768,32767)) return 2;
        world.locations[id]=(int16_t)value;
    }
    if (!tera_recruit_start(&session,&world,(TeraRecruitRequest)request,ready) ||
        !tera_recruit_key(&session,(unsigned)key)) return 3;
    printf("R %d %d %d %u %u %u %u\n",(int)session.outcome,session.ready,session.negotiator,
           session.rolls,session.initial_party_count,session.available_mask,session.joined_mask);
    memset(&packet,0,sizeof packet);
    packet.state=session.world.characters;
    packet.phase=TERA_CREATE_DONE; /* Transporte un roster, pas une nouvelle creation. */
    tera_frontend_show_creation(&packet);
    return 0;
}
