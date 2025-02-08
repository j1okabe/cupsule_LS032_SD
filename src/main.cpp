
#include <SD.h>
#include <SdFat.h>
#include "EspEasyLED.h"
#include "EspEasyTimer.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SharpMemLS032.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include "OneButton.h"
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>

SdFat sd;
SdCard card;
FsVolume volume;

// any pins can be used
#define SHARP_SCK GPIO_NUM_15
#define SHARP_MOSI GPIO_NUM_13
#define SHARP_SS GPIO_NUM_3
#define SHARP_EXTCOM GPIO_NUM_5
#define SHARP_DISP GPIO_NUM_7
#define SHARP_MISO GPIO_NUM_NC
#define WAITTIME 3000
#define Button1_pin GPIO_NUM_0

#define Button2_pin GPIO_NUM_42
#define bz_pin GPIO_NUM_2
#define BLACK 0
#define WHITE 1

#define DBG_SERIAL Serial

// SPIピンの定義
#define SD_MISO 39
#define SD_MOSI 12
#define SD_SCK 14
#define SD_CS 11

char alfabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ!"
                 "#$%&'()*+,-./0123456789:;<=>?@ abcdefghijklmnopqrstuvwxyz[\\]^_`{|}~";
SPIClass mySPI(HSPI);

EspEasyLED *rgbled;
bool ComState = false;
String imgfilenames[64];
int imgcount = 0;
int imgpositon = -1;
unsigned long currentMillis;
unsigned long previousMillis = 0; // 最後に処理を実行した時間を記録する変数
// hw_timer_t* timer = NULL;
// EspEasyTimer timer1(TIMER_GROUP_0, TIMER_0);
// Adafruit_SharpMem display(SHARP_SCK, SHARP_MOSI, SHARP_SS, 336, 536);
Adafruit_SharpMem display(&mySPI, SHARP_SS, 336, 536);
// EspEasyTimer timer1(
//     TIMER_GROUP_0,
//     TIMER_0); // Dual core=APP_CPU_NUM(core1), Single Core=PRO_CPU_NUM(core0)
OneButton button1(Button1_pin, true);
OneButton button2(Button2_pin, true);

void drawBMP(const char *filename, int x, int y);

void IRAM_ATTR onTimer()
{
  ComState = !ComState;
  digitalWrite(SHARP_EXTCOM, ComState);
}

void click_fwd(void)
{
  imgpositon++;
  if (imgpositon >= imgcount)
  {
    imgpositon = 0;
  }
  tone(bz_pin, 2000); // Set the frequency to 1000 Hz
  delay(50);          // Beep for 100 milliseconds
  noTone(bz_pin);     // Stop the beep
  String fname = "/img/" + imgfilenames[imgpositon];
  drawBMP(fname.c_str(), 0, 0);
  display.refresh();
  previousMillis = currentMillis;
}
void click_rew(void)
{

  if (imgpositon <= 0)
  {
    imgpositon = imgcount - 1;
  }
  else
  {
    imgpositon--;
  }
  tone(bz_pin, 2000); // Set the frequency to 1000 Hz
  delay(50);          // Beep for 100 milliseconds
  noTone(bz_pin);     // Stop the beep
  String fname = "/img/" + imgfilenames[imgpositon];
  drawBMP(fname.c_str(), 0, 0);
  display.refresh();
  previousMillis = currentMillis;
}

void setup()
{

  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);

  pinMode(bz_pin, OUTPUT);
  pinMode(GPIO_NUM_46, OUTPUT);
  digitalWrite(GPIO_NUM_46, HIGH);
  pinMode(SHARP_EXTCOM, OUTPUT);
  digitalWrite(SHARP_EXTCOM, LOW);
  pinMode(SHARP_DISP, OUTPUT);
  digitalWrite(SHARP_DISP, LOW);
  button1.attachPress(click_fwd);
  button2.attachPress(click_rew);
  rgbled = new EspEasyLED(GPIO_NUM_21, 1, 20);
  rgbled->showColor(EspEasyLEDColor::GREEN);
  // start & clear the display
  mySPI.begin(SHARP_SCK, SHARP_MISO, SHARP_MOSI, SHARP_SS);
  display.begin();
  display.setRotation(1);
  display.clearDisplay();
  display.fillScreen(BLACK);
  display.setCursor(0, 10);
  display.setTextColor(WHITE);
  display.println(alfabet);
  display.setCursor(0, 36);
  display.setFont(&FreeMono9pt7b);
  display.println(alfabet);

  digitalWrite(SHARP_DISP, HIGH);
  // timer1.begin(onTimer, 250);

  display.println("/img file name");
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS))
  {
    display.println("SD mout failed");
    display.refresh();
    return;
  }

  File root = SD.open("/img");
  File file = root.openNextFile();
  imgcount = 0;
  while (file)
  {
    String filename = file.name();
    if (filename[0] != '.' && filename.endsWith(".bmp"))
    { //  最初の文字が'.'でなく、.bmpで終わる場合のみ追加
      imgfilenames[imgcount] = filename;
      display.println(filename);
      imgcount++;
    }
    file = root.openNextFile();
  }
  display.println("SD /img files count");
  display.println(imgcount);

  display.refresh();

  digitalWrite(SHARP_DISP, HIGH);
}

const long interval = 500; // 500msの間隔
bool ledtoggle = false;
void loop()
{
  currentMillis = millis(); // 現在の時間を取得
  button1.tick();
  button2.tick();

  // 500ms経過したかどうかをチェック
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis; // 最後に処理を実行した時間を更新

    // ここに500msごとに実行したい処理を書く
    display.refresh();
    ledtoggle = !ledtoggle;
    if (ledtoggle)
    {
      rgbled->showColor(EspEasyLEDColor::GREEN);
    }
    else
    {
      rgbled->showColor(EspEasyLEDColor::SILVER);
    }
  }
  delay(1);
}

uint16_t read16(File f)
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}

uint32_t read32(File f)
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}
void drawBMP(const char *filename, int x, int y)
{
  File bmpFile;
  int bmpWidth, bmpHeight;
  uint8_t bmpDepth;
  uint32_t bmpImageoffset;
  uint32_t rowSize;

  boolean goodBmp = false;
  boolean flip = true;
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0, startTime = millis();

  if ((x >= display.width()) || (y >= display.height()))
    return;

  bmpFile = SD.open(filename);
  if (!bmpFile)
  {
    Serial.print(F("File not found"));
    return;
  }

  if (read16(bmpFile) == 0x4D42)
  {

    (void)read32(bmpFile);
    (void)read32(bmpFile);
    bmpImageoffset = read32(bmpFile);

    (void)read32(bmpFile);

    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1)
    {
      bmpDepth = read16(bmpFile);

      if ((bmpDepth == 8) && (read32(bmpFile) == 0))
      {
        goodBmp = true;

        rowSize = (bmpWidth + 3) & ~3;

        if (bmpHeight < 0)
        {
          bmpHeight = -bmpHeight;
          flip = false;
        }

        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= display.width())
          w = display.width() - x;
        if ((y + h - 1) >= display.height())
          h = display.height() - y;

        uint8_t *buffer = new uint8_t[rowSize * bmpHeight]; // 全体バッファを確保

        bmpFile.seek(bmpImageoffset);
        bmpFile.read(buffer, rowSize * bmpHeight); // 全体をバッファに読み込む

        for (row = 0; row < h; row++)
        {
          uint8_t *rowPtr;
          if (flip)
            rowPtr = buffer + (bmpHeight - 1 - row) * rowSize;
          else
            rowPtr = buffer + row * rowSize;

          for (col = 0; col < w; col++)
          {
            uint8_t pixel = rowPtr[col];
            if (pixel == 0)
            {
              display.drawPixel(col, row, pixel);
            }
            else
            {
              display.drawPixel(col, row, 1);
            }
          }
        }
        delete[] buffer; // バッファを解放
      }
    }
  }

  bmpFile.close();
  if (!goodBmp)
  {
    Serial.println(F("BMP format not recognized."));
    display.println(F("BMP format not recognized."));
  }
}
