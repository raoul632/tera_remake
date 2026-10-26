/* Native COM SOUND/DELAY, seed 1986; audit_progression_sound.py. */
typedef struct {uint32_t hash,seed;unsigned notes,ms;} OriginalProgressionSound;
static const OriginalProgressionSound original_progression_sounds[]={
    {0xb06c78ea,0x7a94c121,621,9315}, /* objective */
    {0x2b8ead38,0xa6c0eb99,207,3105}, /* recruitment */
    {0x912a4f4f,0xef2a27cc,250,2500}, /* meduz_entry */
    {0x4774e005,0xc4168811,248,561}, /* refusal */
    {0x5e53b587,0xd5b34324,146,1378}, /* trap_sweep */
    {0x22b91c63,0xf0aae252,16,32}, /* explosion_sound */
    {0xd0f590b4,0x000007c2,100,3000}, /* meduz_guardians */
};
