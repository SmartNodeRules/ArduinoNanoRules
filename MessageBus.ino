#if FEATURE_MSGBUS
//********************************************************************************************
// Receive message bus packets
//********************************************************************************************
void MSGBusReceive()
{
  int packetSize = portUDP.parsePacket();
  
  if (packetSize > 0 && packetSize < 80)
  {
    byte packetBuffer[80];
    int len = portUDP.read(packetBuffer, packetSize);

    // check if this is a plain text message, do not process other messages
    if (packetBuffer[0] > 127)
      return;

    packetBuffer[packetSize]=0;
    #if FEATURE_SERIAL
      Serial.println((char*)packetBuffer);
    #endif

    String msg = &packetBuffer[0];

    // First process messages that request confirmation
    // These messages start with '>' and must be addressed to my node name
    String mustConfirm = String(">") + Settings.Name + String("/");
    if (msg.startsWith(mustConfirm)) {
      String reply = "<" + msg.substring(1);
      UDPSend(reply);
    }
    if (msg[0] == '>'){
     msg = msg.substring(1); // Strip the '>' request token from the message
    }

    
    // Process confirmation messages
    if (msg[0] == '<'){
      for (byte x = 0; x < CONFIRM_QUEUE_MAX ; x++) {
        if (confirmQueue[x].Name.substring(1) == msg.substring(1)) {
          confirmQueue[x].State = 0;
          break;
        }
      }
      #if FEATURE_SERIAL
        Serial.println(msg);
      #endif
      return; // This message needs no further processing, so return.
    }
    
    // Special MSGBus system events
    if (msg.substring(0, 7) == F("MSGBUS/")){
      String sysMSG = msg.substring(7);
      if (sysMSG.substring(0, 9) == F("Hostname=")){
        //nodelist(remoteIP, sysMSG.substring(9));
      }
      if (sysMSG.substring(0, 7) == F("Refresh")){
        MSGBusAnnounceMe();
      }
    }
    
    rulesProcessing('r', msg);
    
    portUDP.stop();
    portUDP.begin(Settings.Port);
  }
  portUDP.flush();
}


void MSGBusSend(String &msg){
    if (msg[0] == '>') {
      for (byte x = 0; x < CONFIRM_QUEUE_MAX ; x++) {
        if (confirmQueue[x].State == 0) {
          confirmQueue[x].Name = msg;
          confirmQueue[x].Attempts = 9;
          confirmQueue[x].State = 1;
          confirmQueue[x].TimerTicks = 3;
          UDPSend(msg);
          break;
        }
      }
    }
    else
      UDPSend(msg);
}


//********************************************************************************************
// Check MessageBus queue
//********************************************************************************************
void MSGBUSQueue() {
  for (byte x = 0; x < CONFIRM_QUEUE_MAX ; x++) {
    if (confirmQueue[x].State == 1){
      if(confirmQueue[x].Attempts !=0){
        confirmQueue[x].TimerTicks--;
        if(confirmQueue[x].TimerTicks == 0){
          confirmQueue[x].TimerTicks = 3;
          confirmQueue[x].Attempts--;
          UDPSend(confirmQueue[x].Name);
        }
      }
      else{
        #if FEATURE_SERIAL
          Serial.println(F("Confirmation Timeout"));
        #endif
        confirmQueue[x].State = 0;
      }
    }
  }
}


//********************************************************************************************
// Send UDP message
//********************************************************************************************
void UDPSend(String& msg)
{
  IPAddress broadcastIP(255, 255, 255, 255);
  portUDP.beginPacket(broadcastIP, Settings.Port);
  portUDP.print(msg);
  portUDP.endPacket();
  #if FEATURE_SERIAL
    Serial.println(msg);
  #endif
}


//********************************************************************************************
// UDP Send message bus hostname announcement
//********************************************************************************************
void MSGBusAnnounceMe(){
  String msg = F("MSGBUS/Hostname=");
  msg += Settings.Name;
  UDPSend(msg);
}
#endif
