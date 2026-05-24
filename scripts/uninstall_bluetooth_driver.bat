:: sc stop LesserJoyBluetoothDriver
:: sc delete LesserJoyBluetoothDriver
:: del %SystemRoot%\System32\Drivers\lesser-joy-bluetooth-driver.sys

pnputil /delete-driver oem6.inf
