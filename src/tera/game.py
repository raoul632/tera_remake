"""Boucle de jeu textuelle (CLI) -- explore Amarande, Alfol, Meduz et la
Cite des Cranes, ouvre des coffres, declenche des combats. Valide le
moteur de donnees sans rendu, comme le recommande le blueprint
(section 7, etape 2).

Portee assumee : le Temple de Seth (tache 4) n'a pas de generateur
ecrit cette session -- tache 4 reste injouable. Les points de passage
entre Amarande et la Cite des Cranes/la Pyramide/Alfol sont des
positions FIXES choisies pour ce port (AMARANDE_CITE_ENTRANCE etc.),
pas les positions aleatoires (Sav.f_LocExtra) de l'original -- cette
partie-la depend d'un systeme de recrutement de PNJ non implemente
(docs/TERA_notes_moteur.md section 20.3). Voir CLAUDE.md pour l'etat
d'avancement complet.
"""
from __future__ import annotations
import random
import sys
from typing import Callable

from .models import Character, SaveGame
from .worldgen import (
    generate_dungeon, generate_alfol_base, generate_meduz,
    CHEST_KEY_BASE, CHEST_MAP, CHEST_STAIRS_UP, CHEST_STAIRS_DOWN, CHEST_ALTAR,
    CHEST_BUREAU_MEDIAS, CHEST_RADIOACTIF, CHEST_MEDITER, CHEST_INVOQUER,
    EXIT_POSITION,
)
from .amarande import (
    generate_mountains, generate_towns, generate_secret_words, try_move as amarande_try_move,
    direction_hint as amarande_direction_hint,
    AMARANDE_CITE_ENTRANCE, AMARANDE_PYRAMIDE, AMARANDE_ALFOL, ALFOL_BASE_ENTRANCE,
)
from .combat import (
    attack_power, monster_resistance, spawn_monster_hp, initiative_check,
    negotiation_roll, ACTION_TYPES, DYNAMIC_ITEM_ACTIONS,
)
from .data_loader import get_monsters, get_objects
from .save import save_to_file, load_from_file

DUNGEON_FLOORS = 8

# Toute question posee au joueur (choix d'action, mot secret, oui/non) et
# tout message affiche passent par ces deux callbacks plutot que par des
# input()/print() en dur -- confirme par le manuel officiel (section 18
# des notes moteur) : l'original n'a JAMAIS de console separee, tout se
# passe dans le meme ecran graphique avec une seule ligne de commande en
# bas. Par defaut (CLI), ask=input et log=print ; render/app.py injecte
# ses propres versions (ligne de saisie/journal dans la fenetre Pygame).
Ask = Callable[[str], str]
Log = Callable[[str], None]
# Callback optionnel, appele une fois avec le Monster au tout debut d'un
# combat -- permet a render/app.py d'afficher un sprite pendant que le
# reste de la boucle de combat reste purement textuel (ask/log). No-op
# par defaut (CLI).
OnMonster = Callable[[object], None]

DIRECTIONS = {"n": "N", "s": "S", "e": "E", "o": "O"}
DIR_DELTA = {"N": -10, "S": 10, "E": 1, "O": -1}  # donjons (grille 10x10)
COMBAT_ACTIONS = list(ACTION_TYPES.keys())

HELP_DUNGEON = (
    "n/s/e/o, ouvrir, invoquer, combattre, monter, descendre, sortir, "
    "inventaire, quitter et sauver, charger, aide"
)
HELP_AMARANDE = (
    "n/s/e/o, entrer (Cite des Cranes/Alfol/ville), monter (Pyramide), "
    "inventaire, quitter et sauver, charger, aide"
)
HELP_ALFOL_SURFACE = "n/s/e/o, entrer (base), sortir (vers Amarande), inventaire, aide"
HELP_SHOP = "acheter <lettre>, vendre <nom>, hopital, sortir, inventaire, aide"


BONUS_LETTERS = {
    "F": "force", "I": "intelligence", "N": "chance",
    "E": "endurance", "V": "volonte", "H": "habilete",
    "A": "agilite", "C": "charme", "T": "vitalite",
}


def _roll_primary_stats(rng: random.Random) -> tuple[int, int, int]:
    """Port exact de OVL_0022.PAS : Force=RANDOM(5)+5 (5-9), Intelligence
    depend de Force (RANDOM(3)+3 si Force>7, sinon RANDOM(6)+4), Chance
    = le reste (somme totale toujours 18)."""
    force = rng.randrange(5) + 5
    if force > 7:
        intelligence = rng.randrange(3) + 3
    else:
        intelligence = rng.randrange(6) + 4
    chance = 18 - force - intelligence
    return force, intelligence, chance


def _split_stat(total: int, label: str, ask: Ask, log: Log, rng: random.Random) -> int:
    """Port de la repartition d'une stat primaire (OVL_0022.PAS) : le
    joueur tape combien de points vont a la 1ere sous-stat (1 a total-1),
    le reste va a la seconde. Reponse hors bornes -> tirage aleatoire
    ("tres drole..", comme l'original)."""
    raw = ask(f"Combien de points de {label} (1-{total - 1}) ? ").strip()
    try:
        value = int(raw)
        if not (1 <= value <= total - 1):
            raise ValueError
    except ValueError:
        log("tres drole..")
        value = rng.randrange(total - 1) + 1
    return value


def _create_one_character(rng: random.Random, ask: Ask, log: Log) -> Character:
    name = ask("Inscris son nom : ").strip() or "Aventurier"
    gender = ""
    while gender not in ("H", "F"):
        gender = ask("H(omme) ou F(emme) ? ").strip().upper()

    force, intelligence, chance = _roll_primary_stats(rng)
    log(f"{name} a pour qualites principales : Force={force}  Intelligence={intelligence}  Chance={chance}")

    endurance = _split_stat(force, "ENDURANCE (endurance/agilite)", ask, log, rng)
    volonte = _split_stat(intelligence, "VOLONTE (volonte/charme)", ask, log, rng)
    vitalite = _split_stat(chance, "VITALITE (vitalite/habilete)", ask, log, rng)

    stats = {
        "force": force, "intelligence": intelligence, "chance": chance,
        "endurance": endurance, "agilite": force - endurance,
        "volonte": volonte, "charme": intelligence - volonte,
        "vitalite": vitalite, "habilete": chance - vitalite,
    }
    if gender == "H":
        stats["force"] += 2
    else:
        stats["charme"] += 2

    remaining = 6
    while remaining > 0:
        letter = ask(f"Bonifie une qualite (F/I/N/E/V/H/A/C/T -- {remaining} restants) : ").strip().upper()
        if letter in BONUS_LETTERS:
            stats[BONUS_LETTERS[letter]] += 1
            remaining -= 1
        else:
            log("?")

    return Character(
        name=name, gender=gender, stats=stats,
        hp=20, mp=10, max_hp=20, max_mp=10, in_party=True,
    )


def create_party(rng: random.Random, ask: Ask = input, log: Log = print) -> list[Character]:
    """Port de OVL_0022.PAS, y compris la boucle de reroll complet
    ("gardes-tu ce personnage ?")."""
    while True:
        char = _create_one_character(rng, ask, log)
        log(f"{char.name} cree ({char.gender}) -- {char.stats}")
        keep = ask("Gardes-tu ce personnage ? (o/n) : ").strip().lower()
        if keep != "n":
            return [char]


def dungeon_here_lines(world, pos: int, world_id: int) -> list[str]:
    """Ce qu'il y a a voir/faire sur la case actuelle d'un donjon --
    factorise pour etre partage entre `describe_cell` (CLI, inclut aussi
    les passages/murs) et le HUD du rendu Pygame (les murs sont deja
    visibles dans la vue couloir, inutile de les repeter en texte)."""
    cell = world.cell(pos)
    lines = []
    if cell.chest == CHEST_STAIRS_UP:
        lines.append("Un escalier monte ici.")
    elif cell.chest == CHEST_STAIRS_DOWN:
        lines.append("Un escalier descend ici.")
    elif cell.chest == CHEST_ALTAR:
        lines.append("Un autel de soin se dresse ici.")
    elif cell.chest == CHEST_MAP:
        lines.append("Une carte du niveau semble trainer ici.")
    if world_id == 9:
        if cell.chest == CHEST_BUREAU_MEDIAS:
            lines.append("Le BUREAU DES MEDIAS est ici (essaie 'payer').")
        elif cell.chest == CHEST_RADIOACTIF:
            lines.append("Un depot de produits radioactifs...")
    elif world_id == 10:
        if cell.chest == CHEST_MEDITER:
            lines.append("Un lieu propice a la meditation (essaie 'mediter').")
        elif cell.chest == CHEST_INVOQUER:
            lines.append("Un lieu propice a l'invocation (essaie 'invoquer').")
    elif cell.chest is not None:
        lines.append("Il y a quelque chose ici (essaie 'ouvrir').")
    if world_id == 0 and pos == EXIT_POSITION:
        lines.append("La sortie vers Amarande est ici (essaie 'sortir').")
    if pos == 0 and world_id in (9, 10):
        lines.append("La sortie est ici (essaie 'sortir').")
    if world.pos_arioch == pos:
        lines.append("Une presence malefique emplit cet endroit... Arioch ?")
    if cell.encounter is not None:
        lines.append("Un monstre te barre la route !")
    return lines


def describe_cell(world, pos: int, world_id: int) -> str:
    cell = world.cell(pos)
    open_dirs = sorted(d for d, code in cell.walls.items() if code != 0)
    lines = [f"Etage {world_id}, case {pos}. Passages : {', '.join(open_dirs) or 'aucun'}."]
    lines.extend(dungeon_here_lines(world, pos, world_id))
    return "\n".join(lines)


def _apply_trap(effect: str, party: list[Character], rng: random.Random) -> None:
    present = [c for c in party if c.in_party and c.state != "HS"]
    if not present:
        return
    if effect == "gaz":
        for c in present:
            c.state = "EM"
    elif effect == "explosion":
        for c in present:
            c.hp = max(0, c.hp - rng.randint(1, 6))
    elif effect == "cyanure":
        rng.choice(present).state = "EM"
    elif effect == "lame":
        target = rng.choice(present)
        target.hp = max(0, target.hp - rng.randint(1, 6))


def resolve_chest(save: SaveGame, party: list[Character], world, pos: int, rng: random.Random) -> str:
    """Port de la logique de coffre/piege, section 5.3 des notes moteur."""
    cell = world.cell(pos)
    code = cell.chest
    if code is None:
        return "Il n'y a rien a ouvrir ici."
    if code in (CHEST_STAIRS_UP, CHEST_STAIRS_DOWN, CHEST_ALTAR):
        return "Ce n'est pas un coffre (utilise 'monter'/'descendre'/'invoquer')."
    if code == 0:
        cell.chest = None
        return "Le coffre est vide."

    actor = party[0]
    # Agilite + Habilete determinent le desamorcage -- confirme par le
    # manuel officiel (manuel/Tera.pdf p.4).
    roll = (
        actor.stats.get("agilite", 0) + actor.stats.get("habilete", 0)
        + actor.skills.get("desamorcage", 0)
    ) * (1 + rng.randrange(4))
    trap_msg = ""
    if roll <= 11 + save.difficulty:
        effect = rng.choice(["gaz", "explosion", "cyanure", "lame"])
        _apply_trap(effect, party, rng)
        trap_msg = f" Un piege se declenche : {effect} !"

    if code == CHEST_MAP:
        save.has_map[save.world_id] = True
        cell.chest = None
        return f"Tu trouves une carte du niveau.{trap_msg}"
    if CHEST_KEY_BASE <= code <= 98:
        key_idx = code - CHEST_KEY_BASE
        save.keys[key_idx] += 1
        cell.chest = None
        return f"Tu trouves la cle n{key_idx}.{trap_msg}"
    if 26 <= code <= 89:
        gold = code - 20
        save.money += gold
        cell.chest = None
        return f"Tu trouves {gold} ducats.{trap_msg}"
    if 1 <= code <= 25:
        objects = get_objects()
        obj_name = objects[code - 1] if code - 1 < len(objects) else f"objet #{code}"
        actor.equipment.append(obj_name)
        cell.chest = None
        return f"Tu trouves {obj_name}.{trap_msg}"
    cell.chest = None
    return f"Tu trouves quelque chose d'indefini (code {code}).{trap_msg}"


def resolve_altar(party: list[Character], rng: random.Random) -> str:
    """Port de C_8315(perso,6,6) > 0 sur l'autel de soin, section 5.3."""
    lines = []
    for c in party:
        if not c.in_party or c.state == "HS":
            continue
        if attack_power(c, "invoquer", party, required_item=None, rng=rng) > 0:
            c.hp, c.mp = c.max_hp, c.max_mp
            lines.append(f"{c.name} est soigne(e) par l'autel.")
        else:
            lines.append(f"{c.name} ne ressent rien.")
    return "\n".join(lines) or "Personne n'est present pour invoquer."


def resolve_meduz_invoquer(save: SaveGame, ask: Ask = input) -> str:
    """Tache 1 (docs/TERA_notes_moteur.md 19.3) : mediter d'abord, puis
    invoquer SecretWords[3] (Pascal 1-based) = save.secret_words[2] ici."""
    word = ask("Invoquer qui ? ").strip()
    if word != save.secret_words[2]:
        return "Rien ne se passe."
    if not save.meditated:
        return "Il faut d'abord mediter."
    if not save.tasks_done[0]:
        save.tasks_done[0] = True
        save.tasks_counter += 1
        return f"La TRANSCENDANCE evolue vers l'harmonie. Tache 1 accomplie ! ({save.tasks_counter}/4)"
    return "Rien de plus ne se passe."


def resolve_bureau_medias(save: SaveGame, ask: Ask = input) -> str:
    """Tache 2 (docs/TERA_notes_moteur.md 19.2) : payer 1000 ducats."""
    if save.tasks_done[1]:
        return "Tu as deja regle cette affaire."
    choice = ask("Etes-vous prets a investir 1000 ducats ? (o/n) : ").strip().lower()
    if choice != "o":
        return "Tu repars sans rien faire."
    if save.money < 1000:
        return "Un cheque en bois ! quel toupet !"
    save.money -= 1000
    save.tasks_done[1] = True
    save.tasks_counter += 1
    return f"Voila une tache accomplie parmi les 4 qui vous incombent ! ({save.tasks_counter}/4)"


# Commerce de ville (port de OVL_00BC::__Buying/__Selling, section 7 des
# notes moteur et mapping_fonctions_pascal_python.md). Prix d'achat
# confirmes texto par MESSAGE[72-90] (a-s) ET recopies depuis le CASE
# 'A'..'S' d'__Buying (memes valeurs, memes objets). **Simplification
# assumee** : toutes les villes d'Amarande (les 20 marqueurs
# save.loc_towns) partagent cette meme boutique generique -- le
# dispatcher d'entree en ville exact par ville (`C_7309`, index de ville
# >9) n'a jamais ete decode (section 19.1 des notes moteur), donc aucune
# distinction de contenu par ville n'est reproductible avec certitude.
# Objets "speciaux" (energie/cartes) restent hors de l'inventaire
# classique, non modelises (pas de sous-systeme minijeu spatial/cartes de
# niveau dans ce port) -- seule la cle n 9 beneficie du systeme
# `save.keys` deja en place, avec la valeur fixe confirmee (`:= 9`, pas
# un increment) portee dans `handle_shop_command`.
SHOP_ITEMS = [
    ("a", "revolver", 100),
    ("b", "desintegrateur", 500),
    ("c", "armure spatiale", 1000),
    ("d", "conteneur d'energie", 50),
    ("e", "carte des canaux de MEDUZ", 100),
    ("f", "plan de la base d'ALFOL", 100),
    ("g", "epee", 25),
    ("h", "hache", 50),
    ("i", "arc", 75),
    ("j", "elixir", 90),
    ("k", "contrepoison", 40),
    ("l", "cle n9", 25),
    ("m", "cuir", 90),
    ("n", "cuirasse", 300),
    ("o", "cristal", 200),
    ("p", "talisman", 250),
    ("q", "anneau", 300),
    ("r", "pierre", 350),
    ("s", "amulette", 750),
]

# Prix de RACHAT fixes, CONFIRMES par `__ComputeSellingPrice` (table de 25
# valeurs par id d'objet, `OVL_00BC.PAS`) -- table entierement distincte de
# celle d'achat ci-dessus (recoupee via le CASE 'A'..'S' d'__Buying pour
# retrouver quel id d'objet correspond a quel nom). Les 4 objets "speciaux"
# (energie/cartes/cle 9) n'ont pas de prix de rachat dans l'original (ils
# ne transitent jamais par l'inventaire d'equipement classique) -- absents
# de cette table, donc invendables ici aussi.
SELL_PRICES = {
    "revolver": 50, "desintegrateur": 250, "armure spatiale": 500,
    "epee": 15, "hache": 25, "arc": 40, "elixir": 45, "contrepoison": 20,
    "cuir": 45, "cuirasse": 150, "cristal": 100, "talisman": 125,
    "anneau": 150, "pierre": 175, "amulette": 375,
}


def _buy_price(base_price: int, negotiator: Character, rng: random.Random) -> int:
    """Port exact de __Buying (section 7) : `prix*30 / (30 + jet/5)` --
    le jet est `C_8315(perso,8,8)` (`negociation_roll`), PAS la stat
    Charme brute comme une hypothese precedente le supposait (le jet
    inclut le multiplicateur aleatoire et le seuil de succes du combat,
    applique au Charme pour la stat et la competence)."""
    roll = negotiation_roll(negotiator, rng)
    return base_price * 30 // (30 + roll // 5)


def _sell_price(name: str, seller: Character, rng: random.Random) -> int | None:
    """Port exact de __Selling/__ComputeSellingPrice (section 7) :
    `prix_rachat + (prix_rachat DIV 100) * C_8315(perso,8,8)`, a partir
    de la table `SELL_PRICES` fixe. Renvoie None si l'objet n'a pas de
    prix de rachat dans l'original (objets "speciaux")."""
    base = SELL_PRICES.get(name)
    if base is None:
        return None
    roll = negotiation_roll(seller, rng)
    return base + (base // 100) * roll


def describe_shop(save: SaveGame) -> str:
    lines = [f"Boutique de la ville -- {save.money} ducats en poche."]
    for letter, name, price in SHOP_ITEMS:
        lines.append(f"[{letter}] {name} -- {price} ducats (prix affiche, negocie au moment de l'achat)")
    lines.append("acheter <lettre> / vendre <nom> / hopital / sortir")
    return "\n".join(lines)


def handle_shop_command(
    save: SaveGame, party: list[Character], cmd: str, rng: random.Random, ask: Ask = input,
) -> str | None:
    present = [c for c in party if c.in_party and c.state != "HS"]

    if cmd == "sortir":
        save.location = "amarande"
        return "Tu quittes la ville."

    if cmd == "hopital":
        if not present:
            return "Personne a soigner."
        # Port exact du corps principal d'OVL_00BC.PAS : le cout est base
        # sur le personnage le PLUS blesse (pas la somme des PV manquants),
        # +1 semaine, et soigne TOUT le groupe (PV + etats) d'un coup.
        missing = max((c.max_hp - c.hp for c in present), default=0)
        if missing <= 0:
            return "Personne n'a besoin de soins."
        weeks = missing + 1
        for c in present:
            c.hp = c.max_hp
            c.state = "OK"
        save.days += 7 * weeks
        return f"Le groupe est soigne par la SECU -- {weeks} semaine(s) ont passe."

    if cmd.startswith("acheter"):
        if not present:
            return "Personne pour acheter."
        parts = cmd.split(maxsplit=1)
        letter = parts[1].strip() if len(parts) > 1 else ask("Acheter quoi (lettre) ? ").strip().lower()
        item = next((i for i in SHOP_ITEMS if i[0] == letter), None)
        if item is None:
            return "Cet objet n'existe pas."
        negotiator = present[0]
        if len(present) > 1:
            chosen = ask(f"Qui negocie ({'/'.join(c.name for c in present)}) ? ").strip()
            negotiator = next((c for c in present if c.name == chosen), negotiator)
        _, name, base_price = item
        price = _buy_price(base_price, negotiator, rng)
        if save.money < price:
            return "Pas assez de ducats."
        save.money -= price
        if name == "cle n9":
            save.keys[9] = 9  # valeur fixe confirmee (Sav.f_Keys[9] := 9), pas un increment
        else:
            negotiator.equipment.append(name)
        return f"{negotiator.name} achete {name} pour {price} ducats."

    if cmd.startswith("vendre"):
        if not present:
            return "Personne pour vendre."
        parts = cmd.split(maxsplit=1)
        obj_name = parts[1].strip() if len(parts) > 1 else ask("Vendre quoi ? ").strip()
        if obj_name not in SELL_PRICES:
            return "La boutique n'achete pas cet objet."
        seller = next((c for c in present if obj_name in c.equipment), None)
        if seller is None:
            return "Personne ne possede cet objet."
        price = _sell_price(obj_name, seller, rng)
        seller.equipment.remove(obj_name)
        save.money += price
        return f"{seller.name} vend {obj_name} pour {price} ducats."

    return None


def _monster_turn(monster, present: list[Character], rng: random.Random, log: Log = print) -> None:
    """Attaque du monstre sur le groupe.

    AUCUNE formule d'attaque de monstre n'a ete retrouvee dans le
    desassemblage (docs/TERA_notes_moteur.md section 17). Mecanisme
    PROVISOIRE, explicitement non fidele, juste pour que le combat soit
    jouable.
    """
    target = rng.choice(present)
    dmg = max(0, monster.hp_base // 3 + rng.randint(0, max(0, monster.hp_variance - 1))
              - target.stats.get("agilite", 0) // 3)
    target.hp = max(0, target.hp - dmg)
    log(f"{monster.name} attaque {target.name} pour {dmg} degats.")
    if target.hp == 0:
        target.state = "HS"
        log(f"{target.name} est hors combat.")


def _party_turn(
    present: list[Character], monster, monster_hp: int, rng: random.Random,
    ask: Ask = input, log: Log = print,
) -> tuple[int, bool]:
    for actor in present:
        log(f"PV de {monster.name} : {max(monster_hp, 0)}")
        action = ""
        while action not in COMBAT_ACTIONS and action != "fuir":
            action = ask(f"{actor.name}, action ({'/'.join(COMBAT_ACTIONS)}/fuir) : ").strip()
        if action == "fuir":
            return monster_hp, True

        required_item = None
        if action in DYNAMIC_ITEM_ACTIONS:
            item = ask(f"  objet utilise pour '{action}' (vide = aucun) : ").strip()
            required_item = item or None

        dmg = attack_power(actor, action, present, required_item=required_item, rng=rng)
        dmg = max(0, dmg - monster_resistance(monster, None))
        if dmg > 0:
            monster_hp -= dmg
            log(f"{actor.name} inflige {dmg} degats.")
        else:
            log(f"{actor.name} rate son action.")
        if monster_hp <= 0:
            return monster_hp, False
    return monster_hp, False


def run_combat(
    save: SaveGame, party: list[Character], world, pos: int, rng: random.Random,
    ask: Ask = input, log: Log = print, on_monster: OnMonster | None = None,
) -> str:
    cell = world.cell(pos)
    monster = {m.id: m for m in get_monsters()}.get(cell.encounter)
    if monster is None:
        return "Le monstre a disparu."
    if on_monster is not None:
        on_monster(monster)
    monster_hp = spawn_monster_hp(monster, rng)
    log(f"Combat contre {monster.name} (PV {monster_hp}) !")

    while True:
        present = [c for c in party if c.in_party and c.state != "HS"]
        if not present:
            return "Le groupe est hors combat..."

        group_first = initiative_check(present, monster, rng)
        if not group_first:
            _monster_turn(monster, present, rng, log=log)
            present = [c for c in party if c.in_party and c.state != "HS"]
            if not present:
                return "Le groupe est hors combat..."

        monster_hp, fled = _party_turn(present, monster, monster_hp, rng, ask=ask, log=log)
        if fled:
            return "Vous fuyez le combat."
        if monster_hp <= 0:
            break

        if group_first:
            _monster_turn(monster, present, rng, log=log)

    cell.encounter = None
    msg = f"{monster.name} est vaincu !"
    if world.pos_arioch == pos:
        save.tasks_done[2] = True
        save.tasks_counter += 1
        msg += f"\nArioch est tombe. Tache 3 accomplie ! ({save.tasks_counter}/4)"
    return msg


def handle_encounter(
    save: SaveGame, party: list[Character], world, pos: int, previous_pos: int, rng: random.Random,
    ask: Ask = input, log: Log = print, on_monster: OnMonster | None = None,
) -> None:
    """Declenche une "mauvaise rencontre" des qu'on entre sur une case
    occupee par un monstre -- confirme par le manuel officiel."""
    monster = {m.id: m for m in get_monsters()}.get(world.cell(pos).encounter)
    name = monster.name if monster else "quelque chose"
    if monster is not None and on_monster is not None:
        on_monster(monster)
    log(f"Mauvaise rencontre : {name} te barre la route !")
    while True:
        choice = ask("negocier / combattre / fuir : ").strip().lower()
        if choice in ("negocier", "n"):
            log("Le monstre ne semble pas dispose a negocier.")
            continue
        if choice in ("combattre", "c"):
            log(run_combat(save, party, world, pos, rng, ask=ask, log=log, on_monster=on_monster))
            return
        if choice in ("fuir", "f"):
            save.position = previous_pos
            log("Tu bats en retraite.")
            return
        log("Reponds par negocier, combattre ou fuir.")


def amarande_here_lines(save: SaveGame) -> list[str]:
    """Ce qu'il y a a voir/faire sur la case actuelle d'Amarande --
    factorise pour etre partage entre le CLI (`describe_amarande`) et le
    HUD du rendu Pygame (`render/app.py`)."""
    pos = save.amarande_position
    lines = []
    if pos == AMARANDE_CITE_ENTRANCE:
        lines.append("L'entree de la Cite des Cranes est ici (essaie 'entrer').")
    if pos == AMARANDE_PYRAMIDE:
        lines.append("La Pyramide se dresse ici (essaie 'monter').")
    if pos == AMARANDE_ALFOL:
        lines.append("L'entree de la base d'Alfol est ici (essaie 'entrer').")
    if pos in save.loc_towns:
        lines.append("Une ville est visible ici (essaie 'entrer').")
    return lines


def describe_amarande(save: SaveGame) -> str:
    pos = save.amarande_position
    lines = [f"Amarande, position {pos}."]
    lines.extend(amarande_here_lines(save))
    lines.append(
        f"Repere -- Cite des Cranes: {amarande_direction_hint(pos, AMARANDE_CITE_ENTRANCE)}, "
        f"Pyramide: {amarande_direction_hint(pos, AMARANDE_PYRAMIDE)}, "
        f"Alfol: {amarande_direction_hint(pos, AMARANDE_ALFOL)}"
    )
    return "\n".join(lines)


def handle_amarande_command(save: SaveGame, cmd: str, ask: Ask = input, log: Log = print) -> str | None:
    if cmd in DIRECTIONS:
        d = DIRECTIONS[cmd]
        new_pos, moved = amarande_try_move(save.amarande_position, d, save.pos_h_mountains, save.pos_v_mountains)
        if not moved:
            return "Une montagne infranchissable bloque le passage."
        save.amarande_position = new_pos
        return None
    if cmd == "entrer":
        if save.amarande_position == AMARANDE_CITE_ENTRANCE:
            save.location = "dungeon"
            save.world_id, save.position = 0, 0
            return "Tu entres dans la Cite des Cranes."
        if save.amarande_position == AMARANDE_ALFOL:
            save.location = "alfol_surface"
            save.alfol_surface_position = 0
            return "Tu approches d'Alfol."
        if save.amarande_position in save.loc_towns:
            save.location = "shop"
            return "Tu entres dans la ville."
        return "Il n'y a rien a franchir ici."
    if cmd == "monter":
        if save.amarande_position != AMARANDE_PYRAMIDE:
            return "Il n'y a pas de pyramide ici."
        word = ask("Monter en haut de la pyramide... et y invoquer qui ? ").strip()
        if word != save.secret_words[1]:
            return "Rien ne se passe."
        log("Chute dans le temps..")
        save.location = "dungeon"
        save.world_id, save.position = 10, 0
        return None
    return None


def try_move_dungeon(
    save: SaveGame, party: list[Character], world, direction: str, rng: random.Random,
    ask: Ask = input, log: Log = print, on_monster: OnMonster | None = None,
) -> str | None:
    """Deplacement dans un donjon (Cite des Cranes/base d'Alfol/Meduz).
    Renvoie un message a afficher (mur bloque), ou None si le deplacement
    a reussi -- une rencontre eventuelle est geree via handle_encounter."""
    cell = world.cell(save.position)
    if cell.walls[direction] == 0:
        return "Un mur bloque le passage."
    previous_pos = save.position
    save.position += DIR_DELTA[direction]
    new_cell = world.cell(save.position)
    if new_cell.encounter is not None:
        handle_encounter(save, party, world, save.position, previous_pos, rng, ask=ask, log=log, on_monster=on_monster)
    return None


def try_stairs_up(save: SaveGame, cell) -> str:
    if cell.chest != CHEST_STAIRS_UP:
        return "Pas d'escalier montant ici."
    target_floor = save.world_id - 1
    target_world = save.worlds[target_floor]
    down_pos = next(
        (i for i, c in enumerate(target_world.cells) if c.chest == CHEST_STAIRS_DOWN),
        0,
    )
    save.world_id, save.position = target_floor, down_pos
    return "Tu montes."


def try_stairs_down(save: SaveGame, cell) -> str:
    if cell.chest != CHEST_STAIRS_DOWN:
        return "Pas d'escalier descendant ici."
    save.world_id, save.position = save.world_id + 1, 0
    return "Tu descends."


def try_exit_dungeon(save: SaveGame) -> str:
    if save.world_id == 0 and save.position == EXIT_POSITION:
        save.location = "amarande"
        save.amarande_position = AMARANDE_CITE_ENTRANCE
        return "Tu remontes vers Amarande."
    if save.world_id == 9 and save.position == 0:
        save.location = "alfol_surface"
        save.alfol_surface_position = ALFOL_BASE_ENTRANCE
        return "Tu remontes vers la surface d'Alfol."
    if save.world_id == 10 and save.position == 0:
        save.location = "amarande"
        save.amarande_position = AMARANDE_PYRAMIDE
        save.days += 7
        return "Chute dans le temps.. tu es de retour pres de la Pyramide (une semaine a passe)."
    return "Il faut etre a la sortie de cet etage pour sortir."


def try_mediter(save: SaveGame, party: list[Character], cell) -> str:
    if not (save.world_id == 10 and cell.chest == CHEST_MEDITER):
        return "Il n'y a rien a mediter ici."
    if save.meditated:
        return "Rien de nouveau ne se produit."
    save.meditated = True
    for c in party:
        c.stats["volonte"] = c.stats.get("volonte", 0) + 1
        c.stats["intelligence"] = c.stats.get("intelligence", 0) + 1
    return "Extension des facultes mentales ! (+1 Volonte, +1 Intelligence pour le groupe)"


def handle_alfol_surface_command(save: SaveGame, cmd: str) -> str | None:
    if cmd in DIRECTIONS:
        d = DIRECTIONS[cmd]
        delta = {"N": 1, "S": -1, "E": 10, "O": -10}[d]
        save.alfol_surface_position = (save.alfol_surface_position + delta) % 100
        return None
    if cmd == "entrer":
        if save.alfol_surface_position == ALFOL_BASE_ENTRANCE:
            save.location = "dungeon"
            save.world_id, save.position = 9, 0
            return "Tu entres dans la base d'Alfol."
        return "Il n'y a rien a franchir ici."
    if cmd == "sortir":
        save.location = "amarande"
        save.amarande_position = AMARANDE_ALFOL
        return "Tu repars vers Amarande."
    return None


def main() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")

    rng = random.Random()
    print("=== TERA remake -- boucle de jeu textuelle ===")
    print(
        "NB : le Temple de Seth (tache 4) n'a pas encore de generateur -- "
        "cette tache reste injouable. Les points de passage sur Amarande "
        "sont fixes plutot qu'a leur position aleatoire d'origine "
        "(voir game.py, docstring)."
    )

    party = create_party(rng)
    save = SaveGame(characters=party, difficulty=5)

    print("Generation du monde...")
    save.pos_h_mountains, save.pos_v_mountains = generate_mountains(rng)
    save.loc_towns = generate_towns(rng)
    save.secret_words = generate_secret_words(rng)
    save.worlds = generate_dungeon(num_floors=DUNGEON_FLOORS, difficulty=save.difficulty, rng=rng)
    save.worlds[9] = generate_alfol_base(save.difficulty, rng)
    save.worlds[10] = generate_meduz(rng)

    save.location = "amarande"
    save.amarande_position = AMARANDE_CITE_ENTRANCE

    while True:
        if not save.party_alive:
            print("\nLe groupe est mort. Fin de partie.")
            return
        if save.victory:
            print("\nToutes les taches sont accomplies. Victoire !")
            return

        print()
        if save.location == "amarande":
            print(describe_amarande(save))
            cmd = input("> ").strip().lower()
            if cmd in DIRECTIONS or cmd in ("entrer", "monter"):
                msg = handle_amarande_command(save, cmd)
                if msg:
                    print(msg)
                continue
            if cmd == "aide":
                print(HELP_AMARANDE)
                continue

        elif save.location == "shop":
            print(describe_shop(save))
            cmd = input("> ").strip().lower()
            if cmd == "sortir" or cmd == "hopital" or cmd.startswith("acheter") or cmd.startswith("vendre"):
                msg = handle_shop_command(save, party, cmd, rng)
                if msg:
                    print(msg)
                continue
            if cmd == "aide":
                print(HELP_SHOP)
                continue

        elif save.location == "alfol_surface":
            print(f"Surface d'Alfol, position {save.alfol_surface_position}.")
            if save.alfol_surface_position == ALFOL_BASE_ENTRANCE:
                print("L'entree de la base est ici (essaie 'entrer').")
            cmd = input("> ").strip().lower()
            if cmd in DIRECTIONS or cmd in ("entrer", "sortir"):
                msg = handle_alfol_surface_command(save, cmd)
                if msg:
                    print(msg)
                continue
            if cmd == "aide":
                print(HELP_ALFOL_SURFACE)
                continue

        else:  # "dungeon"
            world = save.worlds[save.world_id]
            cell = world.cell(save.position)
            print(describe_cell(world, save.position, save.world_id))
            cmd = input("> ").strip().lower()

            if cmd in DIRECTIONS:
                msg = try_move_dungeon(save, party, world, DIRECTIONS[cmd], rng)
                if msg:
                    print(msg)
                continue
            elif cmd == "ouvrir":
                print(resolve_chest(save, party, world, save.position, rng))
                continue
            elif cmd == "invoquer":
                if save.world_id == 10 and cell.chest == CHEST_INVOQUER:
                    print(resolve_meduz_invoquer(save))
                elif cell.chest == CHEST_ALTAR:
                    print(resolve_altar(party, rng))
                else:
                    print("Il n'y a rien a invoquer ici.")
                continue
            elif cmd == "mediter":
                print(try_mediter(save, party, cell))
                continue
            elif cmd == "payer":
                if save.world_id == 9 and cell.chest == CHEST_BUREAU_MEDIAS:
                    print(resolve_bureau_medias(save))
                else:
                    print("Il n'y a rien a payer ici.")
                continue
            elif cmd == "combattre":
                if cell.encounter is not None:
                    print(run_combat(save, party, world, save.position, rng))
                else:
                    print("Rien a combattre ici.")
                continue
            elif cmd == "monter":
                print(try_stairs_up(save, cell))
                continue
            elif cmd == "descendre":
                print(try_stairs_down(save, cell))
                continue
            elif cmd == "sortir":
                print(try_exit_dungeon(save))
                continue
            elif cmd == "aide":
                print(HELP_DUNGEON)
                continue

        # commandes communes a tous les lieux
        if cmd == "inventaire":
            for c in party:
                print(f"{c.name} : PV {c.hp}/{c.max_hp} PM {c.mp}/{c.max_mp} -- {c.equipment}")
            print(f"Argent : {save.money}, Cles : {save.keys}, Taches : {save.tasks_counter}/4")
        elif cmd in ("quitter et sauver", "sauver"):
            path = input("Nom du fichier de sauvegarde : ").strip() or "save.json"
            save_to_file(save, path)
            print(f"Partie sauvegardee dans {path}.")
            return
        elif cmd == "charger":
            path = input("Nom du fichier a charger : ").strip() or "save.json"
            save = load_from_file(path)
            party = save.characters
            print("Partie chargee.")
        else:
            print("Commande inconnue. Tape 'aide'.")


if __name__ == "__main__":
    main()
