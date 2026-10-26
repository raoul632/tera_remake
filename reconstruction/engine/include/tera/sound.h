#ifndef TERA_SOUND_H
#define TERA_SOUND_H
#include "tera/interactive.h"
/* Emettre une duree de presentation ou une frequence, sans toucher au RNG.
 * hz=0 : silence. ms=0 : ecriture sans DELAY, duree materielle non certifiee. */
void tera_media_wait(const TeraCombatGame *g,unsigned ms);
void tera_media_tone(const TeraCombatGame *g,unsigned hz,unsigned ms);
/* C6121/C6179/C65B5 et effets composes : remplacent leurs tirages jetes,
 * avec le meme ordre/bornes et un score de notes par valeur. */
void tera_sound_sweep(TeraCombatGame *g,unsigned from,unsigned to,unsigned step,unsigned bound);
void tera_sound_burst(TeraCombatGame *g,unsigned count);
void tera_sound_action(TeraCombatGame *g,char action);
void tera_sound_hit(TeraCombatGame *g);
void tera_sound_victory(TeraCombatGame *g);
/* Objectifs : trois melodies originales ; renvoie le total RNG consomme. */
unsigned tera_sound_objective(TeraCombatGame *g);
/* C61D1 : count notes 10*RANDOM(bound), puis silence gap ms. */
unsigned tera_sound_random_tones(TeraCombatGame *g,unsigned bound,unsigned delay,unsigned gap,unsigned count);
void tera_sound_entry_meduz(TeraCombatGame *g);
void tera_sound_refusal(TeraCombatGame *g);
void tera_sound_ending(TeraCombatGame *g,int victory);
/* C62B7 : duration >= 2 ; renvoie les tirages consommes. Les seuils et
 * frequences sont ceux des appelants originaux, jamais des choix SDL. */
unsigned tera_sound_noise(TeraCombatGame *g,unsigned base,unsigned duration,unsigned delay,
    unsigned threshold,unsigned gate,unsigned noise1,unsigned noise2);
/* C6352/C6496/C6518 : rapports originaux positifs, denominateur non nul.
 * alternate choisit C6496 ; sinon C6518(multiplier,numerator,denominator). */
unsigned tera_sound_country(TeraCombatGame *g,unsigned numerator,unsigned denominator);
unsigned tera_sound_melody(TeraCombatGame *g,int alternate,unsigned multiplier,
    unsigned numerator,unsigned denominator);
/* C6221 : base*(100+80*cos(0.2*n))/100, n=count..1, troncature entiere.
 * C666B/C6721 : quatre etages de 1 ms, bornes inclusives 50..1200. */
void tera_sound_wave(TeraCombatGame *g,unsigned base,unsigned delay,unsigned count);
void tera_sound_hyperspace(TeraCombatGame *g,int descending);
unsigned tera_sound_steps(TeraCombatGame *g,int fast);
unsigned tera_sound_alert(TeraCombatGame *g);
#endif
