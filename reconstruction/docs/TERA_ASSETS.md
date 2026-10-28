# Ressources Original — état actuel (28 octobre 2026)

Le frontend utilise désormais les ressources intégrées à la compilation.
La chaîne PNG/JSON éditables, les commandes et les contraintes d'édition sont
décrites dans [TERA_EMBEDDED_RESOURCES.md](TERA_EMBEDDED_RESOURCES.md).
Les quinze empreintes de référence restent dans
[TERA_ASSET_MANIFEST.md](TERA_ASSET_MANIFEST.md).

Les règles de décodage C sont conservées : CP437, atlas 82 sprites,
92168 pixels, plans entrelacés et lectures dépassant une frontière de sprite,
six pages CGA, transparence par octet nul. Les variantes de palette sont
appliquées par le renderer existant. Le core et les sauvegardes sont inchangés.

Les anciens outils `tools/extract_sprites.py` et `tools/extract_text_data.py`
ont été inspectés sans modification. Le premier décrit à tort la transparence
par pixel noir, bien que son entrelacement des plans corresponde au C ; sa
mention d'absence de chargeur BANNIERE est dépassée par C_DA21. Le second
extrait les chaînes mais perd leur fin inutilisée. Aucun n'est importé par la
nouvelle chaîne, qui est vérifiée contre les décodeurs C et chaque octet DATA.

## Preuves conservées

CONFIRMÉ — PAS/DATA : OVL_00DB, TERA.PAS et TERAMAIN/C_DA21 pour les
formats, tailles, seuils d'échelle, troncatures et copie CGA de la bannière.
CONFIRMÉ — BIN : C5051/C5E04/C60BB pour l'omission des octets nuls,
C4473 et CAFE8 pour les sélecteurs MESSAGE et les réponses aux commandes.
Ces références ne sont pas lues par le jeu compilé.

Les tests SDL vérifient chaque pixel RGBA, les masques des 82 sprites et les
six fonds. Ils prouvent le transfert des données vers le rendu actuel, sans
certifier toutes les compositions, couleurs et temporisations observables du
jeu DOS. Voir le registre existant des écarts et la validation de cette livraison.
