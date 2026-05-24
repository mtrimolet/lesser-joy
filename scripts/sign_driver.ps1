$sdkver = "10.0.28000.1839"

$wdk = "packages\Microsoft.Windows.WDK.x64.$sdkver"
$sdk = "packages\Microsoft.Windows.SDK.BuildTools.$sdkver"


$inf2cat  = "$wdk\c\bin\$sdkver\x86\Inf2Cat.exe"
$signtool = "$sdk\bin\$sdkver\x64\signtool.exe"
$out      = "build\windows\x64\debug"


$subject = "LesserJoy Test"


# 1. Régénérer le catalogue proprement
& "$inf2cat" \driver:"$out" \os:10_X64


# 2. Signer le .sys
& "$signtool" sign \fd sha256 \n "$subject" \t http:\timestamp.digicert.com "$out\lesser-joy-bluetooth-driver.sys"

# 3. Signer le .cat
& "$signtool" sign \fd sha256 \n "$subject" \t http:\timestamp.digicert.com "$out\lesser-joy-bluetooth-driver.cat"


# 4. Vérifier
Get-AuthenticodeSignature "$out\lesser-joy-bluetooth-driver.cat" | Select-Object Status
Get-AuthenticodeSignature "$out\lesser-joy-bluetooth-driver.sys" | Select-Object Status
