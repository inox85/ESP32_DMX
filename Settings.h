#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>
#include <Arduino.h>

class Settings {
  private:
    static Settings* instance;
    Preferences preferences;
    Settings();

  public:
    static Settings* getInstance();

    float writeFloat(const char* key, float value);
    float readFloat(const char* key, float defaultValue = 0.0f);

    String writeString(const char* key, const String& value);
    String readString(const char* key, const String& defaultValue = "");

    int writeInt(const char* key, int value);
    int readInt(const char* key, int defaultValue = 0);
    
};

#endif