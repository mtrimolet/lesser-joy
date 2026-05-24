:: copy Z:\build\windows\x64\debug\lesser-joy-bluetooth-driver.sys %SystemRoot%\System32\Drivers\lesser-joy-bluetooth-driver.sys
:: sc create LesserJoyBluetoothDriver type= kernel        binPath= %SystemRoot%\System32\Drivers\lesser-joy-bluetooth-driver.sys DisplayName= "LesserJoy Bluetooth driver"
:: sc start  LesserJoyBluetoothDriver

pnputil /add-driver Z:\build\windows\x64\debug\lesser-joy-bluetooth-driver.inf /install
