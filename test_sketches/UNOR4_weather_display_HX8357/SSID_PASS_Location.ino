#define EEPROM_WIFI_ADDR 4096  // about half way of the 8K
#define EEPROM_WEATHER_LOCATION (EEPROM_WIFI_ADDR+128)
#define MAX_LOCATION_NAME_LENGTH 64 

uint16_t compute_checksum() {
  uint16_t sum = 0;
  int index = 0;
  const char *pcstr = wifi_config.ssid;
  for (index = 0; index < 32 && *pcstr; index++) sum += *pcstr++;

  pcstr = wifi_config.pass;
  for (index = 0; index < 32 && *pcstr; index++) sum += *pcstr++;

  return sum;
}

void get_string(const char *title, char *psz) {
  while (Serial.read() != -1) {}
  Serial.println(title);
  while (!Serial.available()) { delay(5); }
  String str = Serial.readString();
  str.trim();
  str.toCharArray(psz, 32);
}

void retrieveWiFiConfigInfo() {
  // Try to read the data from EEPROM.
//  pinMode(2, INPUT_PULLUP);
  delay(5);
  //if (digitalRead(2)) {
    EEPROM.get(EEPROM_WIFI_ADDR, wifi_config);

    uint16_t computed_checksum = compute_checksum();
    //Serial_printf("read SSID: %s, Pass:%s CHK: %u %u\n", wifi_config.ssid, wifi_config.pass, wifi_config.checksum, computed_checksum);

    if (wifi_config.ssid[0] && (computed_checksum == wifi_config.checksum)) return;  // have the data.
 // }

  Serial.println("Enter WiFi Information:");
  get_string("Wifi SSID:", wifi_config.ssid);
  get_string("Password:", wifi_config.pass);
  wifi_config.checksum = compute_checksum();

  EEPROM.put(EEPROM_WIFI_ADDR, wifi_config);
}

struct weather_location_info {
  char name[MAX_LOCATION_NAME_LENGTH];
  uint16_t checksum;
};


void retrieveWeatherLocation() {
  struct weather_location_info loc_info;
  EEPROM.get(EEPROM_WEATHER_LOCATION, loc_info);

  uint16_t sum = 0;
  const char *pcstr = loc_info.name;
  for (int index = 0; index < MAX_LOCATION_NAME_LENGTH && *pcstr; index++) sum += *pcstr++;
  if (loc_info.name[0] && (sum == loc_info.checksum)) {
    weather_city = String(loc_info.name);
  }
}

void updateWeatherLocation() {
  struct weather_location_info loc_info;
  memset(&loc_info, 0, sizeof(loc_info));
  weather_city.toCharArray(loc_info.name, MAX_LOCATION_NAME_LENGTH);
  loc_info.checksum = 0;
  const char *pcstr = loc_info.name;
  for (int index = 0; index < MAX_LOCATION_NAME_LENGTH && *pcstr; index++) loc_info.checksum += *pcstr++;
  EEPROM.put(EEPROM_WEATHER_LOCATION, loc_info);
}


