@echo off
REM Lanceur du projet TERA remake -- double-clic pour demarrer.
REM Regle automatiquement PYTHONPATH (necessaire, "tera" n'est pas un
REM paquet installe) et utilise le bon interpreteur pour chaque mode :
REM `python3` (sans pip/pygame) pour le texte/les tests, `python`
REM (E:\Python\Python310, a pip/pygame) pour le rendu graphique et les
REM outils d'extraction -- voir CLAUDE.md pour le detail de cette
REM particularite de ce poste.

setlocal
cd /d "%~dp0"
set "PYTHONPATH=%~dp0src"

:menu
cls
echo ===============================================
echo   TERA remake -- LORICIELS 1986
echo ===============================================
echo.
echo   1. Jouer (interface graphique, Pygame)
echo   2. Jouer (mode texte, sans dependance)
echo   3. Lancer les tests de fumee
echo   4. Extraire les sprites (.IMA -^> PNG)
echo   5. Regenerer les donnees de jeu (VERBES/OBJETS/...)
echo   Q. Quitter
echo.
set /p choix="Choix : "

if /i "%choix%"=="1" goto graphique
if /i "%choix%"=="2" goto texte
if /i "%choix%"=="3" goto tests
if /i "%choix%"=="4" goto sprites
if /i "%choix%"=="5" goto textdata
if /i "%choix%"=="q" goto fin
goto menu

:graphique
python -m tera.render.app
if errorlevel 1 (
    echo.
    echo Erreur : verifie que pygame est installe pour cet interpreteur
    echo ^(python -m pip install -r requirements.txt^).
    pause
)
goto menu

:texte
python3 -m tera.game
goto menu

:tests
python3 tests\test_smoke.py
pause
goto menu

:sprites
python tools\extract_sprites.py
pause
goto menu

:textdata
python3 tools\extract_text_data.py
pause
goto menu

:fin
endlocal
