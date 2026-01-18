$p = "c:\DirectX11\NEWchumini\NEWchumini\chumini\NoteManager.cpp"
$l = Get-Content $p
Write-Host "Total Lines: $($l.Count)"

# Define Indices (0-based)
# Line 806 (1-based) is index 805.
$idxPreEnd = 805
$idxInstStart = 811
$idxInstEnd = 1105
$idxOrphanStart = 1107
$idxOrphanEnd = 1229
$idxEndBrace = 1230

# Verify boundaries
Write-Host "Pre End: $($l[$idxPreEnd])"
Write-Host "Inst Start: $($l[$idxInstStart])"
Write-Host "Inst End: $($l[$idxInstEnd])"
Write-Host "Orphan Start: $($l[$idxOrphanStart])"
Write-Host "Orphan End: $($l[$idxOrphanEnd])"
Write-Host "End Brace: $($l[$idxEndBrace])"

# Construct new content
# Order: Pre + Orphan + Instancing + EndBrace (+ Empty line if any)
$new = $l[0..$idxPreEnd] + $l[$idxOrphanStart..$idxOrphanEnd] + $l[$idxInstStart..$idxInstEnd] + $l[$idxEndBrace..($l.Count-1)]

# Write back
$new | Set-Content $p -Encoding UTF8
Write-Host "File updated."
