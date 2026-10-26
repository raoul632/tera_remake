#ifndef TERA_INPUT_H
#define TERA_INPUT_H
typedef enum { TERA_INPUT_PURGE, TERA_INPUT_MONSTER15_FRAME } TeraInputBoundary;
/* Service facultatif de clavier brut. PURGE vide les touches tamponnees ;
 * FRAME teste leur presence APRES l'image (1..6), sans les lire. Le callback
 * ne rappelle pas le moteur et ne modifie ni son etat ni son RNG. FRAME
 * peut presenter l'image/duree deja emises avant de sonder le clavier,
 * en continuant de traiter la fermeture de fenetre. PURGE reste immediat.
 * NULL signifie clavier vide. Contexte emprunte a l'appelant : runtime
 * uniquement, jamais une donnee a serialiser dans une sauvegarde. */
typedef struct {
    int (*probe)(void *context,TeraInputBoundary boundary,unsigned frame);
    void *context;
} TeraInputSource;
static inline int tera_input_boundary(const TeraInputSource *input,
    TeraInputBoundary boundary,unsigned frame)
{
    return input && input->probe ? input->probe(input->context,boundary,frame)!=0 : 0;
}
#endif
