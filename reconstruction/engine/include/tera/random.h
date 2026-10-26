#ifndef TERA_RANDOM_H
#define TERA_RANDOM_H
#include <stdint.h>

/* Etat exact des deux mots DS:01FC/01FE, a conserver entre rencontres. */
typedef struct { uint32_t seed; } TeraRandom;
/** RANDOMIZE (0F14) : CX=heure/minute, DX=seconde/centieme DOS.
 * Pas de tirage ni de melange supplementaire. Retour 0 pour heure invalide. */
int tera_random_clock_seed(TeraRandom *rng, unsigned hour, unsigned minute,
                           unsigned second, unsigned hundredth);
/** RANDOM(n), 10DA/10E6 : seed=129*seed+361962E9 modulo 2^32,
 * resultat=(seed>>17)%n. Conserver le biais modulo original.
 * Retour 0 pour pointeur NULL ou borne nulle/>65535, sans mutation. */
int tera_random_draw(TeraRandom *rng, unsigned bound, unsigned *result);
/** C62B7 : effets du bruit sur la graine, sans synthese sonore. duree-1
 * iterations ; un jet RANDOM20 puis, sous seuil, RANDOM2 et RANDOM255/256.
 * Renvoie le nombre de tirages consommes. Parametres des appels BIN seulement. */
unsigned tera_random_sound_noise(TeraRandom *rng, unsigned duration, unsigned threshold);
/** C6496/C6518 : les frequences ne changent pas les trois sequences RNG. */
unsigned tera_random_sound_melody(TeraRandom *rng);
/** C68AD : trois rafales de 60 tirages, bornes 30,90,270. */
unsigned tera_random_sound_alert(TeraRandom *rng);
/** C67D7 : pas longs (3 rafales 500/450) ou rapides (500 tirages). */
unsigned tera_random_sound_steps(TeraRandom *rng, int fast_moves);
/** C6352 : sept segments de musique d'ambiance, frequences sans effet RNG. */
unsigned tera_random_sound_country(TeraRandom *rng);
#endif
