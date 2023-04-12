#define _NL2Client_h
#include "Arduino.h"


class NL2Client {

  union ArrayToInteger {
    uint32_t integer;
    byte array[4];
  };
        
  union ArrayToShort {
    short shortnum;
    byte array[2];
  };

  union ArrayToFloat {
    float floatnum;
    byte array[4];
  };
  
  private:
    char* host;
    uint16_t port;
    uint16_t timeout;
    boolean debug;
    byte writeArray[50];
    byte readArray[100];
    short readDatasize = 0;
    short readMessageid = 0;
    ArrayToInteger intconv;
    ArrayToShort shortconv;   
    ArrayToFloat floatconv;   
    WiFiClient client;   
  public:
    NL2Client(char* host, uint16_t port, uint16_t timeout = 500, boolean debug = false) {
      this->host = host;
      this->port = port;
      this->timeout = timeout;
      this->debug = debug;
    }
    uint32_t telemetryInt[8];
    float telemetryFloat[11];
    uint32_t stationState;
    uint32_t currentCoaster;
    uint32_t nearestStation;

    boolean conn() {
      if(!client.connected()){
          if (!client.connect(host, port, timeout)) {
            if(debug){Serial.print("Connection to ");Serial.print(host);Serial.print(':');Serial.print(port);Serial.println(" failed");}
            return false;
          }
          else {
            if(debug){Serial.print("Connected to ");Serial.print(host);Serial.print(':');Serial.println(port);}  
            return true;
          }
        }
        else{
          return true;  
        }
    }

    boolean sendMessage(unsigned int message_id, uint32_t request_id = millis(), unsigned short datasize = 0, byte data[] = {}) {
          if(!conn()){return false;}

          while(client.available()){client.read();} //Clear receive buffer
          
          unsigned char message_start = 'N';
          unsigned char message_end = 'L';

          writeArray[0] = message_start;
  
          shortconv.shortnum = message_id;
          writeArray[1] = shortconv.array[1];
          writeArray[2] = shortconv.array[0];
          
          intconv.integer = request_id;
          writeArray[3] = intconv.array[3];
          writeArray[4] = intconv.array[2];
          writeArray[5] = intconv.array[1];
          writeArray[6] = intconv.array[0];
        
          shortconv.shortnum = datasize;
          writeArray[7] = shortconv.array[1];
          writeArray[8] = shortconv.array[0];

          if(datasize > 0){
            for(int i = 0; i < datasize; i++){
              writeArray[9+i] = data[i];
            }  
          }
          
          writeArray[9+datasize] = message_end;

          /*if(debug){
              for(int i = 0; i < 10+datasize; i++){
                Serial.print("writeArray[");Serial.print(i);Serial.print("] = ");
                Serial.println(writeArray[i]);
                }  
            }*/

          client.write(writeArray,10+datasize);
        
          long timeoutMillis = millis();
          while(client.available() == 0){ //Wait for response
            if(millis() - timeoutMillis > timeout){
              if(debug){Serial.println("Connection timeout");}
              client.stop();
              return false;
            }
          }

          if((char)client.read() == 'N'){
            if(debug){Serial.println("Message start >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");}
            }
          else{
            return false;
          }

          shortconv.array[1] = client.read();
          shortconv.array[0] = client.read();
          if(debug){Serial.print("Message ID: ");Serial.println(shortconv.shortnum,DEC);}
          readMessageid = shortconv.shortnum;

          intconv.array[3] = client.read();
          intconv.array[2] = client.read();
          intconv.array[1] = client.read();
          intconv.array[0] = client.read();
          if(debug){
            Serial.print("Request ID: ");Serial.print(intconv.integer,DEC);
            if(intconv.integer == request_id){Serial.println(" (matching request)");}
            else{Serial.print(" (NOT matching request )");Serial.println(request_id,DEC);}
          }
    
          shortconv.array[1] = client.read();
          shortconv.array[0] = client.read();
          if(debug){Serial.print("Data size: ");Serial.println(shortconv.shortnum,DEC);}
    
          for(int i = 0; i < shortconv.shortnum; i++){
            readArray[i] = client.read();
            if(debug){Serial.print((char)readArray[i]);}
            readDatasize = i;
            }
            if(debug){Serial.println();}
    
          if((char)client.read() == 'L'){
            if(debug){Serial.println("Message complete <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");}
            return true;
            }
          return false;

    delay(50);
    
    }

    boolean Idle() {
        if(sendMessage(0,millis(),0) and readMessageid == 1){
          return true;
        }
          return false;
    } 

    String getVersion() {
      if(sendMessage(3,millis(),0) and readMessageid == 4){
        return ((String)readArray[0]+"."+(String)readArray[1]+"."+(String)readArray[2]+"."+(String)readArray[3]);
      }
    }

    boolean updateTelemetry() {
      if(sendMessage(5,millis(),0) and readMessageid == 6){
        for(int offset = 0; offset < 32; offset = offset + 4){
            intconv.array[3] = readArray[offset+0];
            intconv.array[2] = readArray[offset+1];
            intconv.array[1] = readArray[offset+2];
            intconv.array[0] = readArray[offset+3];
            telemetryInt[offset/4] = intconv.integer;
            if(debug){Serial.print(offset/4);Serial.print(">");Serial.println(intconv.integer);}
         }
         for(int offset = 32; offset < 76; offset = offset + 4){ 
            floatconv.array[3] = readArray[offset+0];
            floatconv.array[2] = readArray[offset+1];
            floatconv.array[1] = readArray[offset+2];
            floatconv.array[0] = readArray[offset+3];
            telemetryFloat[(offset/4)-8] = floatconv.floatnum;
            if(debug){Serial.print((offset/4)-8);Serial.print(">");Serial.println(floatconv.floatnum);}
         }

          if(debug){
            for(int i = 0; i <= 31; i++){
               Serial.print("1>");
               Serial.print(i);
               Serial.print(":");
               Serial.print(bitRead(telemetryInt[0],i));
               Serial.println();
           }
        }
        return true;
      }
        return false;  
    }

    uint32_t getCoasterCount() {
      if(sendMessage(7,millis(),0) and readMessageid == 8){
        intconv.array[0] = readArray[3];
        intconv.array[1] = readArray[2];
        intconv.array[2] = readArray[1];
        intconv.array[3] = readArray[0];
        return (intconv.integer);
      }
      else{
        return 0;
        }
    }

    String getCoasterName(uint32_t coasterID) {
      byte data[4];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
        
      if(sendMessage(9,millis(),4,data) and readMessageid == 10){
        String result  = "";
        for(int i = 0; i <= readDatasize; i++){
            result = result + (char)readArray[i];
        }
        return (result);
      }
      return "";
    }

    boolean getCurrentCoasterAndNearestStation(){
      if(sendMessage(11,millis(),0) and readMessageid == 12){
        intconv.array[0] = readArray[3];
        intconv.array[1] = readArray[2];
        intconv.array[2] = readArray[1];
        intconv.array[3] = readArray[0];
        currentCoaster = intconv.integer;
        intconv.array[0] = readArray[7];
        intconv.array[1] = readArray[6];
        intconv.array[2] = readArray[5];
        intconv.array[3] = readArray[4];
        nearestStation = intconv.integer;
        return true;
      }
        return false;  
      }

    uint32_t getCurrentCoaster() {
      if(sendMessage(11,millis(),0) and readMessageid == 12){
        intconv.array[0] = readArray[3];
        intconv.array[1] = readArray[2];
        intconv.array[2] = readArray[1];
        intconv.array[3] = readArray[0];
        return (intconv.integer);
      }
        return 0;  
    }

    uint32_t getNearestStation() {
      if(sendMessage(11,millis(),0) and readMessageid == 12){
        intconv.array[0] = readArray[7];
        intconv.array[1] = readArray[6];
        intconv.array[2] = readArray[5];
        intconv.array[3] = readArray[4];
        return (intconv.integer);
      }
        return 0;  
    }

    boolean setEstop(uint32_t coasterID, boolean state) {
      byte data[5];
      intconv.integer = coasterID;  
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      data[4] = (char)state;
      
      if(sendMessage(13,millis(),5,data) and readMessageid == 1){
        return true;
        }
      return false;
    }

    uint32_t getStationState(uint32_t coasterID, uint32_t stationID) {
      byte data[8];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      intconv.integer = stationID;
      data[4] = intconv.array[3];
      data[5] = intconv.array[2];
      data[6] = intconv.array[1];
      data[7] = intconv.array[0];      
      
      if(sendMessage(14,millis(),8,data) and readMessageid == 15){        
        intconv.array[0] = readArray[3];
        intconv.array[1] = readArray[2];
        intconv.array[2] = readArray[1];
        intconv.array[3] = readArray[0];
        stationState = intconv.integer;
        return (intconv.integer);
      }
        return 0;  
    }

    boolean setManualMode(uint32_t coasterID, uint32_t stationID, boolean state) {
      byte data[9];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      intconv.integer = stationID;
      data[4] = intconv.array[3];
      data[5] = intconv.array[2];
      data[6] = intconv.array[1];
      data[7] = intconv.array[0];
      data[8] = (char)state;
      
      if(sendMessage(16,millis(),9,data) and readMessageid == 1){
        return true;
        }
      return false;
    }

    boolean dispatch(uint32_t coasterID, uint32_t stationID) {
      byte data[8];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      intconv.integer = stationID;
      data[4] = intconv.array[3];
      data[5] = intconv.array[2];
      data[6] = intconv.array[1];
      data[7] = intconv.array[0];      
      
      if(sendMessage(17,millis(),8,data) and readMessageid == 1){        
        return true;
      }
        return false;  
    }

    boolean setGates(uint32_t coasterID, uint32_t stationID, boolean state) {
      byte data[9];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      intconv.integer = stationID;
      data[4] = intconv.array[3];
      data[5] = intconv.array[2];
      data[6] = intconv.array[1];
      data[7] = intconv.array[0];
      data[8] = (char)state;
      
      if(sendMessage(18,millis(),9,data) and readMessageid == 1){
        return true;
        }
      return false;
    }

    boolean setHarness(uint32_t coasterID, uint32_t stationID, boolean state) {
      byte data[9];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      intconv.integer = stationID;
      data[4] = intconv.array[3];
      data[5] = intconv.array[2];
      data[6] = intconv.array[1];
      data[7] = intconv.array[0];
      data[8] = (char)state;
      
      if(sendMessage(19,millis(),9,data) and readMessageid == 1){
        return true;
        }
      return false;
    }

    boolean setPlatform(uint32_t coasterID, uint32_t stationID, boolean state) {
      byte data[9];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      intconv.integer = stationID;
      data[4] = intconv.array[3];
      data[5] = intconv.array[2];
      data[6] = intconv.array[1];
      data[7] = intconv.array[0];
      data[8] = (char)state;
      
      if(sendMessage(20,millis(),9,data) and readMessageid == 1){
        return true;
        }
      return false;
    }

    boolean setFlyerCar(uint32_t coasterID, uint32_t stationID, boolean state) {
      byte data[9];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      intconv.integer = stationID;
      data[4] = intconv.array[3];
      data[5] = intconv.array[2];
      data[6] = intconv.array[1];
      data[7] = intconv.array[0];
      data[8] = (char)state;
      
      if(sendMessage(21,millis(),9,data) and readMessageid == 1){
        return true;
        }
      return false;
    }

    boolean attractionLoadPark(String file, boolean state) {
        byte data[file.length()+1];
        file.getBytes(data, file.length());
        data[file.length()] = (char)state;
        if(sendMessage(24,millis(),file.length()+1,data) and readMessageid == 1){
          return true;
        }
          return false;
    }

    boolean attractionClosePark() {
        if(sendMessage(25,millis(),0) and readMessageid == 1){
          return true;
        }
          return false;
    } 

    boolean quitServer() {
        if(sendMessage(26,millis(),0) and readMessageid == 1){
          return true;
        }
          return false;
    } 

    boolean setPause(boolean state){
      byte data[1];
      data[0] = (char)state;
      if(sendMessage(27,millis(),1,data) and readMessageid == 1){
        return true;  
      }
      return false;
    }

    boolean attractionResetPark(boolean state){
      byte data[1];
      data[0] = (char)state;
      if(sendMessage(28,millis(),1,data) and readMessageid == 1){
        return true;  
      }
      return false;
    }

    boolean selectSeat(uint32_t coasterID, uint32_t trainID, uint32_t carID, uint32_t seatID) {
      byte data[16];
      intconv.integer = coasterID;
      data[0] = intconv.array[3];
      data[1] = intconv.array[2];
      data[2] = intconv.array[1];
      data[3] = intconv.array[0];
      intconv.integer = trainID;
      data[4] = intconv.array[3];
      data[5] = intconv.array[2];
      data[6] = intconv.array[1];
      data[7] = intconv.array[0];   
      intconv.integer = carID;
      data[8] = intconv.array[3];
      data[9] = intconv.array[2];
      data[10] = intconv.array[1];
      data[11] = intconv.array[0]; 
      intconv.integer = seatID;
      data[12] = intconv.array[3];
      data[13] = intconv.array[2];
      data[14] = intconv.array[1];
      data[15] = intconv.array[0];    
      
      if(sendMessage(29,millis(),16,data) and readMessageid == 1){        
        return true;
      }
        return false;  
    }

    boolean attractionSetMode(boolean state){
      byte data[1];
      data[0] = (char)state;
      if(sendMessage(30,millis(),1,data) and readMessageid == 1){
        return true;  
      }
      return false;
    }

    boolean recenterVR() {
        if(sendMessage(31,millis(),0) and readMessageid == 1){
          return true;
        }
          return false;
    } 

    boolean setCustomView(float posX, float posY, float posZ, float azimuth, float elevation, boolean walk) {
      byte data[21];
      floatconv.floatnum = posX;
      data[0] = floatconv.array[3];
      data[1] = floatconv.array[2];
      data[2] = floatconv.array[1];
      data[3] = floatconv.array[0];
      floatconv.floatnum = posY;
      data[4] = floatconv.array[3];
      data[5] = floatconv.array[2];
      data[6] = floatconv.array[1];
      data[7] = floatconv.array[0];
      floatconv.floatnum = posZ;
      data[8] = floatconv.array[3];
      data[9] = floatconv.array[2];
      data[10] = floatconv.array[1];
      data[11] = floatconv.array[0];
      floatconv.floatnum = azimuth;
      data[12] = floatconv.array[3];
      data[13] = floatconv.array[2];
      data[14] = floatconv.array[1];
      data[15] = floatconv.array[0];
      floatconv.floatnum = elevation;
      data[16] = floatconv.array[3];
      data[17] = floatconv.array[2];
      data[18] = floatconv.array[1];
      data[19] = floatconv.array[0];
      data[20] = (char)walk;
      
      if(sendMessage(32,millis(),21,data) and readMessageid == 1){        
        return true;
      }
        return false;  
    }

    boolean telemetryInPlay(){return bitRead(telemetryInt[0],0);}
    boolean telemetryBreaking(){return bitRead(telemetryInt[0],1);}
    boolean telemetryPaused(){return bitRead(telemetryInt[0],2);}
    uint32_t telemetryCurrentFrame(){return (telemetryInt[1]);}    
    uint32_t telemetryViewMode(){return (telemetryInt[2]);}   
    uint32_t telemetryCurrentCoaster(){return (telemetryInt[3]);}   
    uint32_t telemetryCoasterStyle(){return (telemetryInt[4]);}   
    uint32_t telemetryCurrentTrain(){return (telemetryInt[5]);}
    uint32_t telemetryCurrentCar(){return (telemetryInt[6]);}    
    uint32_t telemetryCurrentSeat(){return (telemetryInt[7]);}  
    float telemetrySpeed(){return (telemetryFloat[0]);}
    float telemetryPosX(){return (telemetryFloat[1]);}  
    float telemetryPosY(){return (telemetryFloat[2]);} 
    float telemetryPosZ(){return (telemetryFloat[3]);}   
    float telemetryRotationX(){return (telemetryFloat[4]);}
    float telemetryRotationY(){return (telemetryFloat[5]);}
    float telemetryRotationZ(){return (telemetryFloat[6]);}
    float telemetryRotationW(){return (telemetryFloat[7]);}
    float telemetryGforceX(){return (telemetryFloat[8]);}
    float telemetryGforceY(){return (telemetryFloat[9]);}
    float telemetryGforceZ(){return (telemetryFloat[10]);}
    
    boolean stationEstop(){return bitRead(stationState,0);};
    boolean stationManual(){return bitRead(stationState,1);};
    boolean stationCanDispatch(){return bitRead(stationState,2);};
    boolean stationGatesCanClose(){return bitRead(stationState,3);};
    boolean stationGatesCanOpen(){return bitRead(stationState,4);};
    boolean stationHarnessCanClose(){return bitRead(stationState,5);};
    boolean stationHarnessCanOpen(){return bitRead(stationState,6);};
    boolean stationPlatformCanRaise(){return bitRead(stationState,7);};
    boolean stationPlatformCanLower(){return bitRead(stationState,8);};
    boolean stationFlyerCarCanLock(){return bitRead(stationState,9);};
    boolean stationFlyerCarCanUnlock(){return bitRead(stationState,10);};
    boolean stationTrainInsideStation(){return bitRead(stationState,11);};
    boolean stationTrainInsideStationIsCurrentRideView(){return bitRead(stationState,12);};

};
