
#include "SPI.h"
#include "TFT_eSPI.h"
#include <Keypad.h>
#include <string.h>
#include "qrcode.h"
#include "Bitcoin.h"
#include "utility/segwit_addr.h"

////////CHANGE///////

String lnbits = "https://lnbits.com";
String posId = "9j309f3f320fm042m3f";
String posSecret = "f234gf3rg3rwg3rg3";

/////////////////////

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
String randomPin;
bool settle = false;

// The custom font file attached to this sketch must be included
#include "MyFont.h"

#define BIGFONT &FreeMonoBold24pt7b
#define MIDFONT &FreeMonoBold12pt7b
#define SMALLFONT &FreeMonoBold9pt7b
#define TINYFONT &TomThumb

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

//Set keypad
const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
//21 22 17
byte rowPins[rows] = {12, 13, 15, 2}; //connect to the row pinouts of the keypad
byte colPins[cols] = {17, 22, 21}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
int checker = 0;
char maxdig[20];

void setup(void) {
  Serial.begin(115200);
  pinMode (2, OUTPUT);
  digitalWrite(2, HIGH);
  tft.begin();
  tft.setRotation(3);

  logo();
  delay(3000);
}

void loop() {
  inputs = "";
  settle = false;
  displaySats(); 
  bool cntr = false;
  while (cntr != true){
   char key = keypad.getKey();
   if (key != NO_KEY){
     virtkey = String(key);
     Serial.println(virtkey);
       if (virtkey == "#"){
        randomPin = String(random(1000,9999));
        makeLNURL(lnbits + "/lnurlpos/api/" + posId + "/" + randomPin + "/" + inputs);
        qrShowCode(lnurl);
        int counta = 0;
         while (settle != true){
           virtkey = String(keypad.getKey());
           if (virtkey == "*"){
            tft.fillScreen(TFT_BLACK);
            settle = true;
            cntr = true;
           }
           else if (virtkey == "#"){
            showPin();
           }
         }
       }
      
      else if (virtkey == "*"){
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
    }
  }
}

//helpers

void to_upper(char * arr){
  for (size_t i = 0; i < strlen(arr); i++)
  {
    if(arr[i] >= 'a' && arr[i] <= 'z'){
      arr[i] = arr[i] - 'a' + 'A';
    }
  }
}

void makeLNURL(String XXX){
  char Buf[200];
  XXX.toCharArray(Buf, 200);
  char *url = Buf;
  byte * data = (byte *)calloc(strlen(url)*2, sizeof(byte));
  size_t len = 0;
  int res = convert_bits(data, &len, 5, (byte *)url, strlen(url), 8, 1);

  char * charLnurl = (char *)calloc(strlen(url)*2, sizeof(byte));
  bech32_encode(charLnurl, "lnurl", data, len);
  to_upper(charLnurl);
  lnurl = charLnurl;
}

//display functions

void qrShowCode(String lnurl){
  tft.fillScreen(TFT_WHITE);
  lnurl.toUpperCase();
  const char* lnurlChar = lnurl.c_str();
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(20)];
  qrcode_initText(&qrcode, qrcodeData, 6, 0, lnurlChar);
    for (uint8_t y = 0; y < qrcode.size; y++) {

        // Each horizontal module
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if(qrcode_getModule(&qrcode, x, y)){       
                tft.fillRect(60+3*x, 5+3*y, 3, 3, TFT_BLACK);
            }
            else{
                tft.fillRect(60+3*x, 5+3*y, 3, 3, TFT_WHITE);
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

void displaySats(){
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
  tft.setFreeFont(BIGFONT);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(10, 80);
  tft.println(amount);
  delay(100);
  virtkey = "";
}

void logo(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);      // White characters on black background
  tft.setFreeFont(BIGFONT);
  tft.setCursor(7,70);       // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("LNURLPoS");         // Using tft.print means text background is NEVER rendered

  tft.setTextColor(TFT_PURPLE, TFT_BLACK);      // White characters on black background
  tft.setFreeFont(SMALLFONT);
  tft.setCursor(42,90);       // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("Powered by LNbits");         // Using tft.print means text background is NEVER rendered
}
