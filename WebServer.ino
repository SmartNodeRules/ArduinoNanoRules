#if FEATURE_WEB
//********************************************************************************
// Handle webserver requests
//********************************************************************************
void WebServerHandleClient() {
  int freeMem = FreeMem();
  client = WebServer.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    char request[40];
    byte count = 0;
    boolean getrequest = true;
    boolean post = false;
    String webdata = "";
    String arg = "";
    unsigned long timer = millis() + 2000;
    boolean timeOut = false;
    while (client.connected()  && millis() < timer) {
      if(millis() >= timer)
        timeOut = true;
      if (client.available()) {
        char c = client.read();
        if (getrequest)
          if (count < 40)
            request[count++] = c;

        if (c == '\n' && currentLineIsBlank) {
          int eepromPos=0;
          if(client.available()){
            char c1 = client.read(); // should be 'b' or 'r'
            char c2 = client.read(); // should be '='
            if(c2 == '='){
              if(c1 == 'r')
              eepromPos=512;
            }
            post = true;
          }
          while (client.available()) { // post data...
            char c = client.read();
            webdata += c;
            int pos = webdata.indexOf("%0D%0A");
            if(pos != -1){
              String line = webdata.substring(0,pos);
              line = URLDecode(line.c_str());
              for(int x = 0; x < line.length();x++){
                EEPROM.write(eepromPos++, line[x]);
              }
              EEPROM.write(eepromPos++, 0);
              webdata = "";
            }
          }
          if(post){ // write EOT marker
            EEPROM.write(eepromPos++, 3);
          }
          
          byte pos = 5; // for GET, querystring starts at position 5
          if (request[0] == 'P') // for POST, position is 6
            pos = 6;

          // add get querystring to post data
          int posQS = -1;
          for (byte x=0; x<40;x++)
            if (request[x] == '?'){
               posQS = x;
               break;
            }
          if (posQS >= 0)
          {
            for (byte x=posQS; x<count; x++)
              if (request[x] == ' ') request[x]=0; // strip remainder of "GET /?cmd=gpio,9,1 HTTP/1.1
            arg = request;
            arg = arg.substring(posQS + 1);
          }

          addHeader(true,true);

          // to save code, we use a very simple approach where webpage names are single char long
          char cmd = request[pos];
          switch (cmd)
          {
            case 'b':
            case 'r':
              handle_edit(cmd);
              break;
            case 't':
              client.print(F("<BR><a class=\"button-link\" href=\"s\">Reboot</a>"));
              client.print(F("<BR><BR>FreeMem:"));
              client.print(freeMem);
              client.print(F("<BR><BR>Uptime:"));
              client.print(uptime);
              break;
            case 's':
              Reboot();
              break;
            case 'c':
              arg = URLDecode(arg.c_str());
              arg = arg.substring(4); // strip 'cmd='
              ExecuteCommand(arg.c_str());
              addCustom();
              break;
            default:
              addCustom();
          }
          break;
        }

        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          getrequest = false;
          request[count] = 0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(100);
    client.stop();
    if(timeOut)
    {
      Enc28J60.init(mac);
    }
    webdata = "";
  }
}


//********************************************************************************
// Add top menu
//********************************************************************************
void addCustom()
{
  //String event = F("System#Web");
  //rulesProcessing('r',event);
  String rule = "";
  int eepromPos = 512;
  for (int x = eepromPos; x < eepromPos + FILESIZE_MAX; x++) {
    byte data = EEPROM.read(x);
    if (data == 3)
      break;
    if (data != 0)
      rule += (char)data;
    else
    { // rule complete
      rule.trim();
      if (rule.startsWith(F("webPrint"))){
        rule = parseTemplate(rule, rule.length());
        client.println(rule.substring(9));
      }
      if (rule.startsWith(F("webButton"))){
        client.println(F("<a class=\""));
        client.println(parseString(rule,2,';'));
        client.println(F("\" href=\""));
        client.println(parseString(rule,3,';'));
        client.println(F("\">"));
        client.println(parseString(rule,4,';'));
        client.println(F("</a>"));
      }
      rule = "";
    }
  }
}


//********************************************************************************
// Add top menu
//********************************************************************************
void addHeader(boolean showTitle, boolean showMenu)
{
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));  // the connection will be closed after completion of the response
  client.println();
  client.println(F("<meta name=\"viewport\" content=\"width=width=device-width, initial-scale=1\">"));

  client.println(F("<style>"));
  client.println(F("* {font-family:sans-serif; font-size:12pt;}"));
  client.println(F("h1 {font-size: 16pt; color: #07D; margin: 8px 0; font-weight: bold;}"));
  client.println(F(".button-link {padding:5px 15px; background-color:#07D; color:#FFF; border:solid 1px #FFF; text-decoration:none}"));
  client.println(F("th {padding:10px; background-color:black; color:#ffffff;}"));
  client.println(F("td {padding:7px;}"));
  client.println(F("table {color:black;}"));
  client.println(F("</style>"));
  client.println(F("<h1>"));
  client.println(Settings.Name);
  client.println(F("</h1>"));
    
  if (showMenu)
  {
    client.print(F("<a class=\"button-link\" href=\".\">Main</a>"));
    client.print(F("<a class=\"button-link\" href=\"b\">Boot</a>"));
    client.print(F("<a class=\"button-link\" href=\"r\">Rules</a>"));
    client.print(F("<a class=\"button-link\" href=\"t\">Tools</a>"));
  }
  client.print(F("<form method='post'><table>"));
}

//********************************************************************************
// Web Interface boot rules page
//********************************************************************************
void handle_edit(char section) {

  client.println(F("<table><TR><TD>"));
  client.println(F("<form method='post'>"));
  client.print(F("<textarea name='"));
  client.write(section);
  client.println(F("' rows='15' cols='80' wrap='off'>"));

  int eepromPos=0;
  if(section == 'r')
    eepromPos=512;
  int x=0;
  for(x = eepromPos; x < eepromPos+FILESIZE_MAX; x++){
    byte data = EEPROM.read(x);
    if (data == 3)
      break;
    if (data != 0)
      client.write(data);
    else
      client.println();
  }
  client.println(F("</textarea>"));
  client.println(F("<TR><TD>Current size: "));
  client.println(x-eepromPos);
  client.println(F(" characters (Max "));
  client.println(FILESIZE_MAX);
  client.println(F(")"));
  client.println(F("<TR><TD><input class=\"button-link\" type='submit' value='Submit'>"));
  client.println(F("</table></form>"));
}
//********************************************************************************
// Decode special characters in URL of get/post data
//********************************************************************************
String URLDecode(const char *src)
{
  String rString;
  const char* dst = src;
  char a, b;

  while (*src) {

    if (*src == '+')
    {
      rString += ' ';
      src++;
    }
    else
    {
      if ((*src == '%') &&
          ((a = src[1]) && (b = src[2])) &&
          (isxdigit(a) && isxdigit(b))) {
        if (a >= 'a')
          a -= 'a' - 'A';
        if (a >= 'A')
          a -= ('A' - 10);
        else
          a -= '0';
        if (b >= 'a')
          b -= 'a' - 'A';
        if (b >= 'A')
          b -= ('A' - 10);
        else
          b -= '0';
        rString += (char)(16 * a + b);
        src += 3;
      }
      else {
        rString += *src++;
      }
    }
  }
  return rString;
}
#endif

