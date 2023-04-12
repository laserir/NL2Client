# NL2Client
An easy-to-use client for the NoLimits 2 Telemetry Server for Arduino, ESP32...

## Installation
Search for "NL2Client" in the Arduino IDE Library Manager.

## Usage
### Create an instance
```
NL2Client nl2("192.168.178.51", 15151, 500, false);     // Arguments: IP address, port, timeout, debug
```

### Get values (pretty self-explanatory)
| Function  | Arguments | Returns |
| --- | --- | --- |
| .Idle()  | none | true, if connected to server<br/>false, otherwise |
| .Idle()  | none | true, if connected to server<br/>false, otherwise |

nl2;
nl2.getVersion();
nl2.getCurrentCoaster();
nl2.getNearestStation();
nl2.getCoasterCount();
