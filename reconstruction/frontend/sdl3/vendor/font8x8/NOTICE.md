# Police IBM PC BIOS / CGA (CP437)

`ibm_cp437.h` contient deux tables bitmap de 256 caractères de 8 × 8 pixels,
avec bit 7 à gauche, sans conversion Unicode ni glyphe « ? » de substitution.
Le mode graphique prend BIOS.F08 ; les fenêtres de texte à 80 colonnes prennent
CGA.F08. Les cadres sont dessinés avec les glyphes CP437.

Source des données : VileR, collection vga-text-mode-fonts, FONTS/PC-IBM,
consultée le 17 octobre 2026 :

- https://github.com/viler-int10h/vga-text-mode-fonts/blob/master/FONTS/PC-IBM/BIOS.F08
- https://github.com/viler-int10h/vga-text-mode-fonts/blob/master/FONTS/PC-IBM/CGA.F08
- Description et provenance : https://github.com/viler-int10h/vga-text-mode-fonts/blob/master/FONTS.TXT

SHA-256 des bitmaps sources (2 048 octets chacun) :

- BIOS.F08 : 75c79a7e7fa423dda67ec6d6d76cec86b63f85677726368750c75b0920ddf319
- CGA.F08 : e1935cd2bfdf0188775e0bcfd9fdc263dcfb17c81fd465ce1c671269f3d6a2a3

Les deux anciennes tables Latin-1 font8x8_basic.h / font8x8_ext_latin.h sont
conservées comme historique, mais ne sont plus utilisées. Elles ont été
publiées dans le domaine public par Daniel Hepper / Marcel Sondaar.

La référence matérielle est IBM PC/XT CGA ; la police appartient à la machine,
pas aux fichiers de TERA. Un clone doté d'une autre ROM peut différer.
