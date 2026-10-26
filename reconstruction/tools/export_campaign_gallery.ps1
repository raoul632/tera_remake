param([Parameter(Mandatory=$true)][string]$Directory)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$galleryPath=(Resolve-Path -LiteralPath $Directory).Path
$rows=@(Import-Csv -LiteralPath (Join-Path $galleryPath 'parcours.csv'))
$labels=@{
    1='Écran titre';2='Création du héros';3='Départ sur Amarande';4='Aux portes de la ville'
    5='Exploration des paysages';7='Soins, achats et équipement';8='Entrée dans les ruines'
    9='Les couloirs des ruines';14='Dans les étages de la cité';19='Défaite : une succession reprend la quête'
    22='Arioch vaincu : premier objectif';23='Entrée dans le domaine de Seth'
    24='Seth invoqué : deuxième objectif';25='Préparation du départ spatial'
    26='Affrontement pendant le vol';27="Arrivée sur la surface d'Alfol"
    28="Exploration d'Alfol";29="Dans les salles d'Alfol";30='Alfol : troisième objectif'
    31='Entrée dans Méduz';32='Exploration de Méduz';33='Traversée du tunnel'
    34='Sanctuaire de Méduz : quatre objectifs';35='Retour et victoire — score 8'
}
$sceneNames=@{0='Ruines';1='Cité : étage 1';2='Cité : étage 2';3='Cité : étage 3';4='Cité : étage 4';5='Cité : étage 5';6='Cité : étage 6';7='Cité : étage 7';8='Domaine de Seth';9='Alfol';10='Méduz';11='Amarande'}
$figures=[Collections.Generic.List[string]]::new()
foreach($row in $rows) {
    $id=[int]$row.image;$stem='{0:D3}' -f $id
    $bitmap=[Drawing.Bitmap]::new((Join-Path $galleryPath "$stem.bmp"))
    try {$bitmap.Save((Join-Path $galleryPath "$stem.png"),[Drawing.Imaging.ImageFormat]::Png)} finally {$bitmap.Dispose()}
    $label=if($labels.ContainsKey($id)) {$labels[$id]} else {$row.label}
    if($id -ge 13 -and $id -le 21 -and $id -ne 14 -and $id -ne 19) {$label=$sceneNames[[int]$row.world]}
    $row | Add-Member -NotePropertyName title -NotePropertyValue $label
    $safe=[Net.WebUtility]::HtmlEncode($label)
    $zone=[Net.WebUtility]::HtmlEncode($sceneNames[[int]$row.world])
    $metadata=if($id -le 2) {'Ouverture de la partie'} else {"Entrée $($row.input) · $zone · position $($row.position) · objectifs $($row.objectives)/4"}
    $figures.Add("<figure id='image-$stem'><a href='$stem.png'><img loading='lazy' src='$stem.png' alt='$safe'></a><figcaption><b>$stem · $safe</b><span>$metadata</span></figcaption></figure>")
}
$html=@"
<!doctype html><html lang="fr"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>TERA — une partie en 35 captures</title>
<style>body{margin:0;background:#111521;color:#ecedf5;font:16px/1.6 system-ui,sans-serif}header,main,footer{max-width:1400px;margin:auto;padding:28px}h1{font-size:clamp(28px,4vw,48px);margin:0}p{max-width:900px;color:#bfc6da}.tags{color:#f1d883}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(340px,1fr));gap:24px}figure{margin:0;background:#1d2333;border:1px solid #353d51;border-radius:8px;overflow:hidden}img{display:block;width:100%;height:auto;image-rendering:pixelated;background:#000}figcaption{padding:14px}figcaption span{display:block;color:#acb8d0;font-size:13px}a{color:#9edbea}footer{color:#bfc6da}</style>
<header><div class="tags">SDL / C · graine 1986 · captures réelles du rendu</div><h1>Une partie complète de TERA</h1>
<p>Je crée le héros, recrute trois compagnons et équipe le groupe. L'exploration de la cité coûte plusieurs vies : cinq successions sont nécessaires. Le parcours finit par vaincre Arioch, accomplir l'objectif de Seth, voyager vers Alfol, puis traverser Méduz et son sanctuaire. Retour sur Amarande et victoire : score 8.</p>
<p>Simulation automatique dans le frontend SDL, attentes accélérées. 3 886 entrées, 36 combats gagnés, 83 fuites, 5 successions et 23 sauvegardes/rechargements. Les décisions du pilote utilisent les cartes et les informations internes du moteur ; ce récit ne représente pas une découverte à l'aveugle. Les images proviennent du framebuffer SDL avant affichage. Cliquer pour ouvrir chaque capture à sa taille originale.</p>
<a href="apercu.png">Voir la planche de 12 étapes</a></header><main>
$($figures -join "`n")
</main><footer>35 captures chronologiques · douze zones visitées · quatre objectifs accomplis · aucune modification des règles ni de la progression.</footer></html>
"@
[IO.File]::WriteAllText((Join-Path $galleryPath 'index.html'),$html,[Text.UTF8Encoding]::new($false))
# Planche distincte : captures intactes accessibles dans l'album, miniatures
# reduites sans lissage pour conserver la lisibilite du dessin CGA.
$selection=3,7,10,19,21,22,24,26,28,30,33,35
$sheet=[Drawing.Bitmap]::new(1056,1268)
$g=[Drawing.Graphics]::FromImage($sheet)
$g.Clear([Drawing.Color]::FromArgb(17,21,33))
$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$g.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::Half
$font=[Drawing.Font]::new('Segoe UI',10)
$titleFont=[Drawing.Font]::new('Segoe UI',18,[Drawing.FontStyle]::Bold)
$brush=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(235,239,249))
try {
    $g.DrawString('TERA — une partie complète dans SDL',$titleFont,$brush,18,12)
    $g.DrawString('Graine 1986 · 4 objectifs · 36 victoires · 5 successions · score final 8',$font,$brush,18,49)
    for($i=0;$i -lt $selection.Count;$i++) {
        $id=$selection[$i];$row=$rows | Where-Object {[int]$_.image -eq $id}
        $x=16+($i%3)*348;$y=88+[math]::Floor($i/3)*292
        $bmp=[Drawing.Bitmap]::new((Join-Path $galleryPath ('{0:D3}.png' -f $id)))
        try {$g.DrawImage($bmp,[Drawing.Rectangle]::new($x,$y,328,246))} finally {$bmp.Dispose()}
        $g.DrawString("$id. $($row.title)",$font,$brush,[Drawing.RectangleF]::new($x,$y+249,328,40))
    }
    $sheet.Save((Join-Path $galleryPath 'apercu.png'),[Drawing.Imaging.ImageFormat]::Png)
} finally {$g.Dispose();$sheet.Dispose();$font.Dispose();$titleFont.Dispose();$brush.Dispose()}
Write-Output "Album : $galleryPath/index.html ($($rows.Count) captures)"
