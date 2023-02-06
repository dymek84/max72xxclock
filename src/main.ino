
#include <Arduino.h>
#include <SPI.h>
#include "RTClib.h"
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "DigitLedDisplay.h"
#include <TM1637Display.h>
/* Pin connections

static const uint8_t D0   = GPIO16;
static const uint8_t D1   = GPIO5; // TM1637Display // SCL
static const uint8_t D2   = GPIO4; // TM1637Display // SDA
static const uint8_t D3   = GPIO0;
static const uint8_t D4   = GPIO2; // DHT
static const uint8_t D5   = GPIO14;  // digits8
static const uint8_t D6   = GPIO12; // max matrix
static const uint8_t D7   = GPIO13; // max matrix
static const uint8_t D8   = GPIO15; // max matrix  // digits8
static const uint8_t D9   = GPIO3; // digits8
static const uint8_t D10  = GPIO1;    */
// =======================================================================
// CHANGE YOUR CONFIG HERE:
// =======================================================================
// const char* ssid     = "TECHLOGICS_7598111110";     // SSID of local network
// const char* password = "9487100100";   // Password on network
const char *ssid = "Damian";       //"Damian";                 // your network SSID (name)
const char *password = "12345678"; //"12345678";               // your network password

// #define SDA 5                      // Pin sda (I2C) (D1 on esp8266)
// #define
#define DHTPIN D4     // what pin we're connected to // GPIO 12 = D6
#define DHTTYPE DHT11 // DHT 22  (AM2302)
// #include <ESP8266WebServer.h>

WiFiClient client;
RTC_PCF8563 rtc;
DHT dht(DHTPIN, DHTTYPE);
String date;
String temperature;
/* DigitLedDisplay::DigitLedDisplay(int dinPin, int csPin, int clkPin);            */
DigitLedDisplay digits8 = DigitLedDisplay(D3, D8, D6);

#define NUM_MAX 4
#define LINE_WIDTH 16
#define ROTATE 90

// for max7219 matrix
#define DIN_PIN 15 // D8
#define CS_PIN 13  // D7
#define CLK_PIN 12 // D6

#include "max7219.h"
#include "fonts.h"
int h, m, s;
DateTime now;

float utcOffset = 1; // Time Zone setting

// for TM1637Display
#define CLK D2 // Define the connections pins:
#define DIO D1

TM1637Display display = TM1637Display(CLK, DIO); // Create display object of type TM1637Display:
// =======================================================================
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
float hum;
// Read temperature as Celsius (the default)
float temp;
int aaa;
void setup()
{
  display.setBrightness(0x0f);
  /* Set the brightness min:1, max:15 */
  digits8.setBright(10);

  /* Set the digit count */
  digits8.setDigitLimit(8);
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 1); // Adjust the brightness between 0 and 15
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  // printdate();
  printStringWithShift("Connecting ", 16);
  Wire.begin();
  rtc.begin();
  // rtc.adjust(DateTime(2023, 1, 26, 23, 50, 0));
  now = rtc.now();
  while (WiFi.status() != WL_CONNECTED)
  {
    aaa++;
    delay(500);
      Serial.println(",.");
    if (aaa >= 3)
    {
      break;
    }
  }
    Serial.print("Connected: ");
    Serial.println(WiFi.localIP());
}
// =======================================================================
#define MAX_DIGITS 16
byte dig[MAX_DIGITS] = {0};
byte digold[MAX_DIGITS] = {0};
byte digtrans[MAX_DIGITS] = {0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx = 0;
int dy = 0;
byte del = 0;

long localEpoc = 0;
long localMillisAtUpdate = 0;
float roundoff(float value, unsigned char prec)
{
  float pow_10 = pow(10.0f, (float)prec);
  return round(value * pow_10) / pow_10;
}
void digit8TEST()
{

  /* Prints data to the display */
  digits8.printDigit(12345678);
  delay(500);
  digits8.clear();

  digits8.printDigit(22222222);
  delay(500);
  digits8.clear();

  digits8.printDigit(44444444);
  delay(500);
  digits8.clear();

  for (int i = 0; i < 100; i++)
  {
    digits8.printDigit(i);

    /* Start From Digit 4 */
    digits8.printDigit(i, 4);
    delay(50);
  }

  for (int i = 0; i <= 10; i++)
  {
    /* Display off */
    digits8.off();
    delay(150);

    /* Display on */
    digits8.on();
    delay(150);
  }

  /* Clear all display value */
  digits8.clear();
  delay(500);

  for (long i = 0; i < 100; i++)
  {
    digits8.printDigit(i);
    delay(25);
  }

  for (int i = 0; i <= 20; i++)
  {
    /* Select Digit 5 and write B01100011 */
    digits8.write(5, B01100011);
    delay(200);

    /* Select Digit 5 and write B00011101 */
    digits8.write(5, B00011101);
    delay(200);
  }

  /* Clear all display value */
  digits8.clear();
  delay(500);
}
// =======================================================================
void loop()
{
  digit8TEST();
  if (dots)
  {
    display.showNumberDec(h * 100 + m, false);
  }
  else
  {
    display.showNumberDecEx(h * 100 + m, 0x40, false);
  }
  // Expect: _301
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  hum = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temp = dht.readTemperature();

  temperature = "     ";
  temperature += "H: ";
  temperature += roundoff(hum, 1);
  temperature += "%  T: ";
  temperature += roundoff(temp, 1);
  temperature += "°C ";
  if (updCnt <= 0)
  { // every 10 scrolls, ~450s=7.5m
    updCnt = 10;
    Serial.println("Getting data ...");
    printStringWithShift("  Getting data", 15);

    getTime();
    Serial.println("Data loaded");
    clkTime = millis();
  }
  if (s > 0 && s < 5)
  {
    printStringWithShift(date.c_str(), 40);
  }
  if (s > 15 && s < 20)
  {
    printStringWithShift(temperature.c_str(), 40);
  }
  else if (s > 30 && s < 35)
  {
    printStringWithShift(date.c_str(), 40);
  }
  else if (s > 45 && s < 50)
  {
    printStringWithShift(temperature.c_str(), 40);
  }
  // if (millis() - clkTime > 20000 && !del && dots)
  // {

  // printStringWithShift(weatherString.c_str(), 40);

  // updCnt--;
  // clkTime = millis();
  //  }
  if (millis() - dotTime > 500)
  {
    dotTime = millis();
    dots = !dots;
  }
  updateTime();
  showAnimClock();

  // Adjusting LED intensity.
  // 12am to 6am, lowest intensity 0
  if ((h == 0) || ((h >= 1) && (h <= 6)))
    sendCmdAll(CMD_INTENSITY, 0);
  // 6pm to 11pm, intensity = 2
  else if ((h >= 18) && (h <= 23))
    sendCmdAll(CMD_INTENSITY, 2);
  // max brightness during bright daylight
  else
    sendCmdAll(CMD_INTENSITY, 10);
}

// =======================================================================

void showSimpleClock()
{
  dx = dy = 0;
  clr();
  showDigit(h / 10, 4, dig7x16);
  showDigit(h % 10, 12, dig7x16);
  showDigit(m / 10, 21, dig7x16);
  showDigit(m % 10, 29, dig7x16);
  showDigit(s / 10, 38, dig7x16);
  showDigit(s % 10, 46, dig7x16);
  setCol(19, dots ? B00100100 : 0);
  setCol(36, dots ? B00100100 : 0);
  refreshAll();
}

// =======================================================================

void showAnimClock()
{

  byte digPos[4] = {1, 8, 17, 25};
  int digHt = 12;
  int num = 4;
  int i;
  if (del == 0)
  {
    del = digHt;
    for (i = 0; i < num; i++)
      digold[i] = dig[i];
    dig[0] = h / 10 ? h / 10 : 10;
    dig[1] = h % 10;
    dig[2] = m / 10;
    dig[3] = m % 10;
    for (i = 0; i < num; i++)
      digtrans[i] = (dig[i] == digold[i]) ? 0 : digHt;
  }
  else
    del--;

  clr();
  for (i = 0; i < num; i++)
  {
    if (digtrans[i] == 0)
    {
      dy = 0;
      showDigit(dig[i], digPos[i], dig6x8);
    }
    else
    {
      dy = digHt - digtrans[i];
      showDigit(digold[i], digPos[i], dig6x8);
      dy = -digtrans[i];
      showDigit(dig[i], digPos[i], dig6x8);
      digtrans[i]--;
    }
  }
  dy = 0;
  setCol(15, dots ? B00100100 : 0);
  refreshAll();
  delay(30);
}

// =======================================================================

void showDigit(char ch, int col, const uint8_t *data)
{
  if (dy<-8 | dy> 8)
    return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if (col + i >= 0 && col + i < 8 * NUM_MAX)
    {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if (!dy)
        scr[col + i] = v;
      else
        scr[col + i] |= dy > 0 ? v >> dy : v << -dy;
    }
}

// =======================================================================

void setCol(int col, byte v)
{
  if (dy<-8 | dy> 8)
    return;
  col += dx;
  if (col >= 0 && col < 8 * NUM_MAX)
    if (!dy)
      scr[col] = v;
    else
      scr[col] |= dy > 0 ? v >> dy : v << -dy;
}

// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i, w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX * 8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX * 8 + i] = 0;
  return w;
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay)
{
  c = convertPolish(c);
  if (c < ' ' || c > '~' + 25)
    return;
  c -= 32;
  int w = showChar(c, font);
  for (int i = 0; i < w + 1; i++)
  {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================
int dualChar = 0;

unsigned char convertPolish(unsigned char _c)
{
  unsigned char c = _c;
  if (c == 196 || c == 197 || c == 195)
  {
    dualChar = c;
    return 0;
  }
  if (dualChar)
  {
    switch (_c)
    {
    case 133:
      c = 1 + '~';
      break; // 'ą'
    case 135:
      c = 2 + '~';
      break; // 'ć'
    case 153:
      c = 3 + '~';
      break; // 'ę'
    case 130:
      c = 4 + '~';
      break; // 'ł'
    case 132:
      c = dualChar == 197 ? 5 + '~' : 10 + '~';
      break; // 'ń' and 'Ą'
    case 179:
      c = 6 + '~';
      break; // 'ó'
    case 155:
      c = 7 + '~';
      break; // 'ś'
    case 186:
      c = 8 + '~';
      break; // 'ź'
    case 188:
      c = 9 + '~';
      break; // 'ż'
    // case 132: c = 10+'~'; break; // 'Ą'
    case 134:
      c = 11 + '~';
      break; // 'Ć'
    case 152:
      c = 12 + '~';
      break; // 'Ę'
    case 129:
      c = 13 + '~';
      break; // 'Ł'
    case 131:
      c = 14 + '~';
      break; // 'Ń'
    case 147:
      c = 15 + '~';
      break; // 'Ó'
    case 154:
      c = 16 + '~';
      break; // 'Ś'
    case 185:
      c = 17 + '~';
      break; // 'Ź'
    case 187:
      c = 18 + '~';
      break; // 'Ż'
    default:
      break;
    }
    dualChar = 0;
    return c;
  }
  switch (_c)
  {
  case 185:
    c = 1 + '~';
    break;
  case 230:
    c = 2 + '~';
    break;
  case 234:
    c = 3 + '~';
    break;
  case 179:
    c = 4 + '~';
    break;
  case 241:
    c = 5 + '~';
    break;
  case 243:
    c = 6 + '~';
    break;
  case 156:
    c = 7 + '~';
    break;
  case 159:
    c = 8 + '~';
    break;
  case 191:
    c = 9 + '~';
    break;
  case 165:
    c = 10 + '~';
    break;
  case 198:
    c = 11 + '~';
    break;
  case 202:
    c = 12 + '~';
    break;
  case 163:
    c = 13 + '~';
    break;
  case 209:
    c = 14 + '~';
    break;
  case 211:
    c = 15 + '~';
    break;
  case 140:
    c = 16 + '~';
    break;
  case 143:
    c = 17 + '~';
    break;
  case 175:
    c = 18 + '~';
    break;
  default:
    break;
  }
  return c;
}

void printStringWithShift(const char *s, int shiftDelay)
{
  while (*s)
  {
    printCharWithShift(*s, shiftDelay);
    s++;
  }
  delay(2000);
}

void getTime()
{
  now = rtc.now();
  h = now.hour();
  m = now.minute();
  s = now.second();
  char buffer[25];
  sprintf(buffer, "%u-%02d-%02d", now.year(), now.month(), now.day());
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  date = "   ";
  date += String(buffer);
  date += "   ";
  date += daysOfTheWeek[now.dayOfTheWeek()];
  date.toUpperCase();
  localMillisAtUpdate = millis();
  localEpoc = (h * 60 * 60 + m * 60 + s);

  WiFiClient client;
  if (!client.connect("www.google.com", 80))
  {
    Serial.println("connection to google failed");

    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10)
  {
    delay(500);
    Serial.println(".");
    repeatCounter++;
  }

  String line;
  client.setNoDelay(false);
  while (client.connected() && client.available())
  {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: "))
    {
      date = "     " + line.substring(6, 17);
      date.toUpperCase();
      //      decodeDate(date);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      //  summerTime = checkSummerTime();
      //   if(h+utcOffset+summerTime>23) {
      //    if(++day>31) { day=1; month++; };  // needs better patch
      //   if(++dayOfWeek>7) dayOfWeek=1;
      //  }
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }
  client.stop();
}

// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = fmod(round(curEpoch + 3600 * utcOffset + 86400L), 86400L);
  h = ((epoch % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}

// =======================================================================\

