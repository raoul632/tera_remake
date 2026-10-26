#ifndef TERA_COMBAT_H
#define TERA_COMBAT_H

#include <stdint.h>

/* Sous-routines numeriques auditees : pas encore un combat complet.
 * Source et statut CONFIRME BIN : docs/spec_combat_actions.md.
 * Le RNG et l'orchestration des tours restent hors scope.
 * Garder les indices Pascal simplifie la comparaison avec les preuves :
 * 1 force, 2 intelligence, 3 chance, 4 endurance, 5 volonte, 6 vitalite,
 * 7 agilite, 8 charme, 9 habilete ; colonne 0 inutilisee.
 * skills utilise les memes indices ; skills[7] designe systeme D. */
typedef struct {
    uint8_t stats[10];
    uint8_t skills[10];
} TeraCombatActor;

/** Calculer le bonus du meilleur objet admissible chez un seul personnage.
 * C_8391/C_8270 : chercher required_item, puis les deux IDs precedents.
 * equipment contient dix IDs originaux (0 = emplacement vide).
 * Retour : 3, 2 ou 1 selon le premier candidat trouve ; 0 si absent,
 * pointeur NULL ou required_item hors 3..25.
 * Aucun partage entre compagnons ni modification d'inventaire.
 * Limite : l'ecriture originale AFCC n'est pas effectuee par cette API. */
unsigned tera_combat_equipment_bonus(const uint8_t equipment[10],
                                     unsigned required_item);

/** Evaluer une branche C_842A, avant resistance et effets de combat.
 * actor : stats et competences du personnage ; action : type interne 1..9.
 * equipment_bonus : bonus deja calcule, borne a 0..3, ignore par 8/9.
 * entry_factor : premier RANDOM(15)+3, toujours requis (3..17).
 * base_factor : second tirage (3..17) pour 1..5/9 ; ignore pour 6..8.
 * Intention : laisser l'appelant gerer les tirages pour tester la formule
 * sans substituer un algorithme RNG non encore valide.
 * Retour : 1 si calcul effectue, 0 si arguments invalides/NULL.
 * En cas de succes, *result recoit le mot signe original, potentiellement
 * negatif apres debordement. En cas d'erreur, *result reste inchange.
 * Un retour 1 ne signifie PAS que l'attaque reussit : *result peut etre 0. */
int tera_combat_action(const TeraCombatActor *actor, unsigned action,
                       unsigned equipment_bonus, unsigned entry_factor,
                       unsigned base_factor, int16_t *result);

/** Calculer C_8315(personnage, stat, competence), sans equipement.
 * stat et skill sont des indices 1..9 ; factor est RANDOM(15)+3 (3..17).
 * Ajoute le niveau brut de systeme D, applique mot16 puis seuil 44/35.
 * Cette fonction ne tire aucun nombre et ne modifie pas actor.
 * Retour : 1 et ecriture de *result si valide ; sinon 0 sans ecriture.
 * Les pointeurs actor/result doivent etre non NULL. */
int tera_combat_base_roll(const TeraCombatActor *actor, unsigned stat,
                         unsigned skill, unsigned factor, int16_t *result);

/* Colonnes 2, 3 et 4 de VALMON : deux resistances et une categorie de
 * reaction. La categorie n'est PAS un ID d'objet de faiblesse. Les autres
 * colonnes du fichier ne sont pas necessaires a cette operation pure. */
typedef struct {
    int16_t resistance_a;
    int16_t resistance_b;
    int16_t reaction_category;
} TeraMonsterReaction;

typedef struct {
    int eligible;       /* Branche reconnue, meme si le jet ne touche pas. */
    int16_t damage;
    int16_t magic_after;
    int16_t monster_hp_after;
    int defeated;       /* Seuil original strict : HP < 3. */
} TeraHeroAttackResult;

/** Resoudre C_8CE3 apres le choix du menu et le calcul C_842A.
 * action_letter : L/T/J/A/D/P/C en majuscules ; toute autre lettre est
 * une action sans branche applicable. raw_action est le resultat signe.
 * secret_word_matches indique le resultat de la comparaison originale,
 * dont la lecture et le traitement de texte restent chez l'appelant.
 * Aucun tirage, inventaire ou acteur n'est modifie. Le cout magique est
 * paye pour une branche admissible, meme quand damage vaut zero.
 * Retour 1 si calcule, 0 pour pointeur NULL, sans ecrire dans result. */
int tera_combat_resolve_hero_attack(const TeraMonsterReaction *monster,
                                   char action_letter, int16_t raw_action,
                                   int16_t magic, int16_t monster_hp,
                                   int secret_word_matches,
                                   TeraHeroAttackResult *result);

typedef struct {
    int16_t damage;
    int poison_draw_required;
    int poisoned;
} TeraMonsterAttackResult;

/** Resoudre C_92FE/C_8C45, sans choisir la cible ni tirer de nombre.
 * monster_type : ID original 1..22 ; monster_hp : HP actuels.
 * raw_defense : resultat C_842A(cible,7) pour ID <15, sinon type 8.
 * Ces actions utilisent leur premier facteur ; type 7 inclut l'armure.
 * Si attaque > defense, l'original consomme RANDOM(11-difficulte), meme
 * pour un monstre non empoisonneur. poison_draw_is_zero est le resultat
 * de ce test fourni par l'appelant ; ignore si aucun tirage requis.
 * poisoned demande l'etat EM, sans l'ecrire. Retour 0 si ID/pointeur
 * invalide, sans ecriture ; sinon 1. Les HP de la cible restent externes. */
int tera_combat_resolve_monster_attack(unsigned monster_type,
                                      int16_t monster_hp, int16_t raw_defense,
                                      int poison_draw_is_zero,
                                      TeraMonsterAttackResult *result);

#endif
