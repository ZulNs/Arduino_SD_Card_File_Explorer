/*
 * SD_Card_File_Explorer.ino
 *
 * Created 1 June 2020 (amid CoViD-19 outbreaks)
 * by ZulNs
 * @Gorontalo, Indonesia
 * 
 * This code is in the public domain.
 * 
 * https://github.com/ZulNs/Arduino_SD_Card_File_Explorer
 */

#include <SPI.h>
#include <SD.h>

#define SDCARD_CS_PIN  10

enum SERIAL_STATE {
  BEGIN,
  COMMAND,
  SPACE,
  PARAM,
  AVOID
};

String activePath;
String chkFile = "";
bool isReadySD = false;

void setup() {
  Serial.begin(57600);
  while (!Serial); // wait for serial port to connect. Needed for native USB port only
  
  Serial.println(F("*** Arduino SD Card File Explorer ***"));
  
  Serial.println(F("\nTo begin, please send any word via Serial Monitor..."));
}

void loop() {
  static String command = "";
  static String param = "";
  static SERIAL_STATE serialStat = BEGIN;
  char inChar;
  File file;
  
  while (Serial.available()) {
    inChar = Serial.read();
    if (inChar == '\n') {
      command.toUpperCase();
      param.toUpperCase();
      
      if (command == "ATTACH") {
        if (isReadySD && chkFileExistence())
            Serial.println(F("\nSD card already been attached"));
        else {
          if (isReadySD) {
            SD.end();
            isReadySD = false;
          }
          Serial.print(F("\nIntializing SD card..."));
          if (SD.begin(SDCARD_CS_PIN)) {
            Serial.println(F("done"));
            file = SD.open("").openNextFile();
            if (file) {
              chkFile = String(file.name());
              file.close();
            }
            else {
              chkFile = "";
              Serial.println(F("Warning: SD card is empty"));
            }
            isReadySD = true;
            activePath = "";
          }
          else
            Serial.println(F("failed"));
        }
      }
      
      else if (command == "EJECT") {
        if (isReadySD) {
          SD.end();
          isReadySD = false;
          Serial.println(F("\nSD card ejected"));
        }
        else
          Serial.println(F("\nSD card already been ejected"));
      }
      
      else if (command == "DIR")
        printDir(param);
      
      else if (command == "CD")
        chgDir(param);
      
      else if (command == "DUMP")
        dumpFile(param);
      
      else if (command == "HELP") {
        Serial.println(F("\nAll available commands:"));
        Serial.println(F("\"ATTACH\""));
        Serial.println(F("   Must be called first before all other commands or after inserting a new SD card."));
        Serial.println(F("\"EJECT\""));
        Serial.println(F("   Must be called before ejecting current SD card."));
        Serial.println(F("\"DIR [directory_path]\""));
        Serial.println(F("   List all available folder or file in the specified directory_path."));
        Serial.println(F("\"CD [directory_path]\""));
        Serial.println(F("   Change current active directory to the specified directory_path."));
        Serial.println(F("\"DUMP file_path\""));
        Serial.println(F("   Printout all the content of the specified file_path."));
      }

      else if (command == "TEST") {
        Serial.println("Exists: \"" + String(SD.exists(param)) + "\"");
      }

      else {
        Serial.println("\nUnknown \"" + command + "\" command");
        Serial.println(F("Please try \"HELP\" command"));
      }
      
      command = "";
      param = "";
      serialStat = BEGIN;
    }
    
    else if (inChar == ' ') {
      if (serialStat == COMMAND)
        serialStat = SPACE;
      else if (serialStat == PARAM)
        serialStat = AVOID;
    }
    
    else {
      switch (serialStat) {
        case BEGIN:
          command += inChar;
          serialStat = COMMAND;
          break;
        case COMMAND:
          command += inChar;
          break;
        case SPACE:
          param += inChar;
          serialStat = PARAM;
          break;
        case PARAM:
          param += inChar;
      }
    }
  }
}

void printDir(String &path) {
  String actualPath;
  if (!chkPath(actualPath, path))
    return;
  File dir = SD.open(actualPath);
  if (!dir.isDirectory()) {
    getParentPath(actualPath);
    Serial.println("\nDirectory of \"/" + actualPath + "\":");
    Serial.print(String(dir.name()) + '\t');
    if (String(dir.name()).length() < 8)
      Serial.print('\t');
    Serial.println(String(dir.size(), DEC));
  }
  else {
    File entry;
    Serial.println("\nDirectory of \"/" + actualPath + "\":");
    while (true) {
      entry = dir.openNextFile();
      if (!entry)
        break;
      Serial.print(String(entry.name()) + '\t');
      if (String(entry.name()).length() < 8)
        Serial.print('\t');
      Serial.println((entry.isDirectory()) ? "<DIR>" : String(entry.size(), DEC));
      entry.close();
    }
  }
  dir.close();
}

void chgDir(String &path) {
  String actualPath;
  if (!chkPath(actualPath, path))
    return;
  File dir = SD.open(actualPath);
  if (!dir.isDirectory())
    Serial.print("\n\"/" + actualPath + "\" isn't a directory");
  else
    activePath = actualPath;
  dir.close();
  Serial.println("\nActive directory: \"/" + activePath + '\"');
}

void dumpFile(String &path) {
  if (path == "") {
    Serial.println(F("\nNo file to dump"));
    return;
  }
  String actualPath;
  if (!chkPath(actualPath, path))
    return;
  File file = SD.open(actualPath);
  if (file.isDirectory())
    Serial.println("\n\"/" + actualPath + "\" isn't a file");
  else {
    Serial.println("\nContent of \"/" + actualPath + "\":");
    while (file.available())
      Serial.write(file.read());
    Serial.println();
  }
  file.close();
}

bool chkPath(String &actualPath, String &path) {
  if (!chkSDC())
    return false;
  actualPath = getActualPath(path);
  if (actualPath != "" && !SD.exists(actualPath)) {
    Serial.println("\nCan't find \"/" + actualPath + '\"');
    return false;
  }
  return true;
}

bool chkSDC() {
  if (isReadySD) {
    if (chkFileExistence())
      return true;
    else {
      SD.end();
      isReadySD = false;
      Serial.println(F("\nCan't access SD card"));
      Serial.println(F("SD card ejected"));
    }
  }
  else
    Serial.println(F("\nSD card wasn't ready"));
  return false;
}

bool chkFileExistence() {
  if (chkFile == "")
    return true;
  return SD.exists(chkFile);  
}

String getActualPath(String &path) {
  String actualPath = (path.startsWith("/")) ? "" : activePath;
  String dir;
  while (true) {
    dir = getFirstDir(path);
    if (dir == "" && path == "")
      break;
    else if (dir == "..")
      getParentPath(actualPath);
    else if (dir != "" && dir != ".") {
      if (actualPath == "")
        actualPath = dir;
      else
        actualPath += '/' + dir;
    }
  }
  return actualPath;
}

String getFirstDir(String &path) {
  String dir;
  int idx = path.indexOf('/');
  if (idx >= 0) {
    dir = path.substring(0, idx);
    path.remove(0, idx + 1);
  }
  else {
    dir = path;
    path = "";
  }
  return dir;
}

void getParentPath(String &path) {
  int idx = path.lastIndexOf('/');
  if (idx >= 0)
    path.remove(idx);
  else
    path = "";
}
