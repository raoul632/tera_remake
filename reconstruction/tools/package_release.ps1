param(
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+(?:-[a-zA-Z0-9.-]+)?$')]
    [string]$Version = '0.2.0',
    [string]$BuildDir = (Join-Path $PSScriptRoot '../build/embedded-win64'),
    [string]$OutputRoot = (Join-Path $PSScriptRoot '../build/releases'),
    [Parameter(Mandatory=$true)][string]$SdlLicense
)
# Local package explicitly requested by the user; never publish or overwrite.
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
    $binary = Join-Path $buildRoot $file
    if(!(Test-Path -LiteralPath $binary)) {throw "Missing build file: $file"}
    $bytes = [IO.File]::ReadAllBytes($binary)
    $pe = [BitConverter]::ToInt32($bytes,60)
    if([BitConverter]::ToUInt16($bytes,$pe+4) -ne 0x8664) {throw "Not Windows x64: $file"}
}
if(!(Test-Path -LiteralPath $SdlLicense)) {throw 'SDL license file absent'}
$commit = (git -C $projectRoot rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0) {throw 'Cannot identify source commit'}
$dirty = [bool](git -C $projectRoot status --porcelain)
$assetManifest = Join-Path $projectRoot 'assets/original/editable/manifest.json'
$generated = Join-Path $projectRoot 'assets/original/generated/embedded.c'
if(!(Test-Path -LiteralPath $assetManifest) -or !(Test-Path -LiteralPath $generated)) {
    throw 'Converted provenance and generated source required for packaging'
}
New-Item -ItemType Directory -Path $packagePath,(Join-Path $packagePath 'notices') | Out-Null
foreach($file in @('tera_sdl3.exe','SDL3.dll')) {
    Copy-Item -LiteralPath (Join-Path $buildRoot $file) -Destination $packagePath
}
Copy-Item -LiteralPath $SdlLicense -Destination (Join-Path $packagePath 'notices/SDL3-LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $projectRoot 'frontend/sdl3/vendor/font8x8/NOTICE.md') -Destination (Join-Path $packagePath 'notices/IBM-CP437-NOTICE.md')
Copy-Item -LiteralPath $assetManifest -Destination (Join-Path $packagePath 'notices/TERA-DATA-PROVENANCE.json')
$readme = @"
TERA - Port original C/SDL3 - v$Version - Windows x64

INSTALLATION
Extrayez entierement le ZIP dans un dossier accessible en ecriture.
Double-cliquez sur tera_sdl3.exe. SDL3.dll doit rester a cote.
Les textes, tables, 82 sprites et six fonds sont integres a l'executable.
Aucune copie de donnees DOS et aucun Python ne sont necessaires.

COMMANDES
Entree passe le titre. Nom puis Entree, H ou F, repartitions de points,
bonifications, O pour garder le personnage. Fleches pour marcher.
F5 sauvegarde, F9 recharge pendant le jeu. F11 : plein ecran.
Voir MANUEL_DEBUTANT.md. La sauvegarde par defaut est tera.sav dans
le repertoire de travail. --save CHEMIN permet de choisir son emplacement.
Pour reprendre des le demarrage : tera_sdl3.exe --load tera.sav

PROVENANCE ET LIMITES
TERA, Loriciels, 1986. Ressources extraites des donnees originales,
converties sans perte puis integrees. Leur origine et leurs droits restent
inchanges. Cette archive locale ne constitue pas une autorisation de
publication ou de redistribution. Empreintes dans notices/.
Sources de base : $commit ; modifications locales : $dirty.
SDL et police : notices/. Aucun mode Enhanced dans cette version.
Les tests du port ne prouvent pas une fidelite DOS globale chiffree.
"@
$readme | Set-Content -LiteralPath (Join-Path $packagePath 'LISEZ_MOI.txt') -Encoding utf8
Copy-Item -LiteralPath (Join-Path $projectRoot 'docs/MANUEL_RAPIDE.md') -Destination (Join-Path $packagePath 'MANUEL_DEBUTANT.md')
$files = @(Get-ChildItem -LiteralPath $packagePath -Recurse -File)
$manifest = [ordered]@{version=$Version;platform='windows-x64';source_commit=$commit;
    source_dirty=$dirty;build_type='Release';resources_embedded=$true;external_data_required=$false;
    generated_source_sha256=(Get-FileHash -LiteralPath $generated).Hash.ToLowerInvariant();
    files=@($files | ForEach-Object {[ordered]@{path=$_.FullName.Substring($packagePath.Length+1).Replace('\','/');
        bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $packagePath 'MANIFEST.json') -Encoding utf8
Compress-Archive -LiteralPath $packagePath -DestinationPath $zipPath -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $zipPath).Hash.ToLowerInvariant()
"$hash  $packageName.zip" | Set-Content -LiteralPath (Join-Path $outputPath "$packageName.sha256.txt") -Encoding ascii
Write-Output "Archive: $zipPath"
Write-Output "SHA256: $hash"
