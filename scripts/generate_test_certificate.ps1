$subject  = "LesserJoy Test"
$password = "Test"
$certs    = "certs"

$cert = New-SelfSignedCertificate -Subject "CN=$subject" -Type CodeSigningCert -CertStoreLocation Cert:\CurrentUser\My -KeyUsage DigitalSignature -KeyAlgorithm RSA -KeyLength 2048 -HashAlgorithm SHA256 -NotBefore (Get-Date).AddDays(-1) -NotAfter (Get-Date).AddYears(10)

# Export .pfx (clé privée + publique) — nécessaire pour signer
$password = ConvertTo-SecureString -String "$password" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath "$certs/$subject.pfx" -Password $password

# Export .cer (clé publique) — pour installer sur la VM
Export-Certificate -Cert $cert -FilePath "$certs/$subject.cer"

$cert.Thumbprint
