param(
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+(?:-[a-zA-Z0-9.-]+)?$')]
    [string]$Version = '0.1.0',
    [string]$BuildDir = (Join-Path $PSScriptRoot '../build/release-win64'),
    [string]$OutputRoot = (Join-Path $PSScriptRoot '../build/releases'),
    [Parameter(Mandatory=$true)][string]$SdlLicense
)
# Assemble explicit files only: never copy runtime-only/ or its private data.
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildRoot = (Resolve-Path -LiteralPath $BuildDir).Path
$packageName = "TERA-Original-Port-v$Version-windows-x64"
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
$outputPath = (Resolve-Path -LiteralPath $OutputRoot).Path
$packagePath = Join-Path $outputPath $packageName
$zipPath = "$packagePath.zip"
if((Test-Path -LiteralPath $packagePath) -or (Test-Path -LiteralPath $zipPath)) {
    throw 'Release output already exists; choose a new version or output directory.'
}
foreach($file in @('tera_sdl3.exe','SDL3.dll')) {
    if(!(Test-Path -LiteralPath (Join-Path $buildRoot $file))) {throw "Missing build file: $file"}
}
if(!(Test-Path -LiteralPath $SdlLicense)) {throw 'SDL license file absent'}
$commit = (git -C $projectRoot rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0) {throw 'Cannot identify source commit'}
New-Item -ItemType Directory -Path $packagePath,(Join-Path $packagePath 'data'),(Join-Path $packagePath 'notices') | Out-Null
foreach($file in @('tera_sdl3.exe','SDL3.dll')) {
    Copy-Item -LiteralPath (Join-Path $buildRoot $file) -Destination $packagePath
}
Copy-Item -LiteralPath $SdlLicense -Destination (Join-Path $packagePath 'notices/SDL3-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $projectRoot 'frontend/sdl3/vendor/font8x8/NOTICE.md') -Destination (Join-Path $packagePath 'notices/IBM-CP437-NOTICE.md')
$resources = @('NOMPERS','VALMON','VERBES','OBJETS','NOMON','MESSAGE','MESSAGE2','MESSAGE3',
    'CRAPIM.IMA','ASTRON.IMA','BANNIERE.IMA','FACADE.IMA','KRONO.IMA','MAISONJ.IMA','MAISONS.IMA')
$dataInstructions = @"
DONNEES ORIGINALES A AJOUTER

Cette archive ne contient pas les donnees originales de TERA.
Copiez depuis votre propre copie du jeu les 15 fichiers suivants dans ce
dossier data, sans creer de sous-dossier :

$($resources -join "`r`n")

TERA.COM et TERA.000 ne sont pas necessaires au port.
Ensuite, revenez au dossier parent et lancez Lancer_TERA.bat.
"@
$dataInstructions | Set-Content -LiteralPath (Join-Path $packagePath 'data/LISEZ_MOI.txt') -Encoding utf8
$readme = @"
TERA - Port original C/SDL3 - v$Version - Windows x64

INSTALLATION
1. Extrayez entierement le ZIP dans un dossier ou vous pouvez enregistrer
   des fichiers (par exemple Documents). Ne jouez pas depuis le ZIP.
2. Ajoutez vos 15 donnees originales dans data. Voir data/LISEZ_MOI.txt.
3. Double-cliquez sur Lancer_TERA.bat pour commencer une nouvelle partie.

POUR REPRENDRE
Double-cliquez sur Reprendre_partie.bat. Une sauvegarde tera.sav doit exister.
F5 sauvegarde pendant l'exploration, F9 recharge. F5 remplace l'ancienne partie.
Les sauvegardes restent dans ce dossier. Elles ne sont pas partagees avec
une autre installation du jeu.

COMMANDES DE DEPART
Entree passe le titre. Nom puis Entree, H ou F, trois repartitions de points,
six bonifications, O pour garder le personnage. Fleches pour marcher.
H puis I : fiche du heros. H puis S puis O : fiches des compagnons.
? ou + : aide. F11 : plein ecran. Lisez MANUEL_DEBUTANT.md pour la suite.

CONTENU ET LIMITES
Programme Windows x64 et SDL3.dll fournis. Aucun Python, DOSBox, compilateur,
TERA.COM ou TERA.000 requis pour jouer. Les donnees originales restent requises.
Il s'agit d'une version de test du port original, sans mode Enhanced.
La fidelite globale de 95-98 % est un objectif, pas un pourcentage mesure.
Certains details de presentation et de temporisation restent a comparer.
Une archive de sources GitHub seule n'est pas ce paquet executable.

PROVENANCE
Sources : commit $commit
SDL 3.2.8, bibliotheque fournie avec sa licence dans notices/.
Les polices IBM CP437 integrees sont documentees dans notices/.
Ce projet est une reconstruction moderne ; les donnees du jeu original
ne sont pas incluses dans la distribution.
"@
$readme | Set-Content -LiteralPath (Join-Path $packagePath 'LISEZ_MOI.txt') -Encoding utf8
foreach($resume in @($false,$true)) {
    $launcher = @'
@echo off
setlocal
cd /d "%~dp0"
for %%F in (NOMPERS VALMON VERBES OBJETS NOMON MESSAGE MESSAGE2 MESSAGE3 CRAPIM.IMA ASTRON.IMA BANNIERE.IMA FACADE.IMA KRONO.IMA MAISONJ.IMA MAISONS.IMA) do (
  if not exist "%~dp0data\%%F" (
    echo Donnee originale manquante : %%F
    echo Consultez data\LISEZ_MOI.txt puis ajoutez vos fichiers du jeu.
    pause
    exit /b 2
  )
)
__RESUME_CHECK__
"%~dp0tera_sdl3.exe" --data "%~dp0data" --save "%~dp0tera.sav" __LOAD__ %*
set "tera_exit=%ERRORLEVEL%"
if not "%tera_exit%"=="0" pause
exit /b %tera_exit%
'@
    $check = if($resume) {@'
if not exist "%~dp0tera.sav" (
  echo Aucune sauvegarde. Lancez Lancer_TERA.bat et sauvegardez avec F5.
  pause
  exit /b 2
)
'@} else {''}
    $load = if($resume) {'--load "%~dp0tera.sav"'} else {''}
    $name = if($resume) {'Reprendre_partie.bat'} else {'Lancer_TERA.bat'}
    $launcher.Replace('__RESUME_CHECK__',$check).Replace('__LOAD__',$load) |
        Set-Content -LiteralPath (Join-Path $packagePath $name) -Encoding ascii
}
$manual = Get-Content -LiteralPath (Join-Path $projectRoot 'docs/MANUEL_RAPIDE.md') -Raw
$manual = $manual.Replace('E:\dev\PYTHON\PROJETS\tera_remake\reconstruction\build\sdl3\runtime-only','Le dossier dans lequel tu as extrait cette archive')
$manual = $manual.Replace('Double-clique sur **tera_sdl3.exe**.','Double-clique sur **Lancer_TERA.bat** apres avoir ajoute les donnees originales dans **data**.')
$start = $manual.IndexOf('**Pour reprendre après avoir fermé le jeu :**')
$end = $manual.IndexOf('## 9.', $start)
if($start -lt 0 -or $end -lt 0) {throw 'Beginner guide section not found'}
$manual = $manual.Substring(0,$start) + "**Pour reprendre après avoir fermé le jeu :** double-clique sur **Reprendre_partie.bat**. Ce lanceur charge directement la sauvegarde tera.sav.`r`n`r`n" + $manual.Substring($end)
$manual | Set-Content -LiteralPath (Join-Path $packagePath 'MANUEL_DEBUTANT.md') -Encoding utf8
$files = @(Get-ChildItem -LiteralPath $packagePath -Recurse -File)
$manifest = [ordered]@{version=$Version;platform='windows-x64';source_commit=$commit;
    build_type='Release';sdl_version='3.2.8';original_data_included=$false;required_data=$resources;
    files=@($files | ForEach-Object {[ordered]@{path=$_.FullName.Substring($packagePath.Length+1).Replace('\','/');
        bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $packagePath 'MANIFEST.json') -Encoding utf8
Compress-Archive -LiteralPath $packagePath -DestinationPath $zipPath -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $zipPath).Hash.ToLowerInvariant()
"$hash  $packageName.zip" | Set-Content -LiteralPath (Join-Path $outputPath "$packageName.sha256.txt") -Encoding ascii
Write-Output "Archive: $zipPath"
Write-Output "SHA256: $hash"
