
#include "SPI.h"
#include "TFT_eSPI.h"
#include <Keypad.h>
#include <string.h>
#include "qrhelper.h"
#include "Bitcoin.h"
#include <Base64.h>
#include <Hash.h>
#include <Conversion.h>
#include <math.h>
#include <esp_sleep.h>
#include "Button2.h"


////////////////////////////////////////////////////////
////////CHANGE! USE LNURLPoS EXTENSION IN LNBITS////////
////////////////////////////////////////////////////////

String server = "https://api.rapaygo.com";
String posId = "sooper";
String key = "secret";
String currency = "USD";
String walletId = "a1627";



////////////////////////////////////////////////////////
////Note: See lines 75, 97, to adjust to keypad size////
////////////////////////////////////////////////////////

//////////////VARIABLES///////////////////


// app variables
int topButtonState = 0;
int bottomButtonState = 0;

String dataId = "";
bool paid = false;
bool shouldSaveConfig = false;
bool down = false;
const char* spiffcontent = "";
String spiffing;
String lnurl;
String choice;
String payhash;
String key_val;
String cntr = "0";
String inputs;
int keysdec;
int keyssdec;
float temp;
String fiat;
float satoshis;
String nosats;
float conversion;
String virtkey;
String payreq;
int randomPin;
bool settle = false;
String preparedURL;

int countIdleMSecs = 0;
int THREE_MIN = 180000;

#include "MyFont.h"

#define BIGFONT &FreeMonoBold24pt7b
#define MIDBIGFONT &FreeMonoBold18pt7b
#define MIDFONT &FreeMonoBold12pt7b
#define SMALLFONT &FreeMonoBold9pt7b
#define TINYFONT &TomThumb


Button2 button;
#define BUTTON_TOP  35

TFT_eSPI tft = TFT_eSPI();
SHA256 h;


//////////////BATTERY///////////////////
const bool shouldDisplayBatteryLevel = true; // Display the battery level on the display?

const byte rows = 4; //four rows
const byte cols = 3; //four columns
char keys[rows][cols] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};


//regular keypad setup
byte rowPins[rows] = {21, 22, 17, 2}; //connect to the row pinouts of the keypad
byte colPins[cols] = {15, 13, 12}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
int checker = 0;
char maxdig[20];


//////////////MAIN///////////////////

void setup(void) {
  Serial.begin(115200);

  button.begin(BUTTON_TOP);
  button.setLongClickHandler(longpress);

  pinMode (2, OUTPUT);
  digitalWrite(2, HIGH);
  h.begin();
  tft.begin();

  //Set to 3 for bigger keypad
  tft.setRotation(1);
  logo();
  delay(3000);
}

void loop() {
  Serial.println("loop");
  inputs = "";
  settle = false;
  displaySats();
  bool cntr = false;
  while (cntr != true) {


    
    char key = keypad.getKey();
  
    if (key != NO_KEY) {
      countIdleMSecs = 0;
      virtkey = String(key);
      if (virtkey == "#") {
        makeLNURL();
        qrShowCode();
        int counta = 0;
        while (settle != true) {
          virtkey = String(keypad.getKey());
          if (virtkey == "*") {
            tft.fillScreen(TFT_BLACK);
            settle = true;
            cntr = true;
          }
          else if (virtkey == "#") {
            showPin();
          }
        }
      }
  
      else if (virtkey == "*") {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_WHITE);
        key_val = "";
        inputs = "";
        nosats = "";
        virtkey = "";
        cntr = "2";
      }
      displaySats();
      
    } else {
      button.loop();
      
      countIdle();
      
      // hibernate timeout
      if (countIdleMSecs>THREE_MIN) {
        esp_deep_sleep_start();
      }
    }
  }
}


void countIdle() {
  countIdleMSecs = countIdleMSecs+50;
//  Serial.println("count "+String(countIdleMSecs));
  delay(50);
}

void longpress(Button2& btn) {
    unsigned int time = btn.wasPressedFor();
    Serial.print("You clicked ");

    //3 secs to sleep
    if (time > 3000) {
      Serial.println("Going to sleep now");
      delay(1000);
      Serial.flush(); 
      esp_deep_sleep_start();
    } else if (time > 1500) {
        Serial.print("a really really long time.");
        
    } else if (time > 1000) {
        Serial.print("a really long time.");
    } else if (time > 500) {
        Serial.print("a long time.");        
    } else {
        Serial.print("long.");        
    }
    Serial.print(" (");        
    Serial.print(time);        
    Serial.println(" ms)");
}


///////////DISPLAY///////////////

void qrShowCode() {
  tft.fillScreen(TFT_WHITE);
  lnurl.toUpperCase();
  const char* lnurlChar = lnurl.c_str();
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(20)];
  qrcode_initText(&qrcode, qrcodeData, 6, 0, lnurlChar);
  for (uint8_t y = 0; y < qrcode.size; y++) {

    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(60 + 3 * x, 5 + 3 * y, 3, 3, TFT_BLACK);
      }
      else {
        tft.fillRect(60 + 3 * x, 5 + 3 * y, 3, 3, TFT_WHITE);
      }
    }
  }
}

void showPin()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(SMALLFONT);
  tft.setCursor(0, 20);
  tft.println("PAYMENT PROOF PIN");
  tft.setCursor(60, 80);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setFreeFont(BIGFONT);
  tft.println(randomPin);
}

void displaySats() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);      // White characters on black background
  tft.setFreeFont(MIDFONT);
  tft.setCursor(0, 20);
  tft.println("AMOUNT THEN #");
  tft.setCursor(60, 130);
  tft.setFreeFont(SMALLFONT);
  tft.println("TO RESET PRESS *");

  inputs += virtkey;
  float amount = float(inputs.toInt()) / 100;
  tft.setFreeFont(MIDFONT);
  tft.setCursor(0, 80);
  tft.print(String(currency) + ":");
  tft.setFreeFont(MIDBIGFONT);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(amount);
  displayBatteryVoltage();
  delay(100);
  virtkey = "";


}

void logo() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);      // White characters on black background
  tft.setFreeFont(BIGFONT);
  tft.setCursor(7, 70);      // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("rapaygo");         // Using tft.print means text background is NEVER rendered

  tft.setTextColor(TFT_GREEN, TFT_BLACK);      // White characters on black background
  tft.setFreeFont(SMALLFONT);
  tft.setCursor(7, 90);      // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("https://rapaygo.com");         // Using tft.print means text background is NEVER rendered
  tft.setCursor(7, 105);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("id: "+String(walletId));
  displayBatteryVoltage();
}

void to_upper(char * arr) {
  for (size_t i = 0; i < strlen(arr); i++)
  {
    if (arr[i] >= 'a' && arr[i] <= 'z') {
      arr[i] = arr[i] - 'a' + 'A';
    }
  }
}

/**
   Display the battery voltage
*/
void displayBatteryVoltage() {
  if (shouldDisplayBatteryLevel) {
    delay(10);
    uint16_t v1 = analogRead(34);
    float v1Voltage = ((float)v1 / 4095.0f) * 2.0f * 3.3f * (1100.0f / 1000.0f);


    // 80%
    if (v1Voltage >= 4.02) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    // 50%
    else if (v1Voltage >= 3.84) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    } else {
      tft.setTextColor(TFT_RED, TFT_BLACK);
    }
    tft.setFreeFont(SMALLFONT);
    tft.setCursor(195, 16);

    int percent = round(((float)v1Voltage / 5.0f) * 100.0f);
    if(percent>100){
      percent = 100;
    }
    String batteryPercent = String(percent);
    tft.print(batteryPercent);
  }
}


//////////LNURL AND CRYPTO///////////////
////VERY KINDLY DONATED BY SNIGIREV!/////

void makeLNURL() {
  randomPin = random(1000, 9999);
  byte nonce[8];
  for (int i = 0; i < 8; i++) {
    nonce[i] = random(9);
  }
  byte payload[8];
  encode_data(payload, nonce, randomPin, inputs.toInt());
  preparedURL = server + "/ln/rapaygo/api/v1/lnurl/";
  preparedURL += toHex(nonce, 8);
  preparedURL += "/";
  preparedURL += toHex(payload, 8);
  preparedURL += "/";
  preparedURL += posId;
  Serial.println(preparedURL);
  char Buf[200];
  preparedURL.toCharArray(Buf, 200);
  char *url = Buf;
  byte * data = (byte *)calloc(strlen(url) * 2, sizeof(byte));
  size_t len = 0;
  int res = convert_bits(data, &len, 5, (byte *)url, strlen(url), 8, 1);
  char * charLnurl = (char *)calloc(strlen(url) * 2, sizeof(byte));
  bech32_encode(charLnurl, "lnurl", data, len);
  to_upper(charLnurl);
  lnurl = charLnurl;
  Serial.println(lnurl);
}

void encode_data(byte output[8], byte nonce[8], int pin, int amount_in_cents) {
  SHA256 h;
  h.write(nonce, 8);
  h.write((byte *)key.c_str(), key.length());
  h.end(output);
  output[0] = output[0] ^ ((byte)(pin & 0xFF));
  output[1] = output[1] ^ ((byte)(pin >> 8));
  for (int i = 0; i < 4; i++) {
    output[2 + i] = output[2 + i] ^ ((byte)(amount_in_cents & 0xFF));
    amount_in_cents = amount_in_cents >> 8;
  }
}


/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
