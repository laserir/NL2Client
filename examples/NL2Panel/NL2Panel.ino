#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <NL2Client.h>

WebServer Server;
WiFiClient client;
AutoConnect Portal(Server);
AutoConnectConfig Config;

NL2Client nl2("192.168.178.51", 15151, 500, false);

//Define buttons
const int buttonPin[] = {13, 12, 14, 27, 26, 25, 33, 32};
/*
int buttonState[] = {1, 1, 1, 1, 1, 1, 1, 1};
int prevButtonState[] = {1, 1, 1, 1, 1, 1, 1, 1};
int actionSent[] = {0, 0, 0, 0, 0, 0, 0, 0};
long startedPressing[] = {0, 0, 0, 0, 0, 0, 0, 0};
*/

//Define LEDs
const int LEDPin[] = {19, 18, 17, 16};
int LEDState[] = {0, 0, 0, 0};

int blinkState = 0;
int blinkInterval = 500;
long blinkMillis = 0;

void setup() {
  delay(1000);
  Serial.begin(115200);

  //Initialize button pins
  for (int i = 0; i < (sizeof(buttonPin) / sizeof(buttonPin[0])); i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);
    //digitalWrite(buttonPin[i], HIGH); // In some versions use INPUT_PULLUP to use the built-in pull up resistor
  }

  //Initialize LED pins (if LEDs set on ESP32)
  for (int i = 0; i < (sizeof(LEDPin) / sizeof(LEDPin[0])); i++) {
    pinMode(LEDPin[i], OUTPUT);
    digitalWrite(LEDPin[i], LOW);
  }

  Config.apid = "ControlPanel-"+String((uint32_t)(ESP.getEfuseMac() >> 32), HEX); //+WiFi.macAddress();
  Config.psk = "nl2";
  Config.hostName = "ControlPanel-"+String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);//+WiFi.macAddress();
  Config.ota = AC_OTA_BUILTIN;
  Config.title ="#ControlPanel";
  Config.boundaryOffset = 64;                   //Veraltet?

  /*ACText(header, "Set Server");
  ACText(caption1, "Enter the server IP. The server must be in the same local network.");
  ACSubmit(save, "SAVE", "/settings_save");
  AutoConnectAux  aux1("/settings", "Set NL2 Server", true, { header, caption1, save });
  
  ACText(caption2, "Server IP saved.");
  //ACSubmit(start, "START", "/mqtt_start"); 
  AutoConnectAux  aux2("/settings_save", "Settings saved", false, { caption2 });
  
  //AutoConnectAux  aux3("/mqtt_start", "MQTT Start");

  Portal.join({ aux1, aux2 });*/
  
  Portal.config(Config);

  Server.on("/",HTTP_GET,returnVirtualPanel);
  Server.on("/json",returnJson);
  
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }
  
}


void loop() {

  Portal.handleClient();

  setLEDs();

  if(digitalRead(buttonPin[0]) == HIGH){ //Keyswitch OFF
    LEDState[0] = 0;
    LEDState[1] = 1;
    LEDState[2] = 0;
    LEDState[3] = 0;
    delay(50);
    return;
    }

  if(nl2.updateTelemetry()){    //Connection to server
      nl2.getCurrentCoasterAndNearestStation();
      nl2.getStationState(nl2.currentCoaster,nl2.nearestStation);

      //Set to manual mode if in automatic mode
      if(nl2.stationManual() == false){
        nl2.setManualMode(nl2.currentCoaster,nl2.nearestStation,true);
        }

      //Lower floor when gates and harness are closed
      if(digitalRead(buttonPin[4]) == LOW and digitalRead(buttonPin[5]) == LOW and nl2.stationPlatformCanLower()){
        nl2.setPlatform(nl2.currentCoaster,nl2.nearestStation,true);
        }

      //Raise floor when gate or harness switch is set to open
      if((digitalRead(buttonPin[4]) == HIGH or digitalRead(buttonPin[5]) == HIGH) and nl2.stationPlatformCanRaise()){
        nl2.setPlatform(nl2.currentCoaster,nl2.nearestStation,false);
        }

      //Lock flyer car when gates and harness are closed
      if(digitalRead(buttonPin[4]) == LOW and digitalRead(buttonPin[5]) == LOW and nl2.stationFlyerCarCanLock()){
        nl2.setFlyerCar(nl2.currentCoaster,nl2.nearestStation,true);
        }

      //Unlock flyer car when gate or harness switch is set to open
      if((digitalRead(buttonPin[4]) == HIGH or digitalRead(buttonPin[5]) == HIGH) and nl2.stationFlyerCarCanUnlock()){
        nl2.setFlyerCar(nl2.currentCoaster,nl2.nearestStation,false);
        }

      //Synchronize gates to switch
      if(digitalRead(buttonPin[4]) == LOW and nl2.stationGatesCanClose()){
        nl2.setGates(nl2.currentCoaster,nl2.nearestStation,false);
        }
      else if(digitalRead(buttonPin[4]) == HIGH and nl2.stationGatesCanOpen()){
        nl2.setGates(nl2.currentCoaster,nl2.nearestStation,true);
        }

      //Synchronize harness to switch
      if(digitalRead(buttonPin[5]) == LOW and nl2.stationHarnessCanClose()){
        nl2.setHarness(nl2.currentCoaster,nl2.nearestStation,false);
        }
      else if(digitalRead(buttonPin[5]) == HIGH and nl2.stationHarnessCanOpen()){
        nl2.setHarness(nl2.currentCoaster,nl2.nearestStation,true);
        }

      //Synchronize E-Stop to switch
      if(digitalRead(buttonPin[2]) == LOW and nl2.stationEstop() == false){
        nl2.setEstop(nl2.currentCoaster,true);
        }
      else if(digitalRead(buttonPin[2]) == HIGH and nl2.stationEstop() == true){
        nl2.setEstop(nl2.currentCoaster,false);
        }

      //Dispatch
      if(digitalRead(buttonPin[3]) == LOW and digitalRead(buttonPin[6]) == LOW and nl2.stationCanDispatch()){
        nl2.dispatch(nl2.currentCoaster,nl2.nearestStation);
        }

      //Reset
      if(digitalRead(buttonPin[1]) == LOW){
        ESP.restart();
        }

      //Set Control LED
      if(nl2.telemetryInPlay() == true and nl2.telemetryPaused() == false){
        LEDState[0] = 1;
        }
      else{
        LEDState[0] = 2;
        }

      //Set Reset LED
      if(nl2.stationEstop()){
        LEDState[1] = 2;
        }
      else if(nl2.telemetryInPlay() == false or nl2.telemetryPaused()){
        LEDState[1] = 1;
        }
      else{
        LEDState[1] = 0;
        }

      //Set Dispatch LEDs
      if(nl2.stationCanDispatch() == true){
        LEDState[2] = 3;
        LEDState[3] = 3;
        }
      else{
        LEDState[2] = 0;
        LEDState[3] = 0;        
        }
    }
  else{   //No connection to server
    LEDState[0] = 0;
    LEDState[1] = 1;
    LEDState[2] = 2;
    LEDState[3] = 3;
    //setLEDs;
    delay(50);
    return;
    }

  //Check for button presses
  /*
  for (int i = 0; i < (sizeof(buttonPin) / sizeof(buttonPin[0])); i++) {
    buttonState[i] = digitalRead(buttonPin[i]);
    long pressingDuration = millis() - startedPressing[i];
    // HIGH = state 1 <- button not pressed
    // LOW  = state 0 <- button pressed

    if (buttonState[i] == 0 and prevButtonState[i] == 1) { //not pressed -> pressed  
      startedPressing[i] = millis();
      actionSent[i] = 0;
    }
    else if (buttonState[i] == 1 and prevButtonState[i] == 0) { //pressed -> not pressed  
      if (pressingDuration > 50 and pressingDuration <= 1000){ //short press
        outputAction(i, 1);
      }
      if (pressingDuration > 50){outputAction(i, 0);} //release
    }
    else if (buttonState[i] == 0 and prevButtonState[i] == 0) { //still pressed
      if(pressingDuration > 1000 and actionSent[i] != 1){ //long press
        actionSent[i] = 1;
        outputAction(i, 2);
      }
    }
   prevButtonState[i] = buttonState[i]; 
  }
*/

  delay(50);
}

/*
void outputAction(int i, int typeOfPress) {
  // typeOfPress 0: release; 1: short, 2: long
  // LEDState 0: off; 1: on; 2: blink (Cycle 1); 3: blink (Cycle 2); 
  
  if(i == 1){ //Reset
    if(typeOfPress == 2){
        ESP.restart();
        }
      }
  else if(i == 3){ //Dispatch left
    if(typeOfPress == 2){
      if(digitalRead(buttonPin[6]) == LOW and nl2.stationCanDispatch()){
        nl2.dispatch(nl2.currentCoaster,nl2.nearestStation);
        }
      }
    }
  else if(i == 6){ //Dispatch right
    if(typeOfPress == 2){
      if(digitalRead(buttonPin[3]) == LOW and nl2.stationCanDispatch()){
        nl2.dispatch(nl2.currentCoaster,nl2.nearestStation);
        }
      }
    }
}
*/

void setLEDs(){
  //Switch blink cycle
  if(millis()-blinkMillis >= blinkInterval){
    if(blinkState==0){blinkState=1;}
    else {blinkState=0;}
    blinkMillis=millis();
    }

  //Set LEDs (Use pins of ESP32)
  for (int i = 0; i < (sizeof(LEDPin) / sizeof(LEDPin[0])); i++) {
    if(LEDState[i] == 0){
        digitalWrite(LEDPin[i], LOW);
    }
    else if(LEDState[i] == 1){
        digitalWrite(LEDPin[i], HIGH);
    }
    else if(LEDState[i] == 2){
        if(blinkState == 0){digitalWrite(LEDPin[i], LOW);}
        else{digitalWrite(LEDPin[i], HIGH);}
    }    
    else if(LEDState[i] == 3){
        if(blinkState == 0){digitalWrite(LEDPin[i], HIGH);}
        else{digitalWrite(LEDPin[i], LOW);}
    }
    //Serial.print(i);Serial.print(">");Serial.println(digitalRead(LEDPin[i])); //DEBUG <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  }

  }

void returnJson(){
  char buffer[2048];
  StaticJsonDocument<2048> doc;
  JsonObject root = doc.to<JsonObject>();
  root["FWversion"] = "0.1";
  root["Uptime"] = millis()/1000;
  root["WiFiStatus"] = WiFi.status();
  root["ConnectedToServer"] = nl2.Idle();
  root["NL2version"] = nl2.getVersion();
  root["CurrentCoaster"] = nl2.getCurrentCoaster();
  root["NearestStation"] = nl2.getNearestStation();
  root["CoasterCount"] = nl2.getCoasterCount();

  nl2.updateTelemetry();
  root["InPlay"] = nl2.telemetryInPlay();
  root["Breaking"] = nl2.telemetryBreaking();
  root["Paused"] = nl2.telemetryPaused();
  root["CurrentFrame"] = nl2.telemetryCurrentFrame();
  root["ViewMode"] = nl2.telemetryViewMode();
  root["CurrentCoaster"] = nl2.telemetryCurrentCoaster();
  root["CoasterStyle"] = nl2.telemetryCoasterStyle();
  root["CurrentTrain"] = nl2.telemetryCurrentTrain();
  root["CurrentCar"] = nl2.telemetryCurrentCar();
  root["CurrentSeat"] = nl2.telemetryCurrentSeat();
  root["Speed"] = nl2.telemetrySpeed();
  root["PosX"] = nl2.telemetryPosX();
  root["PosY"] = nl2.telemetryPosY();
  root["PosZ"] = nl2.telemetryPosZ();
  root["RotationX"] = nl2.telemetryRotationX();
  root["RotationY"] = nl2.telemetryRotationY();
  root["RotationZ"] = nl2.telemetryRotationZ();
  root["RotationW"] = nl2.telemetryRotationW();
  root["GfordeX"] = nl2.telemetryGforceX();
  root["GforceY"] = nl2.telemetryGforceY();
  root["GforceZ"] = nl2.telemetryGforceZ();

  JsonObject coasters = root.createNestedObject("coasters");
  for(int i = 0; i < root["CoasterCount"]; i++){
      JsonObject coasterItem = coasters.createNestedObject(nl2.getCoasterName(i));
      for(int j = 0; j < 5; j++){ //TO-DO: How many stations?
        uint32_t stationState = nl2.getStationState(i,j);
        if(stationState != 0){
          JsonObject stationItem = coasterItem.createNestedObject("Station "+(String)j);
          stationItem["Estop"] = bitRead(stationState,0);
          stationItem["Manual"] = bitRead(stationState,1);
          stationItem["CanDispatch"] = bitRead(stationState,2);
          stationItem["GatesCanClose"] = bitRead(stationState,3);
          stationItem["GatesCanOpen"] = bitRead(stationState,4);
          stationItem["HarnessCanClose"] = bitRead(stationState,5);
          stationItem["HarnessCanOpen"] = bitRead(stationState,6);
          stationItem["PlatformCanRaise"] = bitRead(stationState,7);
          stationItem["PlatformCanLower"] = bitRead(stationState,8);
          stationItem["stationFlyerCarCanLock"] = bitRead(stationState,9);
          stationItem["stationFlyerCarCanUnlock"] = bitRead(stationState,10);
          stationItem["stationTrainInsideStation"] = bitRead(stationState,11);
          stationItem["stationTrainInsideStationIsCurrentRideView"] = bitRead(stationState,12);
          }
        }
    }
  serializeJsonPretty(root, buffer);
  Server.send(200, "application/json", buffer);
  }

void returnVirtualPanel(){
  //TO-DO: No reload --> ajax/jquery to /action. Auto refresh controls (E-Stop on...?)
  if(Server.arg("action") == "test"){
        
    }

  Server.send(200, "text/plain", Server.arg("action"));
  }
