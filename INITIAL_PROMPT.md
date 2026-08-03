# Prompt de reprise — à coller dans Claude Code

Copie-colle ce qui suit comme premier message dans Claude Code, dans le
dossier de ce projet :

---

Ce projet reconstruit TERA (LORICIELS, 1986), un RPG français dont j'ai
retrouvé le code source décompilé et le binaire original. Lis d'abord
CLAUDE.md, puis docs/TERA_notes_moteur.md (tout ce qu'on a confirmé du
moteur original par lecture de code et désassemblage x86) et
docs/TERA_remake_blueprint.md (l'architecture cible du remake).

Le squelette Python dans src/tera/ contient déjà les modèles de données,
une génération de donjon partielle, et la formule de combat complète.
Lance tests/test_smoke.py pour vérifier que ça tourne.

Pour cette session, je veux qu'on avance sur : [PRÉCISE ICI CE QUE TU
VEUX FAIRE — par exemple : "compléter le placement des marqueurs
uniques par étage dans worldgen.py (clé, autel, escalier, Arioch)
d'après la section 5.3 des notes moteur", ou "commencer la boucle de
jeu textuelle en ligne de commande", ou "écrire le premier rendu
Pygame pour la vue de couloir en 1ère personne"].

Respecte les seuils numériques et l'équilibrage déjà présents dans
combat.py et worldgen.py sauf si je demande explicitement de les changer
— ils reproduisent fidèlement l'original de 1986.

---

## Suggestions de prochaines étapes (à choisir ou adapter)

1. **Finir `worldgen.py`** : porter la logique complète de placement
   des marqueurs uniques par étage (clé, autel de soin, escaliers,
   position d'Arioch) et la boucle d'auto-validation, décrites dans
   `docs/TERA_notes_moteur.md` section 5.3.
2. **Boucle de jeu textuelle** : une interface en ligne de commande
   pour explorer un donjon généré, ouvrir des coffres, déclencher des
   combats — sans aucun rendu graphique, juste pour valider que le
   moteur de données est solide.
3. **Sauvegarde/chargement** : sérialiser `SaveGame` en JSON.
4. **Rendu** : commencer par la vue du dessus (Amarande/Alfol) qui est
   plus simple que le couloir en 1ère personne.
5. **Les 4 tâches** : implémenter la logique de victoire complète
   (pyramide, Meduz, Arioch, Autel des Ténèbres) telle que documentée.
