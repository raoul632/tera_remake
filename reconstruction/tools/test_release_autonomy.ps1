param(
    [Parameter(Mandatory=$true)][string]$Archive,
    [Parameter(Mandatory=$true)][string]$Destination
)
# Test the archive the player receives, not files in a compiler output folder.
$ErrorActionPreference='Stop'
$archivePath=(Resolve-Path -LiteralPath $Archive).Path
if(Test-Path -LiteralPath $Destination) {throw 'Use a new isolation directory'}
New-Item -ItemType Directory -Path $Destination | Out-Null
$isolation=(Resolve-Path -LiteralPath $Destination).Path
$cwd=Join-Path $isolation 'unrelated-cwd'
New-Item -ItemType Directory -Path $cwd | Out-Null
Expand-Archive -LiteralPath $archivePath -DestinationPath (Join-Path $isolation 'extracted')
$package=@(Get-ChildItem -LiteralPath (Join-Path $isolation 'extracted') -Directory)
if($package.Count -ne 1) {throw 'Expected one package directory'}
$packagePath=$package[0].FullName
$inventory=@(Get-ChildItem -LiteralPath $packagePath -File -Recurse)
foreach($file in $inventory) {
    $relative=$file.FullName.Substring($packagePath.Length+1).Replace('\','/')
    if($relative -notmatch '^(tera_sdl3\.exe|SDL3\.dll|LISEZ_MOI\.txt|MANUEL_DEBUTANT\.md|MANIFEST\.json|notices/[^/]+\.(txt|md|json))$') {
        throw "Forbidden player package file: $relative"
    }
}
$inventory | Select-Object Name,Length,FullName | ConvertTo-Json |
    Set-Content -LiteralPath (Join-Path $isolation 'inventory-before.json') -Encoding utf8
$evidence=[Collections.Generic.List[object]]::new()
foreach($driver in @('dummy','windows')) {
    $save=Join-Path $cwd "$driver.sav"
    $capture=Join-Path $cwd "$driver.bmp"
    $start=[Diagnostics.ProcessStartInfo]::new()
    $start.FileName=Join-Path $packagePath 'tera_sdl3.exe'
    $start.WorkingDirectory=$cwd
    $start.UseShellExecute=$false
    $start.CreateNoWindow=$true
    $start.RedirectStandardOutput=$true
    $start.RedirectStandardError=$true
    $start.Environment.Clear()
    $start.Environment['SystemRoot']=$env:SystemRoot
    $start.Environment['WINDIR']=$env:SystemRoot
    $start.Environment['PATH']=Join-Path $env:SystemRoot 'System32'
    $start.Environment['TEMP']=$cwd
    $start.Environment['TMP']=$cwd
    $start.Environment['SDL_VIDEODRIVER']=$driver
    $start.Environment['SDL_RENDER_DRIVER']='software'
    $start.Environment['SDL_AUDIODRIVER']='dummy'
    foreach($arg in @('--smoke','--save',$save,'--capture',$capture)) {$start.ArgumentList.Add($arg)}
    $process=[Diagnostics.Process]::Start($start)
    $stdout=$process.StandardOutput.ReadToEndAsync()
    $stderr=$process.StandardError.ReadToEndAsync()
    if(!$process.WaitForExit(30000)) {$process.Kill();throw "Timeout: $driver"}
    $result=[ordered]@{executable=$start.FileName;cwd=$cwd;arguments=@($start.ArgumentList);
        environment=@{};exit_code=$process.ExitCode;stdout=$stdout.Result;stderr=$stderr.Result}
    foreach($key in $start.Environment.Keys) {$result.environment[$key]=$start.Environment[$key]}
    $evidence.Add($result)
    $evidence | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $isolation 'runs.json') -Encoding utf8
    if($process.ExitCode -ne 0 -or $stdout.Result -notmatch 'SDL smoke OK' -or
       !(Test-Path -LiteralPath $save) -or !(Test-Path -LiteralPath $capture)) {throw "Autonomy failed: $driver"}
    Write-Output "$driver : exit 0; title, creation, movement, text, rendering, identical saved/reloaded state"
}
# Reopen the save in a separate normal player process, through CLI --load.
# Closing its own SDL window sends the ordinary quit event (no forced kill).
$start.ArgumentList.Clear()
foreach($arg in @('--load',(Join-Path $cwd 'windows.sav'),'--save',(Join-Path $cwd 'windows.sav'))) {
    $start.ArgumentList.Add($arg)
}
$process=[Diagnostics.Process]::Start($start)
$stdout=$process.StandardOutput.ReadToEndAsync()
$stderr=$process.StandardError.ReadToEndAsync()
Start-Sleep -Milliseconds 1200
$process.Refresh()
if($process.HasExited) {throw "Separate resume exited early: $($stderr.Result)"}
if(!$process.CloseMainWindow()) {$process.Kill();throw 'Cannot close normal resumed SDL window'}
if(!$process.WaitForExit(10000)) {$process.Kill();throw 'Normal resume quit timeout'}
$evidence.Add([ordered]@{executable=$start.FileName;cwd=$cwd;arguments=@($start.ArgumentList);
    environment=$result.environment;exit_code=$process.ExitCode;stdout=$stdout.Result;stderr=$stderr.Result;
    normal_window_started=$true;normal_quit=$true})
$evidence | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $isolation 'runs.json') -Encoding utf8
if($process.ExitCode -ne 0 -or $stderr.Result) {throw 'Separate normal --load failed'}
Write-Output 'separate --load : normal SDL window started, ordinary close, exit 0'
if(@(Get-ChildItem -LiteralPath $packagePath -Recurse -File).Count -ne $inventory.Count) {
    throw 'Runtime created unexpected files inside the player package'
}
Write-Output "PASS: extracted archive, original files absent, unrelated cwd, system-only PATH, cleared environment. Evidence: $isolation"
