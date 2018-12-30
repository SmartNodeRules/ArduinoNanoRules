void rulesProcessing(char section, String& event) {
  boolean match = false;
  boolean codeBlock = false;
  boolean isCommand = false;

  String rule = "";
  int eepromPos = 0;
  if (section == 'r')
    eepromPos = 512;
  int x = 0;
  for (x = eepromPos; x < eepromPos + FILESIZE_MAX; x++) {
    byte data = EEPROM.read(x);
    if (data == 3)
      break;
    if (data != 0)
      rule += (char)data;
    else
    { // rule complete
      rule = parseTemplate(rule, rule.length());
      isCommand = true;
      if (!codeBlock)
      {
        rule.toLowerCase();
        if (rule.startsWith(F("on ")))
        {
          rule = rule.substring(3);
          int split = rule.indexOf(F(" do"));
          if (split != -1)
              rule = rule.substring(0, split);
          if (event.equalsIgnoreCase(rule)) {
            match = true;
            isCommand = false;
            codeBlock = true;
          }
        }
      }
      
      if (rule == F("endon"))
        {
          isCommand = false;
          codeBlock = false;
          match = false;
        }

      if (match && isCommand){
        rule.trim();
        #if FEATURE_SERIAL
          if(Settings.Debug){
            Serial.print(F("ACT: "));
            Serial.println(rule);
          }
        #endif
        ExecuteCommand(rule.c_str());
      }
      rule = "";
    } // rule processing
  }
}

/********************************************************************************************\
  Parse string template
  \*********************************************************************************************/
String parseTemplate(String &tmpString, byte lineSize)
{
  String newString = tmpString;

  // check named uservars
  for (byte x = 0; x < USER_VAR_MAX; x++) {
    String varname = "%" + nUserVar[x].Name + "%";
    String svalue = toString(nUserVar[x].Value, nUserVar[x].Decimals);
    newString.replace(varname, svalue);
  }

  // check named uservar strings
//  for (byte x = 0; x < USER_STRING_VAR_MAX; x++) {
//    String varname = "%" + sUserVar[x].Name + "%";
//    String svalue = String(sUserVar[x].Value);
//    newString.replace(varname, svalue);
//  }

  newString.replace(F("%sysname%"), Settings.Name);
//  newString.replace(F("%systime%"), getTimeString(':'));

  return newString;
}

