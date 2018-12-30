//#define MY_IP               {192,168,0,254}     // default IP if no IP setting
//#define NAME                "NanoRules"         // Enter your device friendly name
//#define DELAY               60                  // Enter your Send delay in seconds
//#define MAC                 0                   // 6th mac digit, must be UNIQUE for each unit!
//#define UDP_PORT            65501

#define FEATURE_SERIAL                   false
#define FEATURE_NETWORK                  true
#define FEATURE_MSGBUS                   true
#define FEATURE_WEB                      true
#define P001                             true
#define P002                             true

#define CONFIRM_QUEUE_MAX                   1
#define FILESIZE_MAX                      512 // max storage in eeprom for boot or rules
#define RULES_TIMER_MAX                     4
#define PLUGIN_MAX                          4
#define USER_VAR_MAX                        4

#define PLUGIN_INIT                         2
#define PLUGIN_READ                         3
#define PLUGIN_ONCE_A_SECOND                4
#define PLUGIN_TEN_PER_SECOND               5
#define PLUGIN_WRITE                       13
#define PLUGIN_UNCONDITIONAL_POLL          25

#include <EEPROM.h>

#if FEATURE_NETWORK
  #include <UIPEthernet.h>
  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
#endif

#if FEATURE_MSGBUS
  EthernetUDP portUDP;
#endif

#if FEATURE_WEB
  EthernetServer WebServer = EthernetServer(80);
  EthernetClient client;
#endif

struct SettingsStruct
{
  byte          Mac;
  byte          IP[4];
  char          Name[26];
  uint16_t      Port;
  byte          Debug=0;  
} Settings;

struct nvarStruct
{
  String Name;
  float Value;
  byte Decimals;
} nUserVar[USER_VAR_MAX];

struct confirmQueueStruct
{
  String Name;
  byte Attempts;
  byte State;
  byte TimerTicks;
} confirmQueue[CONFIRM_QUEUE_MAX];

void(*Reboot)(void)=0;
void setNvar(String varName, float value, int decimals = -1);
String parseString(String& string, byte indexFind, char separator = ',');

boolean (*Plugin_ptr[PLUGIN_MAX])(byte, String&, String&);
byte Plugin_id[PLUGIN_MAX];
String dummyString="";

unsigned long timer1s = 0;
unsigned long timer60s = 0;
unsigned long RulesTimer[RULES_TIMER_MAX];
unsigned long uptime = 0;

void setup()
{
  #if FEATURE_SERIAL
    Serial.begin(57600);
  #endif
  String event = F("System#Config");
  rulesProcessing('b', event);

  #if FEATURE_NETWORK
    mac[5] = Settings.Mac; // make sure every unit has a unique mac address
    IPAddress myIP = Settings.IP;
    Ethernet.begin(mac, myIP);
  #endif
  #if FEATURE_MSGBUS
    portUDP.begin(Settings.Port);
  #endif
  #if FEATURE_WEB
    WebServer.begin();
  #endif

  timer1s = millis() + 1000;
  timer60s = millis() + 60000;

  PluginInit();

  event = F("System#Boot");
  rulesProcessing('b', event);
  rulesProcessing('r', event);
}

void loop()
{
  PluginCall(PLUGIN_UNCONDITIONAL_POLL, dummyString,  dummyString);

  if (millis() > timer1s)
  {
    timer1s = millis() + 1000;
    rulesTimers();
  }

  if (millis() > timer60s)
  {
    timer60s = millis() + 60000;
    uptime++;
    #if FEATURE_MSGBUS
      MSGBusAnnounceMe();
    #endif
  }

  #if FEATURE_MSGBUS
    MSGBusReceive();
  #endif
  #if FEATURE_WEB
    WebServerHandleClient();
  #endif
}

