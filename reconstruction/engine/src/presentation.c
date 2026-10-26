#include "tera/interactive.h"
#include <string.h>

void tera_presentation_interaction(const TeraCombatGame *g,TeraPresentationSource source,
    TeraInteractionEffect effect,int amount)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_INTERACTION,source);
    e.data.interaction.effect=effect;e.data.interaction.amount=amount;
    tera_presentation_emit(g,&e);
}

TeraPresentationEvent tera_presentation_make(const TeraCombatGame *g,
    TeraPresentationKind kind,TeraPresentationSource source)
{
    TeraPresentationEvent e;
    memset(&e,0,sizeof e);e.kind=kind;e.source=source;
    if(g) {e.world=g->world_id;e.position=g->position;e.on_alfol=g->on_alfol;}
    return e;
}
void tera_presentation_emit(const TeraCombatGame *g,const TeraPresentationEvent *e)
{
    if(g && e && g->presentation && g->presentation->emit)
        g->presentation->emit(g->presentation->context,e);
}
void tera_presentation_message(const TeraCombatGame *g,TeraPresentationSource source,
    TeraTextResource resource,unsigned id,unsigned variant)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_MESSAGE,source);
    e.data.message.resource=resource;e.data.message.id=id;e.data.message.variant=variant;
    tera_presentation_emit(g,&e);
}
void tera_presentation_move(const TeraCombatGame *g,TeraPresentationSource source,
    unsigned command,int from,int moved,unsigned wall)
{
    TeraPresentationEvent e=tera_presentation_make(g,moved ? TERA_PRESENT_MOVE : TERA_PRESENT_BLOCKED,source);
    e.data.move.command=command;e.data.move.from=from;e.data.move.to=g->position;
    e.data.move.previous=g->previous_position;e.data.move.wall=wall;
    tera_presentation_emit(g,&e);
}
void tera_presentation_zone(const TeraCombatGame *g,TeraPresentationSource source,
    unsigned from_world,int from_position,int from_alfol)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_ZONE,source);
    e.data.zone.from_world=from_world;e.data.zone.from_position=from_position;
    e.data.zone.from_alfol=from_alfol;tera_presentation_emit(g,&e);
}
void tera_presentation_objective(const TeraCombatGame *g,TeraPresentationSource source,unsigned id)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_OBJECTIVE,source);
    e.data.objective.id=id;tera_presentation_emit(g,&e);
}
void tera_presentation_actor(const TeraCombatGame *g,TeraPresentationSource source,TeraActorEffect reason,unsigned id)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_ACTOR,source);
    if(!g || id>=10) return;
    e.data.actor.reason=reason;e.data.actor.id=id;
    e.data.actor.hp=g->encounter.actors[id].hp;e.data.actor.magic=g->encounter.actors[id].magic;
    memcpy(e.data.actor.condition,g->encounter.actors[id].condition,3);tera_presentation_emit(g,&e);
}
