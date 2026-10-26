#include "tera/interactive.h"
#include "tera/sound.h"
#include <stddef.h>
#include <string.h>

static int16_t signed_word(int32_t value)
{
    uint32_t bits = (uint32_t)value & UINT32_C(65535);
    return (int16_t)(bits < 32768 ? (int32_t)bits : (int32_t)bits - 65536);
}

static void present(TeraCombatSession *s,TeraCombatEventKind kind,unsigned actor,char letter,int value,int reward)
{
    {
        unsigned i;TeraPresentationEvent e=tera_presentation_make(&s->game,TERA_PRESENT_COMBAT,TERA_SOURCE_COMBAT);
        e.data.combat.kind=(unsigned)kind;e.data.combat.actor=actor;
        e.data.combat.letter=letter;e.data.combat.value=value;
        e.data.combat.type=s->game.encounter.monster_type;
        e.data.combat.reward=reward;
        e.data.combat.eligible=s->last_attack_eligible;e.data.combat.item=s->last_bonus_item;
        e.data.combat.monster_hp=s->game.encounter.monster_hp;
        for(i=0;i<10;++i) {
            const TeraRoundActor *a=&s->game.encounter.actors[i];
            TeraBattleVitals *v=&e.data.combat.actors[i];
            v->active=a->active;v->hp=a->hp;v->magic=a->magic;
            v->hp_limit=a->hp_limit;v->magic_limit=a->magic_limit;
            memcpy(v->condition,a->condition,3);
        }
        tera_presentation_emit(&s->game,&e);
    }
}
static void emit(TeraCombatSession *s,TeraCombatEventKind kind,unsigned actor,char letter,int value)
{
    TeraCombatEvent event={kind,actor,letter,value};
    if(s->observe) s->observe(s->context,&event);
    present(s,kind,actor,letter,value,0);
}

static unsigned draw(TeraCombatSession *s, unsigned bound)
{
    unsigned value = 0;
    tera_random_draw(&s->game.random, bound, &value);
    return value;
}

/* C_65B5 et C_6976 : conserver les tirages de la presentation meme sans
 * haut-parleur. Leur position dans le flux influence les prochains jets. */
/* C_6D32 : huit changements de couleurs, y compris avec SoundFlag=0.
 * Ce n'est pas un tirage de degats, mais sa graine est durable. */
static void flash_random(TeraCombatSession *s)
{
    unsigned i;
    for (i = 0; i < 8; ++i) {
        TeraPresentationEvent e=tera_presentation_make(&s->game,TERA_PRESENT_FLASH,TERA_SOURCE_COMBAT);
        e.data.flash.palette=draw(s,4);e.data.flash.border=draw(s,16);e.data.flash.ms=60;
        tera_presentation_emit(&s->game,&e);
    }
}

/* C_6518 -> C_62B7 : la melodie de victoire a trois segments. La boucle
 * s'arrete APRES increment du compteur initial 1 : duree-1 iterations.
 * Le tirage de bruit n'existe que lorsque RANDOM20 < 5. */
static void victory_sound_random(TeraCombatSession *s)
{
    tera_sound_victory(&s->game);
}

/* C_842A : chaque appel tire d'abord RANDOM15, y compris les branches
 * simples qui tirent ensuite un second RANDOM15. Equipement local uniquement. */
static int16_t roll_action(TeraCombatSession *s, unsigned actor, unsigned action)
{
    unsigned required = 0, bonus = 0, first, second = 3;
    int16_t value = 0;
    switch (action) {
    case 1: required = 15; break;
    case 2: required = 18; break;
    case 3: required = s->game.helpers[3]; break;
    case 4: required = s->game.helpers[0]; break;
    case 5: required = s->game.helpers[2]; break;
    case 6: required = s->game.helpers[1]; break;
    case 7: required = 21; break;
    default: break;
    }
    first = draw(s, 15) + 3;
    if (action <= 5 || action == 9)
        second = draw(s, 15) + 3;
    if (required) {
        bonus = tera_combat_equipment_bonus(s->game.equipment[actor], required);
        /* C_8391 ne touche pas AFCC pour les armures (21).
         * Un sort J et une defense mentale le laissent aussi persister. */
        if (required != 21)
            s->bonus_item = bonus ? (uint8_t)(required - 3 + bonus) : 0;
    }
    tera_combat_action(&s->game.encounter.attributes[actor], action, bonus,
                        first, second, &value);
    return value;
}

static unsigned action_type(char letter)
{
    switch (letter) {
    case 'L': return 1; case 'T': return 2; case 'A': return 3;
    case 'D': return 4; case 'P': return 5; case 'C': return 6;
    case 'J': return 9; default: return 0;
    }
}

/* C_86E1 : tirer dans la plage originale puis tester la carte, sans filtrer
 * prealablement la liste des candidats (ce qui changerait le RNG).
 * Une carte sans aucun candidat est une erreur d'integration : le COM
 * bouclerait pour toujours. Le controle ne modifie pas une carte valide. */
static int relocate_escape(TeraCombatSession *s)
{
    TeraCombatGame *g = &s->game;
    unsigned candidate, i, valid = 0;
    if (g->world_id == 11) {
        g->position = (int16_t)(g->on_alfol ? draw(s, 99) + 1 : 3 * draw(s, 255));
    } else {
        for (i = 5; i <= 94; ++i)
            valid |= g->maps_column7[g->world_id * 100 + i];
        if (!valid) {
            s->phase = TERA_COMBAT_ERROR;
            return 0;
        }
        do {
            candidate = g->world_id * 100 + draw(s, 90) + 5;
            g->previous_position = (int16_t)(candidate - 1);
        } while (!g->maps_column7[candidate]);
        g->position = (int16_t)candidate;
    }
    g->encounter.fled = 1;
    g->location_changed = 1;
    emit(s, TERA_EVENT_ESCAPE, s->actor, 'F', g->position);
    return 1;
}

/* C_92FE : la cible vient de la liste figee du tour. Le jet de poison
 * existe pour TOUS les monstres si l'attaque touche, meme les non toxiques. */
static void retaliate(TeraCombatSession *s)
{
    TeraEncounterState *e = &s->game.encounter;
    TeraMonsterAttackResult attack;
    uint8_t target;
    int16_t defense;
    s->retaliation_done = 1;
    /* CASE 1..4 seulement : un tirage vers les autres aptes ne provoque
     * aucune attaque, ni defense ni poison. Ne pas retirer une cible. */
    if (!tera_combat_select_round_target(&s->round, draw(s, s->round.count), &target))
        return;
    defense = roll_action(s, target, e->monster_type < 15 ? 7 : 8);
    tera_combat_resolve_monster_attack(e->monster_type, e->monster_hp,
                                       defense, 0, &attack);
    if (attack.poison_draw_required)
        tera_combat_resolve_monster_attack(e->monster_type, e->monster_hp,
            defense, draw(s, 11 - s->game.difficulty) == 0, &attack);
    e->actors[target].hp = signed_word((int32_t)e->actors[target].hp - attack.damage);
    if (e->monster_type >= 15) flash_random(s);
    if (attack.poisoned)
        memcpy(e->actors[target].condition, "EM", 3);
    s->retaliation_done = 1;
    emit(s, TERA_EVENT_MONSTER_HIT, target, 0, attack.damage);
}

static void finish_combat(TeraCombatSession *s)
{
    TeraCombatRewards reward;
    TeraEncounterState *e = &s->game.encounter;
    unsigned money_draw = e->victory && e->party_available ? draw(s, 50) : 0;
    tera_combat_victory_rewards(e->victory, e->party_available, s->game.money,
        s->game.experience, s->initial_monster_hp, money_draw, &reward);
    s->game.money = reward.money;
    s->game.experience = reward.experience;
    s->phase = TERA_COMBAT_DONE;
    present(s, TERA_EVENT_END, 0, 0, reward.granted,reward.granted ? (int)money_draw+2 : 0);
    if(reward.granted) {
        if(s->game.sound) victory_sound_random(s);
        else tera_media_wait(&s->game,1500);
    }
    /* Le journal logique historique observe toujours la graine finale,
     * APRES la melodie. Le panneau visuel precede sa duree. */
    if(s->observe) {
        TeraCombatEvent event={TERA_EVENT_END,0,0,reward.granted};
        s->observe(s->context,&event);
    }
}

/* C_9567 puis C_4D46 : HS a la fin du tour seulement, maxima des quatre
 * premiers actifs, meme HS. La riposte n'invalide pas immediatement un menu. */
static void finish_round(TeraCombatSession *s)
{
    TeraEncounterState *e = &s->game.encounter;
    unsigned slot, id, shown = 0;
    e->party_available = 0;
    for (slot = 0; slot < s->round.count && slot < 4; ++slot)
        if (tera_combat_update_actor_condition(&e->actors[s->round.actor_ids[slot]]))
            e->party_available = 1;
    for (id = 0; id < 10 && shown < 4; ++id)
        if (e->actors[id].active) {
            TeraCombatLimits limits;
            tera_combat_actor_limits(&e->attributes[id], &limits);
            e->actors[id].hp_limit = limits.hp;
            e->actors[id].magic_limit = limits.magic;
            ++shown;
        }
}

/* Avancer jusqu'au clavier ; une cle n'effectue pas une boucle speculative
 * de tirages. action_context distingue les premieres gardes de C_9419. */
static void advance(TeraCombatSession *s)
{
    TeraEncounterState *e = &s->game.encounter;
    for (;;) {
        if (!s->action_context) {
            if (!e->party_available || e->victory || e->fled) {
                finish_combat(s);
                return;
            }
            if (!tera_combat_prepare_round(e->actors,
                    s->game.monster_values[e->monster_type - 1][4], draw(s, 5), &s->round)) {
                s->phase = TERA_COMBAT_ERROR;
                return;
            }
            ++s->round_number;
            s->slot = 0;
            s->retaliation_done = 0;
            s->action_context = 1;
            emit(s, TERA_EVENT_ROUND, 0, 0, s->round.party_first);
            if (!s->round.party_first)
                retaliate(s);
        }
        /* Le premier emplacement ignore victoire ; les suivants non.
         * Apres F les trois premiers tests HP restent executes. */
        while (s->slot < 4) {
            if (s->slot < s->round.count && !e->fled &&
                (s->slot == 0 || !e->victory) &&
                (s->round.party_first || e->party_available)) {
                s->actor = s->round.actor_ids[s->slot];
                s->attempt = 0;
                s->game.recruit_ready = 0; /* 87AB, debut de chaque menu. */
                s->letter = 0;
                s->phase = TERA_COMBAT_ACTION;
                return;
            }
            if (s->slot < 3 && e->monster_hp < 3)
                e->victory = 1;
            ++s->slot;
        }
        if (e->monster_hp < 3)
            e->victory = 1;
        else if (s->round.party_first && !s->retaliation_done)
            retaliate(s); /* Oui, aussi apres F confirme. */
        finish_round(s);
        s->action_context = 0;
    }
}

static void end_actor_turn(TeraCombatSession *s)
{
    /* C_8CE3:92EF le vide aussi pour une action inadmissible/annulee.
     * F saute ce nettoyage a 92F5. */
    if (!s->game.encounter.fled)
        s->bonus_item = 0;
    if (s->slot < 3 && s->game.encounter.monster_hp < 3)
        s->game.encounter.victory = 1;
    ++s->slot;
    advance(s);
}

static void perform_action(TeraCombatSession *s)
{
    TeraEncounterState *e = &s->game.encounter;
    TeraHeroAttackResult attack;
    int16_t raw = roll_action(s, s->actor, action_type(s->letter));
    if (s->game.sound && s->letter == 'T' &&
        tera_combat_equipment_bonus(s->game.equipment[s->actor], 18))
        tera_sound_action(&s->game,'T'); /* 89FA, apres le jet et avant la phrase. */
    /* C_4473 choisit une phrase : ce tirage est un effet de jeu car il
     * decale les suivants. T sans arme ne passe pas par C_4473. */
    if (s->letter != 'T' || tera_combat_equipment_bonus(s->game.equipment[s->actor], 18))
        emit(s, TERA_EVENT_ACTION, s->actor, s->letter, (int)draw(s, 3));
    else
        emit(s, TERA_EVENT_ACTION, s->actor, s->letter, -1);
    if(s->game.sound && s->letter!='T') tera_sound_action(&s->game,s->letter);
    tera_combat_resolve_hero_attack(&e->reaction, s->letter, raw,
        e->actors[s->actor].magic, e->monster_hp,
        strcmp(s->word, s->game.secret_word) == 0, &attack);
    e->actors[s->actor].magic = attack.magic_after;
    e->monster_hp = attack.monster_hp_after;
    s->last_attack_eligible=(unsigned)attack.eligible;
    s->last_bonus_item=attack.damage>0 ? s->bonus_item : 0;
    if (attack.eligible) {
        tera_media_wait(&s->game,500);
        if (attack.damage > 0 && s->letter == 'J') flash_random(s); /* Code action 10, pas type du monstre. */
        if (attack.damage > 0 && s->bonus_item)
            emit(s, TERA_EVENT_EQUIPMENT, s->actor, s->letter, s->bonus_item);
        s->bonus_item = 0; /* C_8B5E le vide aussi quand l'attaque manque. */
    }
    emit(s, TERA_EVENT_HERO_HIT, s->actor, s->letter, attack.damage);
    if(attack.eligible) {
        if(attack.damage>0) {
            tera_media_wait(&s->game,s->game.sound ? 300 : 700);
            if(s->game.sound) tera_sound_hit(&s->game);
        } else tera_media_wait(&s->game,800);
    } else {
        tera_media_tone(&s->game,s->game.sound ? 100 : 0,400);
        tera_media_wait(&s->game,400);
    }
    end_actor_turn(s);
}

int tera_combat_start(TeraCombatSession *s, const TeraCombatGame *g,
                      unsigned aggressor, TeraCombatObserver observer, void *context)
{
    TeraCombatSession next;
    unsigned id, count = 0;
    const int16_t *row;
    if (!s || !g || aggressor > 9 || g->world_id > 11 || g->difficulty > 10 ||
        (g->sound != 0 && g->sound != 1) ||
        g->encounter.monster_type < 1 || g->encounter.monster_type > 22 ||
        memchr(g->secret_word, 0, sizeof g->secret_word) == NULL)
        return 0;
    row = g->monster_values[g->encounter.monster_type - 1];
    if (row[5] <= 0)
        return 0;
    memset(&next, 0, sizeof next);
    next.game = *g;
    next.observe = observer;
    next.context = context;
    for (id = 0; id < 4; ++id)
        if (g->helpers[id] < 3 || g->helpers[id] > 25)
            return 0;
    next.game.encounter.victory = next.game.encounter.fled = 0;
    next.game.encounter.party_available = 0;
    for (id = 0; id < 10; ++id) {
        TeraRoundActor *actor = &next.game.encounter.actors[id];
        if (actor->active > 1 || actor->condition[2] != 0)
            return 0;
        actor->agility = next.game.encounter.attributes[id].stats[7];
        if (actor->active && tera_combat_update_actor_condition(actor))
            ++count;
    }
    next.game.encounter.party_available = count != 0;
    next.game.encounter.reaction.resistance_a = row[1];
    next.game.encounter.reaction.resistance_b = row[2];
    next.game.encounter.reaction.reaction_category = row[3];
    *s = next;
    if (count) {
        emit(s, TERA_EVENT_INTRO, aggressor, 0, aggressor ? -1 : (int)draw(s, 3));
        if(s->game.sound) {
            tera_media_tone(&s->game,440,600);tera_media_wait(&s->game,80);
            tera_media_tone(&s->game,440,600);
        }
        tera_media_wait(&s->game,1000);
        s->initial_monster_hp = signed_word((int32_t)row[0] + (int32_t)draw(s, (unsigned)row[5]));
        s->game.encounter.monster_hp = s->initial_monster_hp;
        advance(s);
    } else {
        s->phase = TERA_COMBAT_DONE;
        emit(s, TERA_EVENT_END, 0, 0, 0);
    }
    return 1;
}

int tera_combat_key(TeraCombatSession *s, unsigned key)
{
    if (!s || key > 255 || s->phase == TERA_COMBAT_DONE || s->phase == TERA_COMBAT_ERROR)
        return 0;
    if (key >= 'a' && key <= 'z')
        key -= 'a' - 'A';
    if (s->phase == TERA_COMBAT_WORD) {
        if (key >= 'A' && key <= 'Z')
            s->word[s->word_length++] = (char)key;
        else if (key == 8 && s->word_length)
            --s->word_length;
        s->word[s->word_length] = 0;
        if (key == 13 || s->word_length == 10) {
            s->word[9] = 0; /* Le buffer C_42AC admet 10 ; A859 n'en garde que 9. */
            perform_action(s);
        }
    } else if (s->phase == TERA_COMBAT_ACTION) {
        s->letter = (char)key;
        if (action_type(s->letter) || key == 'F')
            s->phase = TERA_COMBAT_CONFIRM;
        else {
            emit(s, TERA_EVENT_PASS, s->actor, 0, (int)s->attempt);
            if (s->attempt)
                end_actor_turn(s);
            else {
                s->attempt = 1;
                s->game.recruit_ready = 1; /* 87D9 avant la deuxieme lecture. */
            }
        }
    } else {
        if (key != 'O' && key != 13) {
            emit(s, TERA_EVENT_CANCEL, s->actor, s->letter, (int)s->attempt);
            if (s->attempt)
                end_actor_turn(s);
            else {
                s->attempt = 1;
                s->game.recruit_ready = 1;
                s->phase = TERA_COMBAT_ACTION;
            }
        } else if (s->letter == 'F') {
            if (relocate_escape(s))
                end_actor_turn(s);
        } else if (s->letter == 'C') {
            memset(s->word, 0, sizeof s->word);
            s->word_length = 0;
            s->phase = TERA_COMBAT_WORD;
        } else
            perform_action(s);
    }
    return 1;
}
