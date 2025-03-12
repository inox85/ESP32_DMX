#include "Settings.h"

Settings* Settings::instance = nullptr;

Settings::Settings() {
  preferences.begin("my_settings"); // Namespace per le impostazioni
}

Settings* Settings::getInstance() {
  if (!instance) {
    instance = new Settings();
  }
  return instance;
}

float Settings::writeFloat(const char* key, float value) {
  preferences.putFloat(key, value);
  return preferences.getFloat(key, 0.0F);
}

float Settings::readFloat(const char* key, float defaultValue) {
  return preferences.getFloat(key, defaultValue);
}

String Settings::writeString(const char* key, const String& value) {
  preferences.putString(key, value);
  return preferences.getString(key, "");
}

String Settings::readString(const char* key, const String& defaultValue) {
  return preferences.getString(key, defaultValue);
}

int Settings::writeInt(const char* key, int value) {
  preferences.putInt(key, value);
  return preferences.getInt(key, 0);
}

int Settings::readInt(const char* key, int defaultValue) {
  return preferences.getInt(key, defaultValue);
}
