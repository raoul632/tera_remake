# Paquet Windows x64 pour GitHub Releases

Le ZIP contient uniquement le port compile, SDL3.dll, les lanceurs, le manuel,
les notices et un manifeste SHA256. Les quinze donnees originales doivent
etre ajoutees par le joueur. Ne pas empaqueter directement `runtime-only/` :
ce dossier de validation contient des donnees privees et des sauvegardes.

## Construction locale

Depuis la racine du depot, avec GCC UCRT64 et SDL3 disponibles :

```powershell
$env:PATH='E:\msys64\ucrt64\bin;'+$env:PATH
cmake -S reconstruction -B reconstruction/build/release-win64 -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=E:/msys64/ucrt64/bin/gcc.exe -DCMAKE_MAKE_PROGRAM=E:/msys64/ucrt64/bin/mingw32-make.exe -DTERA_BUILD_SDL3=ON -DSDL3_DIR=E:/msys64/ucrt64/lib/cmake/SDL3
cmake --build reconstruction/build/release-win64 --target tera_sdl3 -j 6
./reconstruction/tools/package_release.ps1 -Version 0.1.0 -SdlLicense E:/msys64/ucrt64/share/licenses/SDL3/LICENSE.txt
```

Le script refuse d'ecraser un paquet existant. Il ne publie rien, ne cree
aucun tag et ne copie aucune ressource originale. Sorties dans
`reconstruction/build/releases/` : ZIP et empreinte SHA256 externe.
Le manifeste conserve le commit source, les fichiers et leurs empreintes.
Utiliser un arbre de sources propre pour que ce commit identifie le build.

Lancer_TERA.bat cree une nouvelle partie ; Reprendre_partie.bat utilise
l'option native `--load` pour charger `tera.sav` sans recreer un personnage.
Les deux placent le repertoire de travail et la sauvegarde dans le paquet.

## Verification avant publication

Extraire le ZIP dans un nouveau dossier. Verifier son manifeste et l'absence
de COM, overlay, IMA, donnees originales et sauvegardes. Le port doit signaler
les donnees absentes. Dans cette copie de test uniquement, ajouter les quinze
fichiers locaux requis, reduire PATH a System32 puis executer le smoke du
binaire optimise. Les fichiers de test ne doivent jamais etre reinclus au ZIP.

Lors de la premiere construction optimisee, GCC a signale `strncpy` avec une
troncature volontaire dans world.c. La copie de neuf octets depuis le tampon
deja rempli de zeros est maintenant explicite. Quatre cas RANDOMIZE et douze
mondes complets ont ete compares au binaire avec le core Release : zero ecart.

## Mise en ligne manuelle

Creer une Release GitHub, idealement marquee **pre-release** pour cette version
de test, et y joindre le ZIP et son fichier SHA256. Le depot doit contenir le
commit indique dans MANIFEST.json. Indiquer clairement dans la description :
Windows x64 ; donnees originales non fournies ; les fichiers de sa propre
copie de TERA sont indispensables. Aucun pourcentage de fidelite atteint n'est
annonce. Le manuel et les instructions d'installation sont dans le ZIP.
