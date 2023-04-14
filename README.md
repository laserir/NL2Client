# NL2Client
An easy-to-use client for the NoLimits 2 Telemetry Server for Arduino, ESP32...

## Installation
Search for "NL2Client" in the Arduino IDE Library Manager.

## Usage
### Example
```
#include <NL2Client.h>

// Create an instance
NL2Client nl2("192.168.178.51", 15151, 500, false);     // Arguments: IP address, port, timeout, debug

if(nl2.updateTelemetry()){
  nl2.getCurrentCoasterAndNearestStation();
  nl2.getStationState(nl2.currentCoaster,nl2.nearestStation);

  //Get values, e.g. ...
  Serial.print("Paused?");Serial.println(nl2.telemetryPaused());
  Serial.print("E-Stop active?");Serial.println(nl2.stationEstop());

  //...or set values, e.g. ...
  if(nl2.stationGatesCanClose()){
    nl2.setGates(nl2.currentCoaster,nl2.nearestStation,false);
  }   
}
```

### Functions
```
bool Idle()                   // true if connected to server
String getVersion()
bool updateTelemetry()        // true if telemetry data successfully retrieved from server
uint32_t getCoasterCount()
String getCoasterName(uint32_t coasterID)
bool getCurrentCoasterAndNearestStation()   // retrieve and set .currentCoaster and .nearestStation
uint32_t getCurrentCoaster()                // call .getCurrentCoasterAndNearestStation() first
uint32_t getNearestStation()                // call .getCurrentCoasterAndNearestStation() first
bool setEstop(uint32_t coasterID, bool state)
uint32_t getStationState(uint32_t coasterID, uint32_t stationID)
bool setManualMode(uint32_t coasterID, uint32_t stationID, bool state)
bool dispatch(uint32_t coasterID, uint32_t stationID)
bool setGates(uint32_t coasterID, uint32_t stationID, bool state)
bool setHarness(uint32_t coasterID, uint32_t stationID, bool state)
bool setPlatform(uint32_t coasterID, uint32_t stationID, bool state)
bool setFlyerCar(uint32_t coasterID, uint32_t stationID, bool state)
bool attractionLoadPark(String file, bool state)    // requires NL2 attraction license
bool attractionClosePark()                          // requires NL2 attraction license
bool attractionResetPark(bool state)                // requires NL2 attraction license
bool attractionSetMode(bool state)                  // requires NL2 attraction license
bool quitServer()
bool setPause(bool state)
bool selectSeat(uint32_t coasterID, uint32_t trainID, uint32_t carID, uint32_t seatID)
bool recenterVR()
bool setCustomView(float posX, float posY, float posZ, float azimuth, float elevation, bool walk)
bool telemetryInPlay()
bool telemetryBreaking()
bool telemetryPaused()
uint32_t telemetryCurrentFrame()
uint32_t telemetryViewMode()
uint32_t telemetryCurrentCoaster()
uint32_t telemetryCoasterStyle()
uint32_t telemetryCurrentTrain()
uint32_t telemetryCurrentCar()
uint32_t telemetryCurrentSeat()
float telemetrySpeed()
float telemetryPosX()
float telemetryPosY()
float telemetryPosZ()
float telemetryRotationX()
float telemetryRotationY()
float telemetryRotationZ()
float telemetryRotationW()
float telemetryGforceX()
float telemetryGforceY()
float telemetryGforceZ()
bool stationEstop()
bool stationManual()
bool stationCanDispatch()
bool stationGatesCanClose()
bool stationGatesCanOpen()
bool stationHarnessCanClose()
bool stationHarnessCanOpen()
bool stationPlatformCanRaise()
bool stationPlatformCanLower()
bool stationFlyerCarCanLock()
bool stationFlyerCarCanUnlock()
bool stationTrainInsideStation()
bool stationTrainInsideStationIsCurrentRideView()
```
