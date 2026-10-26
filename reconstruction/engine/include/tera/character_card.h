#ifndef TERA_CHARACTER_CARD_H
#define TERA_CHARACTER_CARD_H
/* Photographie de presentation autonome, sans pointeurs ni calcul de jeu. */
typedef struct {
    unsigned actor;
    char name[10],condition[3];
    unsigned char stats[10],skills[10];
    int equipment[10];
    int hp,magic,hp_limit,magic_limit,key,days,money,experience;
} TeraCharacterCard;
#endif
