"""Modeles de donnees pour le remake de TERA.

Bases sur la structure tSavedata reconstituee dans docs/TERA_notes_moteur.md.
"""
from __future__ import annotations
from dataclasses import dataclass, field

STATS = [
    "force", "intelligence", "chance", "endurance",
    "volonte", "vitalite", "agilite", "charme", "habilete",
]

# L'association verbe de combat -> action_type/stat vit maintenant dans
# combat.py (ACTION_TYPES, STAT_BY_INDEX), confirmee par le desassemblage
# de C_842A (docs/TERA_notes_moteur.md section 17).


@dataclass
class Character:
    name: str
    gender: str  # 'H' ou 'F'
    stats: dict[str, int] = field(default_factory=dict)
    skills: dict[str, int] = field(default_factory=dict)  # 0 = non forme
    hp: int = 10
    mp: int = 10
    max_hp: int = 10
    max_mp: int = 10
    equipment: list[str] = field(default_factory=list)  # jusqu'a 10 emplacements
    state: str = "OK"  # 'OK' / 'HS' / 'EM'
    in_party: bool = False


@dataclass
class MapCell:
    walls: dict[str, int] = field(default_factory=lambda: {"N": 0, "S": 0, "E": 0, "O": 0})
    chest: int | None = None   # voir table des codes dans docs/TERA_notes_moteur.md section 5.3
    encounter: int | None = None  # id de monstre (1-22)
    extra: int = 0             # 7e attribut, role encore incertain dans l'original


@dataclass
class World:
    id: int
    cells: list[MapCell] = field(default_factory=list)  # 100 cellules, grille 10x10
    pos_arioch: int | None = None  # case (0-99) d'Arioch, uniquement sur son etage

    def cell(self, pos: int) -> MapCell:
        return self.cells[pos]


@dataclass
class Monster:
    id: int
    name: str
    hp_base: int
    resist_a: int
    resist_b: int
    weakness_item_id: int | None
    initiative: int
    hp_variance: int


@dataclass
class SaveGame:
    characters: list[Character] = field(default_factory=list)
    worlds: dict[int, World] = field(default_factory=dict)
    position: int = 0
    world_id: int = 0
    money: int = 500
    energy: int = 50
    difficulty: int = 5
    days: int = 0
    tasks_done: list[bool] = field(default_factory=lambda: [False, False, False, False])
    tasks_counter: int = 0
    secret_words: list[str] = field(default_factory=list)  # 4 mots secrets
    hint_strings: list[str] = field(default_factory=list)
    keys: list[int] = field(default_factory=lambda: [0] * 10)
    has_map: list[bool] = field(default_factory=lambda: [False] * 11)
    pos_arioch: int | None = None

    # Amarande (section 19-20 des notes moteur) : pas un World/MapCell,
    # juste des positions absolues 1000-1899 et des chaines de montagnes.
    pos_h_mountains: list[int] = field(default_factory=list)  # 9 valeurs, bloquent N/S
    pos_v_mountains: list[int] = field(default_factory=list)  # 25 valeurs, bloquent E/O
    loc_towns: list[int] = field(default_factory=list)        # 20 villes
    amarande_position: int = 1000
    alfol_surface_position: int = 0    # 0-99, pas de murs (section 5.3/19)
    meditated: bool = False            # Sav._9418 -- buff Meduz, une seule fois
    location: str = "amarande"         # "amarande" / "alfol_surface" / "dungeon"

    @property
    def victory(self) -> bool:
        return all(self.tasks_done)

    @property
    def party_alive(self) -> bool:
        return any(c.in_party and c.state != "HS" for c in self.characters)
