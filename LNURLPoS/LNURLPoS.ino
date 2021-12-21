
#include "SPI.h"
#include "TFT_eSPI.h"
#include <Keypad.h>
#include <string.h>
#include "qrcode.h"
#include "Bitcoin.h"
#include <Hash.h>
#include <Conversion.h>

////////////////////////////////////////////////////////
////////CHANGE! USE LNURLPoS EXTENSION IN LNBITS////////
////////////////////////////////////////////////////////

String baseURL = "https://fastapi.satoshigo.app/lnurlpos/api/v1/lnurl/8vMSF3EsX8fvN9CzGtEXj6";
String key = "BHUjF46VKrJ83xc4A7cnbQ";
String currency = "USD";

//////////////BATTERY///////////////////
const bool shouldDisplayBatteryLevel = true; // Display the battery level on the display?
const float batteryMaxVoltage = 4.2;         // The maximum battery voltage. Used for battery percentage calculation
const float batteryMinVoltage = 3.73;        // The minimum battery voltage that we tolerate before showing the warning

////////////////////////////////////////////////////////
////Note: See lines 75, 97, to adjust to keypad size////
////////////////////////////////////////////////////////

//////////////VARIABLES///////////////////

String dataId = "";
bool paid = false;
bool shouldSaveConfig = false;
bool down = false;
const char *spiffcontent = "";
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

#include "MyFont.h"

#define BIGFONT &FreeMonoBold24pt7b
#define MIDBIGFONT &FreeMonoBold18pt7b
#define MIDFONT &FreeMonoBold12pt7b
#define SMALLFONT &FreeMonoBold9pt7b
#define TINYFONT &TomThumb

TFT_eSPI tft = TFT_eSPI();
SHA256 h;

// QR screen colours
int qrScreenBrightness = 180;
uint16_t qrScreenBgColour = tft.color565(qrScreenBrightness, qrScreenBrightness, qrScreenBrightness);

//////////////KEYPAD///////////////////

const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

//Big keypad setup
//byte rowPins[rows] = {12, 13, 15, 2}; //connect to the row pinouts of the keypad
//byte colPins[cols] = {17, 22, 21}; //connect to the column pinouts of the keypad

//LilyGO T-Display-Keyboard
//byte rowPins[rows] = {21, 27, 26, 22}; //connect to the row pinouts of the keypad
//byte colPins[cols] = {33, 32, 25}; //connect to the column pinouts of the keypad

// 4 x 4 keypad setup
//byte rowPins[rows] = {21, 22, 17, 2}; //connect to the row pinouts of the keypad
//byte colPins[cols] = {15, 13, 12}; //connect to the column pinouts of the keypad

//Small keypad setup
byte rowPins[rows] = {21, 22, 17, 2}; //connect to the row pinouts of the keypad
byte colPins[cols] = {15, 13, 12};    //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);
int checker = 0;
char maxdig[20];

//////////////MAIN///////////////////

void setup(void)
{
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  h.begin();
  tft.begin();

  //Set to 3 for bigger keypad
  tft.setRotation(1);

  logo();
  delay(3000);
}

void loop()
{
  inputs = "";
  settle = false;
  displaySats();
  bool cntr = false;
  while (cntr != true)
  {
    displayBatteryVoltage();
    char key = keypad.getKey();
    if (key != NO_KEY)
    {
      virtkey = String(key);
      if (virtkey == "#")
      {
        makeLNURL();
        qrShowCode();
        int counta = 0;
        while (settle != true)
        {
          virtkey = String(keypad.getKey());
          if (virtkey == "*")
          {
            tft.fillScreen(TFT_BLACK);
            settle = true;
            cntr = true;
          }
          else if (virtkey == "#")
          {
            showPin();
          }
          // Handle screen brighten on QR screen
          else if (virtkey == "1")
          {
            adjustQrBrightness("increase");
          }
          // Handle screen dim on QR screen
          else if (virtkey == "4")
          {
            adjustQrBrightness("decrease");
          }
        }
      }

      else if (virtkey == "*")
      {
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

void adjustQrBrightness(String direction)
{
  if (direction == "increase" && qrScreenBrightness >= 0)
  {
    qrScreenBrightness = qrScreenBrightness + 25;
    if (qrScreenBrightness > 255)
    {
      qrScreenBrightness = 255;
    }
  }
  else if (direction == "decrease" && qrScreenBrightness <= 255)
  {
    qrScreenBrightness = qrScreenBrightness - 25;
    if (qrScreenBrightness < 0)
    {
      qrScreenBrightness = 5;
    }
  }
  qrScreenBgColour = tft.color565(qrScreenBrightness, qrScreenBrightness, qrScreenBrightness);
  qrShowCode();
}

///////////DISPLAY///////////////

void qrShowCode()
{
  tft.fillScreen(qrScreenBgColour);
  lnurl.toUpperCase();
  const char *lnurlChar = lnurl.c_str();
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(20)];
  qrcode_initText(&qrcode, qrcodeData, 6, 0, lnurlChar);
  for (uint8_t y = 0; y < qrcode.size; y++)
  {

    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++)
    {
      if (qrcode_getModule(&qrcode, x, y))
      {
        tft.fillRect(60 + 3 * x, 5 + 3 * y, 3, 3, TFT_BLACK);
      }
      else
      {
        tft.fillRect(60 + 3 * x, 5 + 3 * y, 3, 3, qrScreenBgColour);
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

void displaySats()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // White characters on black background
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

  delay(100);
  virtkey = "";
}

void logo()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // White characters on black background
  tft.setFreeFont(BIGFONT);
  tft.setCursor(7, 70);  // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("LNURLPoS"); // Using tft.print means text background is NEVER rendered

  tft.setTextColor(TFT_PURPLE, TFT_BLACK); // White characters on black background
  tft.setFreeFont(SMALLFONT);
  tft.setCursor(42, 90);          // To be compatible with Adafruit_GFX the cursor datum is always bottom left
  tft.print("Powered by LNbits"); // Using tft.print means text background is NEVER rendered
}

void to_upper(char *arr)
{
  for (size_t i = 0; i < strlen(arr); i++)
  {
    if (arr[i] >= 'a' && arr[i] <= 'z')
    {
      arr[i] = arr[i] - 'a' + 'A';
    }
  }
}

/**
 * Display the battery voltage
 */
void displayBatteryVoltage()
{
  if (shouldDisplayBatteryLevel)
  {
    bool showBatteryVoltage = false;
    delay(100);
    uint16_t v1 = analogRead(34);
    float batteryCurV = ((float)v1 / 4095.0f) * 2.0f * 3.3f * (1100.0f / 1000.0f);
    float batteryAllowedRange = batteryMaxVoltage - batteryMinVoltage;
    float batteryCurVAboveMin = batteryCurV - batteryMinVoltage;

    int batteryPercentage = (int)(batteryCurVAboveMin / batteryAllowedRange * 100);

    if (batteryPercentage > 100)
    {
      batteryPercentage = 100;
    }

    int textColour = TFT_GREEN;
    if (batteryPercentage > 70)
    {
      textColour = TFT_GREEN;
    }
    else if (batteryPercentage > 30)
    {
      textColour = TFT_YELLOW;
    }
    else
    {
      textColour = TFT_RED;
    }

    tft.setTextColor(textColour, TFT_BLACK);
    tft.setFreeFont(SMALLFONT);

    int textXPos = 195;
    if (batteryPercentage < 100)
    {
      textXPos = 200;
    }

    tft.fillRect(textXPos - 2, 0, 50, 20, TFT_BLACK);
    tft.setCursor(textXPos, 16);

    // Is the device charging?
    if (batteryCurV > 4.9)
    {
      tft.print("CHRG");
    }
    // Show the current voltage
    else if (batteryPercentage > 10)
    {
      tft.print(String(batteryPercentage) + "%");
    }
    else
    {
      tft.print("LO!");
    }

    if (showBatteryVoltage)
    {
      tft.setFreeFont(SMALLFONT);
      tft.setCursor(155, 36);
      tft.print(String(batteryCurV) + "v");
    }
  }
}

//////////LNURL AND CRYPTO///////////////

void makeLNURL()
{
  randomPin = random(1000, 9999);
  byte nonce[8];
  for (int i = 0; i < 8; i++)
  {
    nonce[i] = random(256);
  }
  byte payload[51]; // 51 bytes is max one can get with xor-encryption
  size_t payload_len = xor_encrypt(payload, sizeof(payload), (uint8_t *)key.c_str(), key.length(), nonce, sizeof(nonce), randomPin, inputs.toInt());
  preparedURL = baseURL + "?p=";
  preparedURL += toBase64(payload, payload_len, BASE64_URLSAFE | BASE64_NOPADDING);
  Serial.println(preparedURL);
  char Buf[200];
  preparedURL.toCharArray(Buf, 200);
  char *url = Buf;
  byte *data = (byte *)calloc(strlen(url) * 2, sizeof(byte));
  size_t len = 0;
  int res = convert_bits(data, &len, 5, (byte *)url, strlen(url), 8, 1);
  char *charLnurl = (char *)calloc(strlen(url) * 2, sizeof(byte));
  bech32_encode(charLnurl, "lnurl", data, len);
  to_upper(charLnurl);
  lnurl = charLnurl;
  Serial.println(lnurl);
}

/*
 * Fills output with nonce, xored payload, and HMAC.
 * XOR is secure for data smaller than the key size (it's basically one-time-pad). For larger data better to use AES.
 * Maximum length of the output in XOR mode is 1+1+nonce_len+1+32+8 = nonce_len+43 = 51 for 8-byte nonce.
 * Payload contains pin, currency byte and amount. Pin and amount are encoded as compact int (varint).
 * Currency byte is '$' for USD cents, 's' for satoshi, 'E' for euro cents.
 * Returns number of bytes written to the output, 0 if error occured.
 */
int xor_encrypt(uint8_t * output, size_t outlen, uint8_t * key, size_t keylen, uint8_t * nonce, size_t nonce_len, uint64_t pin, uint64_t amount_in_cents){
  // check we have space for all the data:
  // <variant_byte><len|nonce><len|payload:{pin}{amount}><hmac>
  if(outlen < 2 + nonce_len + 1 + lenVarInt(pin) + 1 + lenVarInt(amount_in_cents) + 8){
    return 0;
  }
  int cur = 0;
  output[cur] = 1; // variant: XOR encryption
  cur++;
  // nonce_len | nonce
  output[cur] = nonce_len;
  cur++;
  memcpy(output+cur, nonce, nonce_len);
  cur += nonce_len;
  // payload, unxored first - <pin><currency byte><amount>
  int payload_len = lenVarInt(pin) + 1 + lenVarInt(amount_in_cents);
  output[cur] = (uint8_t)payload_len;
  cur++;
  uint8_t * payload = output+cur; // pointer to the start of the payload
  cur += writeVarInt(pin, output+cur, outlen-cur); // pin code
  cur += writeVarInt(amount_in_cents, output+cur, outlen-cur); // amount
  cur++;
  // xor it with round key
  uint8_t hmacresult[32];
  SHA256 h;
  h.beginHMAC(key, keylen);
  h.write((uint8_t *)"Round secret:", 13);
  h.write(nonce, nonce_len);
  h.endHMAC(hmacresult);
  for(int i=0; i < payload_len; i++){
    payload[i] = payload[i] ^ hmacresult[i];
  }
  // add hmac to authenticate
  h.beginHMAC(key, keylen);
  h.write((uint8_t *)"Data:", 5);
  h.write(output, cur);
  h.endHMAC(hmacresult);
  memcpy(output+cur, hmacresult, 8);
  cur += 8;
  // return number of bytes written to the output
  return cur;
}
