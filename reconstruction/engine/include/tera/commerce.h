#ifndef TERA_COMMERCE_H
#define TERA_COMMERCE_H
#include "tera/exploration.h"
#include "tera/inventory.h"
typedef enum { TERA_SHOP_MENU,TERA_SHOP_CONFIRM,TERA_SHOP_ACTOR,
    TERA_SHOP_BUY,TERA_SHOP_SELL,TERA_SHOP_DONE,TERA_SHOP_ERROR } TeraShopPhase;
typedef struct {
    TeraExplorationState state;
    TeraShopPhase phase;
    unsigned command,actor,item,first_menu;
    int16_t price,weeks;
    int outcome; /* 1=achat,2=vente,3=hopital,4=pauvre,5=plein,6=HS. */
} TeraShopSession;
/** OVL00BC complet apres confirmation d'entree de TERAMAIN.
 * Copie le monde ; suspend a chaque lecture ; aucun calendrier quotidien.
 * Prix et compteurs suivent les Integer 16 bits du binaire. */
int tera_shop_start(TeraShopSession *session,const TeraExplorationState *state);
int tera_shop_key(TeraShopSession *session,unsigned key);
/** Catalogue A..S confirme BIN ; retourne 0 hors catalogue. */
int tera_shop_catalog(unsigned key,unsigned *item,int *price);
/** Tarif affiche de revente (liste originale 1..24), avant negociation.
 * Lecture pure ; retour 0 hors liste sans modifier price. */
int tera_shop_sale_quote(unsigned item,int *price);
/** Facades TERAMAIN des dix lieux : conserver les tirages graphiques
 * et musicaux. force=1 redessine Seth au retour, force=0 exige A749.
 * Aucun effet hors Amarande ou pour les panneaux 10..19. Le materiel video est abstrait. */
int tera_places_present(TeraExplorationState *state,int force);
#endif
