//=============================================================================
// A Weather display app for the Arduino Uno R4 plus currently ST7796 display
// This sketch was developed to test out different features of the
// UNO R4
//
// Developed by: mjs513 and KurtE
//
// It uses the boards built in Wifi controller to communicate with the
// website open-meteo.com to request location, weather and air quality
// information, which is returned in a JSON format, which is displayed
// on the screen.
//
// This sketch stores, the Wifi-security information as well as
// site location into the 8K EEPROM on the Arduino UNO R4
//
// Note: if the SSID/Password are incorrect, you can force it to
// prompt for this information, by having the D2 pin jumpered to
// ground at program startup.
//
//
// This sketch also uses the touch controller on the display.  If you
// click on the 5-day forecast area on a day, the display will be
// updated to show that day’s information, clicking the back button,
// returns you to the main screen.  If you click on the keyboard
// icon toward the upper right, it brings up a keyboard entry
// area which allows you to type in a new location, which can
// be any location known by open-meteo, including zip codes.
// Alternatively, you can also type in a new location using
// the serial monitor.
//
// The Keyboard code is a modified version of:
// https://github.com/KrisKasprzak/ILI9341_t3_Keypad
//
//
// More information about this sketch can be found up at:
// https://forum.arduino.cc/t/porting-wifi-app-that-displays-weather-from-giga-and-teensy-boards-to-uno-r4-wifi/1460156
//
// While doing this we have done similar programs on some different hardware
// specifically on Teensy 4.x boards using different Wifi Setups.
//     https://github.com/mjs513/Teensy-WiFi-Apps
//
// Thes different sketches are discussed on a few different forum threads:
// https://forum.arduino.cc/t/playing-with-zephyr-v1-0-0-on-giga-and-wifi/1458103
// https://forum.pjrc.com/index.php?threads/teensy-4-x-esp32-stack.78071/
// https://forum.pjrc.com/index.php?threads/call-to-arms-teensy-wifi-true.77099/
//
// Warning: There are no guarantees or warrantees with this sketch.
// Use it at your own risk or hopefully fun.
//=============================================================================

struct image_info {
  const void *image;
  const uint16_t *palette;
  uint8_t image_top;
  uint8_t image_bottom;
};

#define USE_TOUCH
#define USE_KEYBOARD
//#define printForecast
#define orientation 1  // or 1 (landscape)

#include <SPI.h>
#include <EEPROM.h>
#include <DIYables_TFT_Touch_Shield.h>

// Config Touch Pins
#if defined(XPT_TOUCH)
#endif

//#include "ST77XX_renesas_font_Arial.h"
//#include "ST77XX_renesas_font_ArialBold.h"
/******************** GFX FONTS ***********************/
//#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
//#include <Fonts/FreeSans18pt7b.h>
//#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
//#include <Fonts/FreeSansBold18pt7b.h>
//Custom Fonts, https:/rop.nl/truetype2gfx/
//#include "customFonts/FreeSansBold14pt7b.h"
//#include "customFonts/FreeSansBold20pt7b.h"
//#include "customFonts/FreeSans14pt7b.h"
//#include "customFonts/FreeSans20pt7b.h"
//#include "customFonts/FreeSans16pt7b.h"
//#include "customFonts/FreeSansBold16pt7b.h"
#include "customFonts/FreeSans10pt7b.h"
#include "customFonts/FreeSansBold10pt7b.h"

// (Optional) Calibration values. Just in case touch does not work properly,
// run the TouchCalibration example and update the values below.
#define TOUCH_LEFT_X 224 //136
#define TOUCH_RIGHT_X 876 // 907
#define TOUCH_TOP_Y 947
#define TOUCH_BOT_Y 221 // 139
//tft.setTouchCalibration(847, 243, 921, 226);


/**************************************************
* Setup Teensy Wifi                               *
***************************************************/
#include <WiFiS3.h>
#define ARDUINOJSON_ENABLE_PROGMEM 0
#include <ArduinoJson.h>
#include "forwardDecs.h"

// Allocated document capacity
JsonDocument doc;

//#include "arduino_secrets.h"
//char ssid[] = SECRET_SSID;  // your network SSID (name)
//char pass[] = SECRET_PASS;  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;  // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
// IPAddress server(93,184,216,34);  // IP address for example.com (no DNS)


WiFiClient client;

const char *server = "api.open-meteo.com";
const char *geocoding_api_server = "geocoding-api.open-meteo.com";
const char *air_quality_server = "air-quality-api.open-meteo.com";

const int port = 80;
constexpr uint32_t kDHCPTimeout = 15000;


String weather_city = "Disneyland";
String weather_time_zone = "";
double weather_latitude = 0;
double weather_longitude = 0;


/**************************************************
* Define Display pins and sd card Chipselect pin  *
***************************************************/
DIYables_TFT_HX8357D_Shield tft;


#if defined(USE_KEYBOARD)
#include "GFX_Keyboard.h"
// easy way to include fonts but change globally
#define FONT_BUTTON &FreeSans10pt7b  // font for keypad buttons

Keyboard MyKeyboard(&tft);
#endif  //use keyboard
uint16_t ScreenLeft = 30, ScreenRight = 468, ScreenTop = 302, ScreenBottom = 3;

/**************************************************
* Initialize PNG Libraray                         *
***************************************************/
//PNG png;

/************************************************/

uint32_t g_weather_cycle_time_ms = (uint32_t)(60 * 60 * 1000);  // cycle time in MS 10 * 60 *1000;
uint32_t g_last_cycle_time_ms = 0;

// -------------------------------------------------------------------
// print out WiFi status
// -------------------------------------------------------------------
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void Serial_printf(const char *format, ...) {
  char buffer[256];
  va_list ap;
  va_start(ap, format);
  int cb_ret = vsnprintf(buffer, sizeof(buffer), format, ap);
  Serial.write(buffer, cb_ret);
}

void tft_printf(const char *format, ...) {
  uint8_t buffer[256];
  va_list ap;
  va_start(ap, format);
  int cb_ret = vsnprintf((char *)buffer, sizeof(buffer), format, ap);
  tft.write(buffer, cb_ret);
}

const GFXfont *cur_gfx_font = nullptr;
inline void SetTFTFont(const GFXfont *f) {
  cur_gfx_font = f;
  tft.setFont(f);
}



/*************************************************/
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {}
  tft.begin();
  //tft.init(320, 480, 0, 0, ST7796S_RGB);
  tft.setRotation(orientation);  // Landscape (480x320)
                                 //  tft.setOrigin(0,0);
  tft.fillScreen(COLOR_RED);
  delay(1000);
  tft.fillScreen(COLOR_GREEN);
  delay(1000);
  tft.fillScreen(COLOR_BLUE);
  delay(1000);

  // Configure their Touch shield
    tft.setTouchCalibration(TOUCH_LEFT_X, TOUCH_RIGHT_X, TOUCH_TOP_Y, TOUCH_BOT_Y);
  tft.fillScreen(COLOR_BG);
  //  tft.setFont(Arial_10);

  Serial.println("initialization done.");

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  retrieveWiFiConfigInfo();
  retrieveWeatherLocation();

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(wifi_config.ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    //    status = WiFi.begin(ssid, 20);
    status = WiFi.begin(wifi_config.ssid, wifi_config.pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to wifi");

  printWifiStatus();

  appState = (weather_time_zone.length() == 0) ? FETCH_MAP_CITY_TO_LOCATION : FETCH_CURRENT;


#if defined(USE_KEYBOARD)
  MyKeyboard.init(COLOR_BLACK, COLOR_WHITE, COLOR_BLUE, COLOR_DARKGREY, COLOR_DARKGREY, COLOR_NAVY, COLOR_BLACK, FONT_BUTTON);
  MyKeyboard.setTouchLimits(ScreenLeft, ScreenRight, ScreenTop, ScreenBottom);
  // optional methods
  // max input characters is controlled by in the .h file
  // #define MAX_KEYBOARD_CHARS 18
  // change input display color
  MyKeyboard.setDisplayColor(COLOR_WHITE, COLOR_BLUE);
  // want rounded corners?
  // MyKeyboard.setCornerRadius(3);
  // Set initial instructions
  // MyKeyboard.setInitialText("IP 111.222.333.444");
  // MyKeyboard.hideInput(); // for hidden password input
  // optional to populate the input box
  //  strcpy(MyKeyboard.data, "TEXT");
#endif  //use keyboard
}

void loop() {
  switch (appState) {
    case FETCH_MAP_CITY_TO_LOCATION:
      Serial.println("\n[0/3] Map City to Location...");
      if (sendMapCityRequest()) {
        appState = READ_MAP_CITY_TO_LOCATION;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_MAP_CITY_TO_LOCATION:
      if (processIncomingStream(printMapCityData)) {
        appState = FETCH_CURRENT;
      }
      break;

    case FETCH_CURRENT:
      Serial.println("\n[1/3] Requesting Current Weather...");
      if (sendCurrentRequest()) {
        appState = READ_CURRENT;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_CURRENT:
      if (processIncomingStream(printCurrentData)) {
        appState = FETCH_DAILY;  //was FETCH_HOURLY
      }
      break;

    case FETCH_HOURLY:
      Serial.println("\n[2/3] Requesting Hourly Forecast...");
      if (sendHourlyRequest()) {
        appState = READ_HOURLY;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_HOURLY:
      if (processIncomingStream(printHourlyData)) {
        appState = FETCH_DAILY;
      }
      break;

    case FETCH_DAILY:
      Serial.println("\n[3/3] Requesting Daily Forecast...");
      if (sendDailyRequest()) {
        appState = READ_DAILY;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_DAILY:
      if (processIncomingStream(printDailyData)) {
        appState = FETCH_AIR_QUALITY;
      }
      break;

    case FETCH_AIR_QUALITY:
      Serial.println("\n[4/4] Requesting Air Quality...");
      if (sendAirQualityRequest()) {
        appState = READ_AIR_QUALITY;
      } else {
        Serial.println("Connection failed.");
        appState = DONE_APP;
      }
      break;

    case READ_AIR_QUALITY:
      if (processIncomingStream(printAirQualityData)) {
        drawWeatherDashboard();
        Serial.println("All data successfully fetched!");
        Serial.println("Enter City name:");
        appState = DONE_APP;
      }
      break;

    case DONE_APP:
      break;
    default:
      break;
  }


#ifdef USE_TOUCH
  int touchX = 0;
  int touchY = 0;
  bool really_touched = false;
  // Their library takes rotation into account.
  if (tft.getTouch(touchX, touchY)) {
    // maybe try to debounce this a little...
    // make sure it is still pressed for a 1/4th of a second...
    #define TOUCH_DEBOUNCE_TIME 150
    uint32_t touch_start_time = millis();
    really_touched = true;
  
    while ((millis() - touch_start_time) < TOUCH_DEBOUNCE_TIME) {
      delay(25);
      if (!tft.getTouch(touchX, touchY)) {
        really_touched = false;
        break;
      }
    }
  }

  if (really_touched) { 
    Serial_printf("Touched (%d, %d)\n", touchX, touchY);
    // STATE 1: Processing touches on the Main Dashboard
    if (currentScreen == SCREEN_MAIN) {
      int selectedDay = getTouchedForecastCard(touchX, touchY);
      if (selectedDay != -1) {
        //Serial.printf("Opening Detail View for Day %d\n", selectedDay);
        showDayDetailScreen(selectedDay);
        delay(300);  // Debounce touch
      }

      if (isKeyboardClicked(touchX, touchY)) {
        Serial.println("Keyboard Clicked");
        showKeyboard();
      }
    }
    // STATE 2: Processing touches on the Detail Screen
    else if (currentScreen == SCREEN_DETAIL) {
      if (isBackButtonClicked(touchX, touchY)) {
        //Serial.println("Back Button Pressed! Returning to Main Dashboard...");
        showMainDashboard();
        delay(300);  // Debounce touch
      }
    }
  }
#endif
#if defined(USE_KEYBOARD)
  else if (currentScreen == SCREEN_KEYBOARD) {
    if (isBackButtonClicked(touchX, touchY)) {
      //Serial.println("Back Button Pressed! Returning to Main Dashboard...");
      showMainDashboard();
      delay(50);  // Debounce touch
    }
  }
#endif  //use keyboard

  if (Serial.available()) {
    weather_city = Serial.readString();
    weather_city.trim();
    weather_city.replace(' ', '+');
    Serial.print("New City: ");
    Serial.println(weather_city);
    appState = FETCH_MAP_CITY_TO_LOCATION;
    updateWeatherLocation();  // save this out to EEPROM
  }

  // see if we timed out and should start a new read cycle
  uint32_t delta_time = millis() - g_last_cycle_time_ms;
  if (delta_time > g_weather_cycle_time_ms) {
    Serial.println("\n*** Start new read cycle ***");
    appState = (weather_time_zone.length() == 0) ? FETCH_MAP_CITY_TO_LOCATION : FETCH_CURRENT;
    g_last_cycle_time_ms = millis();  // don't keep hitting this
  }
}

// Returns card index (0 to 4) if pressed, or -1 if touch is outside cards
int getTouchedForecastCard(int touchX, int touchY) {
  int colWidth = 85;
  int startX = 40;

  int cardY = 185;
  int cardW = colWidth - 8;  // 77px wide
  int cardH = 128;           // 128px tall

  // Loop through all 5 cards and check bounding boxes
  for (int i = 0; i < 5; i++) {
    int cardX = (startX + (i * colWidth)) - 5;

    if (touchX >= cardX && touchX <= (cardX + cardW) && touchY >= cardY && touchY <= (cardY + cardH)) {
      return i;  // Touched card index
    }
  }

  return -1;  // No forecast card touched
}

bool isBackButtonClicked(int touchX, int touchY) {
  // Back button bounds matching: drawRoundRect(20, 260, 100, 40)
  int btnX = 20;
  int btnY = 260;
  int btnW = 100;
  int btnH = 40;

  return (touchX >= btnX && touchX <= (btnX + btnW) && touchY >= btnY && touchY <= (btnY + btnH));
}


bool isKeyboardClicked(int touchX, int touchY) {
  Serial_printf("Keyboard clicked? %d, %d\n", touchX, touchY);

  int btnX = 420;
  int btnY = 0;
  int btnW = 44;
  int btnH = 30;

  return (touchX >= btnX && touchX <= (btnX + btnW) && touchY >= btnY && touchY <= (btnY + btnH));
}
