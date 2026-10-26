#ifndef TERA_PRESENTATION_H
#define TERA_PRESENTATION_H
#include "tera/character_card.h"

/* Evenements par valeur : aucun pointeur vers l'etat ou les ressources.
 * Les IDs/outcomes conservent les enums des modules, voir spec_presentation_c. */
typedef enum {
    TERA_PRESENT_MOVE, TERA_PRESENT_BLOCKED, TERA_PRESENT_ZONE,
    TERA_PRESENT_PLACE, TERA_PRESENT_MESSAGE, TERA_PRESENT_CHEST,
    TERA_PRESENT_TRAP, TERA_PRESENT_INVENTORY, TERA_PRESENT_SHOP,
    TERA_PRESENT_RECRUIT, TERA_PRESENT_OBJECTIVE, TERA_PRESENT_ENDING,
    TERA_PRESENT_COMBAT, TERA_PRESENT_RESIDENT, TERA_PRESENT_ACTOR,
    TERA_PRESENT_CHARACTER,TERA_PRESENT_INTERACTION,
    TERA_PRESENT_TONES,TERA_PRESENT_WAIT,TERA_PRESENT_FLASH,TERA_PRESENT_MONSTER,
    TERA_PRESENT_SCENERY,TERA_PRESENT_RAY,TERA_PRESENT_TUNNEL
} TeraPresentationKind;
typedef enum {
    TERA_SOURCE_EXPLORATION, TERA_SOURCE_NAVIGATION, TERA_SOURCE_AMARANDE,
    TERA_SOURCE_ALFOL, TERA_SOURCE_MEDUZ, TERA_SOURCE_SPACE,
    TERA_SOURCE_DIALOGUE, TERA_SOURCE_INVENTORY, TERA_SOURCE_SHOP,
    TERA_SOURCE_CHEST, TERA_SOURCE_ENDING, TERA_SOURCE_LIFE, TERA_SOURCE_COMBAT,
    TERA_SOURCE_CREATION
} TeraPresentationSource;
typedef enum {
    TERA_TEXT_RESPONSE, /* C4473 : id de verbe, variant 0..2 */
    TERA_TEXT_MESSAGE, /* MESSAGE* : id et variant 0..2 */
    TERA_TEXT_HINT, /* world.hints : id 1..38 ; variant inutilise */
    TERA_TEXT_SETH, /* inscriptions locales : id 100/101 */
    TERA_TEXT_ELLIPSIS, /* absence d'information, pas un ID invente */
    TERA_TEXT_COMMAND_RESPONSE /* id=octet commande, variante C4473 conservee */
} TeraTextResource;
#define TERA_TEXT_NO_VARIANT (~0u)
typedef enum {TERA_EFFECT_TRAP,TERA_EFFECT_HEAL,TERA_EFFECT_RADIATION} TeraActorEffect;
typedef enum {TERA_INTERACTION_MENTAL_EXTENSION,TERA_INTERACTION_MEDIA_PAID,
    TERA_INTERACTION_TOO_POOR} TeraInteractionEffect;

typedef struct {int hp,magic,hp_limit,magic_limit;unsigned active;char condition[3];} TeraBattleVitals;
typedef struct {
    unsigned type,actor,kind,eligible,item;char letter;int value,monster_hp,reward;
    TeraBattleVitals actors[10];
} TeraBattleFrame;
typedef struct {unsigned type,count;unsigned short values[240];} TeraMonsterPicture;
typedef enum {TERA_PICTURE_PLACE,TERA_PICTURE_FACADE,TERA_PICTURE_ALFOL,
    TERA_PICTURE_ROOM,TERA_PICTURE_MEDUZ,TERA_PICTURE_SPACE,TERA_PICTURE_RUINS,TERA_PICTURE_HALT,TERA_PICTURE_COUNT} TeraPictureKind;
/* Tirages graphiques originaux dans leur ordre d'evaluation, sans pointeur.
 * variant distingue les compositions d'une meme famille (lieu/type Alfol). */
typedef struct {unsigned kind,variant,count;unsigned short values[256];} TeraScenePicture;
typedef struct {
    unsigned reset,palette,background,color,radius,erase_radius,hz;
    int x,y,erase_x,erase_y;
    unsigned short stars[48];
} TeraTunnelFrame;

typedef struct {
    TeraPresentationKind kind;
    TeraPresentationSource source;
    unsigned world;
    int position,on_alfol;
    union {
        TeraCharacterCard character;
        struct {TeraInteractionEffect effect;int amount;} interaction;
        struct {unsigned command,wall;int from,to,previous;} move;
        struct {unsigned from_world;int from_position,from_alfol;} zone;
        struct {unsigned id;} place;
        struct {TeraTextResource resource;unsigned id,variant;} message;
        struct {unsigned command,actor,content,reward;int outcome,open;} chest;
        struct {unsigned actor;int kind;} trap;
        struct {unsigned command,subject,target,slot,item;int outcome;} inventory;
        struct {unsigned actor,item;int outcome,price,weeks;} shop;
        struct {int outcome,negotiator;unsigned available_mask,joined_mask;} recruit;
        struct {unsigned id;} objective; /* indices 0..3 de tasks_done */
        struct {int victory;char score[32];} ending;
        TeraBattleFrame combat;
        struct {unsigned count;unsigned short hz[64],ms[64];} tones;
        struct {unsigned ms;} wait;
        struct {unsigned palette,border,ms,audio;} flash;
        TeraMonsterPicture monster;
        TeraScenePicture scenery;
        TeraTunnelFrame tunnel;
        struct {int x,y;unsigned ms;} ray;
        struct {unsigned command,choice,actor;int outcome,east,north;} resident;
        struct {TeraActorEffect reason;unsigned id;int hp,magic;char condition[3];} actor;
    } data;
} TeraPresentationEvent;
typedef struct {
    void (*emit)(void *context,const TeraPresentationEvent *event);
    void *context;
} TeraPresentationSink;

struct TeraCombatGame;
/* Helpers internes aux modules, sans tirage/effet de jeu. make initialise
 * tous les champs a zero et photographie le lieu ; emit appelle le sink. */
TeraPresentationEvent tera_presentation_make(const struct TeraCombatGame *game,
    TeraPresentationKind kind,TeraPresentationSource source);
void tera_presentation_emit(const struct TeraCombatGame *game,const TeraPresentationEvent *event);
void tera_presentation_message(const struct TeraCombatGame *game,TeraPresentationSource source,
    TeraTextResource resource,unsigned id,unsigned variant);
void tera_presentation_move(const struct TeraCombatGame *game,TeraPresentationSource source,
    unsigned command,int from,int moved,unsigned wall);
void tera_presentation_zone(const struct TeraCombatGame *game,TeraPresentationSource source,
    unsigned from_world,int from_position,int from_alfol);
void tera_presentation_objective(const struct TeraCombatGame *game,TeraPresentationSource source,unsigned id);
void tera_presentation_actor(const struct TeraCombatGame *game,TeraPresentationSource source,
    TeraActorEffect reason,unsigned id);
/* Resultat transitoire d'une interaction residente, sans effet de jeu. */
void tera_presentation_interaction(const struct TeraCombatGame *game,TeraPresentationSource source,
    TeraInteractionEffect effect,int amount);
#endif
