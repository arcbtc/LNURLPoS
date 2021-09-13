
#include "SPI.h"
#include "TFT_eSPI.h"
#include <Keypad.h>
#include <string.h>
#include "qrcode.h"
#include <qrcode.h>

String dataId = "";
bool paid = false;
bool shouldSaveConfig = false;
bool down = false;
const char* spiffcontent = "";
String spiffing; 

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
byte rowPins[rows] = {12, 14, 27, 26}; //connect to the row pinouts of the keypad
byte colPins[cols] = {25, 33, 32}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
int checker = 0;
char maxdig[20];

void setup(void) {
  Serial.begin(115200);
  pinMode (2, OUTPUT);
  digitalWrite(2, HIGH);
  tft.begin();
  tft.setRotation(1);
  logo();
  delay(3000);
}

void loop() {
  inputs = "";
  page_input();
  displaySats(); 
  bool cntr = false;
  while (cntr != true){
   char key = keypad.getKey();
   if (key != NO_KEY){
     virtkey = String(key);
     
       if (virtkey == "#"){
        page_processing();

        qrShowCode("ssdfsdf");
        int counta = 0;
         while (settle != true){
           counta++;
           virtkey = String(keypad.getKey());
         
           if (virtkey == "*"){
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(52, 40);
            tft.setTextSize(1);
            tft.setTextColor(TFT_RED);
            tft.println("CANCELLED");
            delay(1000);
            settle = true;
            cntr = true;
           }
           else if (virtkey == "#"){
            makeLNURL();
            delay(1000);
            settle = true;
            cntr = true;
           }
           
           if(counta >30){
            settle = true;
            cntr = true;
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

void page_input()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);      // White characters on black background
  tft.setFreeFont(MIDFONT);
  tft.setCursor(0, 20);
  tft.println("AMOUNT THEN #");
  tft.setCursor(60, 130);
  tft.setFreeFont(SMALLFONT);
  tft.println("TO RESET PRESS *");
}

void page_processing()
{ 
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(49, 40);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.println("PROCESSING");
 
}

void displaySats(){
  inputs += virtkey;
  float temp = float(inputs.toInt()) / 100;
  fiat = String(temp);
  satoshis = temp/conversion;
  int intsats = (int) round(satoshis*100000000.0);
  Serial.println(intsats);
  
  nosats = String(intsats);
  tft.setFreeFont(BIGFONT);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(50, 80);
  tft.println(fiat);
  delay(100);
  virtkey = "";

}

void makeLNURL(){

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
