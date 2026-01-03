$path = "chumini/IngameCanvas.cpp"
$encoding = [System.Text.Encoding]::Default
$content = [System.IO.File]::ReadAllText($path, $encoding)
$find = 'L"851繧ｴ繝√き繧ｯ繝・ヨ"'
$replace = 'L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8"'
$content = $content.Replace($find, $replace)
[System.IO.File]::WriteAllText($path, $content, $encoding)
Write-Host "Replaced content."
