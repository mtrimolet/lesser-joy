$subject = "LesserJoy Test"
$cert    = "certs/$subject.cer"

certutil -addstore "Root" "$cert"
certutil -addstore "TrustedPublisher" "$cert"

certutil -store TrustedPublisher | findstr /i "$subject"
