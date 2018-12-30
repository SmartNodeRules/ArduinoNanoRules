void ExecuteCommand(const char *Line)
{
  // first check the plugins
  String cmd = Line;
  String params = "";
  int parampos = getParamStartPos(cmd, 2);
  if (parampos != -1) {
    params = cmd.substring(parampos);
    cmd = cmd.substring(0, parampos - 1);
  }
  if (PluginCall(PLUGIN_WRITE, cmd, params)) {
    return;
  }
  cmd = "";
  params = "";
  
  boolean success = false;
  char TmpStr1[40];
  TmpStr1[0] = 0;
  char Command[20];
  Command[0] = 0;
  int Par1 = 0;
  int Par2 = 0;
  int Par3 = 0;

  GetArgv(Line, Command, 1);
  if (GetArgv(Line, TmpStr1, 2)) Par1 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 3)) Par2 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 4)) Par3 = str2int(TmpStr1);

  // Config commands

  if (strcasecmp_P(Command, PSTR("Config")) == 0)
  {
    success = true;
    if(GetArgv(Line, TmpStr1, 2)){
      
      if (strcasecmp_P(TmpStr1, PSTR("Mac")) == 0){
        Settings.Mac = Par2;
      }

      if (strcasecmp_P(TmpStr1, PSTR("Name")) == 0){
        if(GetArgv(Line, TmpStr1, 3)){
          strcpy(Settings.Name, TmpStr1);
        }
      }

      if (strcasecmp_P(TmpStr1, PSTR("Network")) == 0){
        if(GetArgv(Line, TmpStr1, 3)){
          str2ip(TmpStr1, Settings.IP);
        }
      }

      if (strcasecmp_P(TmpStr1, PSTR("Port")) == 0){
        Settings.Port = Par2;
      }

      if (strcasecmp_P(TmpStr1, PSTR("Debug")) == 0){
        Settings.Debug = Par2;
      }
    }
    String strLine = Line;
  }

#if FEATURE_MSGBUS
  if (strcasecmp_P(Command, PSTR("MSGBus")) == 0)
  {
    success = true;
    String msg = Line;
    msg = msg.substring(7);
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
#endif

  if (strcasecmp_P(Command, PSTR("Event")) == 0)
  {
    success = true;
    String event = Line;
    event = event.substring(6);
    rulesProcessing('r', event);
  }

  if (strcasecmp_P(Command, PSTR("ValueSet")) == 0)
  {
    success = true;
    if (GetArgv(Line, TmpStr1, 3))
    {
      float result = 0;
      // far to big for Nano, resort to using long for now...
        // Calculate(TmpStr1, &result)
        // result = atof(TmpStr1);
      result = atol(TmpStr1);

      if (GetArgv(Line, TmpStr1, 2)) {
        String varName = TmpStr1;
        if (GetArgv(Line, TmpStr1, 4))
          setNvar(varName, result, Par3);
        else
          setNvar(varName, result);
      }
    }
  }
  
  if (strcasecmp_P(Command, PSTR("TimerSet")) == 0)
  {
    success = true;
    if(Par2)
      RulesTimer[Par1 - 1] = millis() + (1000 * Par2);
    else
      RulesTimer[Par1 - 1] = 0;
  }
  
  if (strcasecmp_P(Command, PSTR("Reboot")) == 0)
  {
    success = true;
    Reboot();
  }

}
/********************************************************************************************\
  Find positional parameter in a char string
  \*********************************************************************************************/
boolean GetArgv(const char *string, char *argv, int argc)
{
  int string_pos = 0, argv_pos = 0, argc_pos = 0;
  char c, d;

  while (string_pos < strlen(string))
  {
    c = string[string_pos];
    d = string[string_pos + 1];

    if       (c == ' ' && d == ' ') {}
    else if  (c == ' ' && d == ',') {}
    else if  (c == ',' && d == ' ') {}
    else if  (c == ' ' && d >= 33 && d <= 126) {}
    else if  (c == ',' && d >= 33 && d <= 126) {}
    else
    {
      argv[argv_pos++] = c;
      argv[argv_pos] = 0;

      if (d == ' ' || d == ',' || d == 0)
      {
        argv[argv_pos] = 0;
        argc_pos++;

        if (argc_pos == argc)
        {
          return true;
        }

        argv[0] = 0;
        argv_pos = 0;
        string_pos++;
      }
    }
    string_pos++;
  }
  return false;
}


