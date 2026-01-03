$path = "chumini/IngameCanvas.cpp"
$encoding = [System.Text.Encoding]::Default
$content = [System.IO.File]::ReadAllText($path, $encoding)
$findPattern = 'L"851[^"]*"'
$replace = 'L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8"'
$content = [regex]::Replace($content, $findPattern, $replace)
[System.IO.File]::WriteAllText($path, $content, $encoding)
Write-Host "Replaced content using Regex."
