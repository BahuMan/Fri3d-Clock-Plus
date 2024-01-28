#include <Adafruit_NeoPixel.h>
#include <Badge2020_Accelerometer.h>
#include <Badge2020_TFT.h>
#include <Badge2020_arduino.h>
#include <WiFi.h>
#include <time.h>
#include <sntp.h>

const char* SSID = "MyWifi";
const char* PASSWORD = "xxx";

const char* ntpServer = "time.nist.gov"; //"pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

//only initialized because it might help with the auto-ftf on/off feature
Badge2020_Accelerometer accelerometer;

#define TOUCH0 27
#define TOUCH1 14
#define TOUCH2 13
Badge2020_TFT tft;

#define LED_PIN    2
#define LED_COUNT  5
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Callback function (gets called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  //I *think* we no longer need wifi after the NTP callback
  //this callback (and the #include <sntp.h>) should be removed if you want constant wifi connectivity
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  DrawClockFace();
}

struct tm timeinfo;
struct tm last = {};

void setup(void) {

  initTFT();

  if (!getLocalTime(&timeinfo)) {
    ConnectNTP();    
  }
  else {
    DrawClockFace();
  }

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP

}

void initTFT() {
  tft.init(240, 240);
  tft.setRotation(2);

  tft.writeCommand(ST77XX_SLPIN);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);

  tft.enableBacklight();
}


//customize the look of the clock face:
const int FACE_RADIUS = 100;
const int HOUR_IND = 20;
const int MIN_IND = 5;
const int FACE_COLOR = ST77XX_WHITE;
const int FACE_BG_COLOR = ST77XX_BLACK;

const int SECONDS_HAND_LENGTH = 95;
const int SECONDS_HAND_COLOR = ST77XX_RED;
const int MINUTES_HAND_LENGTH = 85;
const int MINUTES_HAND_COLOR = ST77XX_WHITE;
const int HOURS_HAND_LENGTH = 50;
const int HOURS_HAND_COLOR = ST77XX_WHITE;


void loop() {
  ProcessClock();
  ProcessMenu();
  delay(5);
}


void ConnectNTP () {
  // set notification call-back function
  sntp_set_time_sync_notification_cb( timeavailable );

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  tft.print("WiFi ");
  tft.print(SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }
  tft.println("");
  tft.println("connected");
  tft.println(WiFi.localIP());
}

void DrawClockFace() {  
  tft.fillScreen(FACE_BG_COLOR);

  int mid_x = tft.width() / 2;
  int mid_y = tft.height() / 2;
  
  for (int h = 0; h < 12; h++) {
    float angle = h * 2.0 * PI / 12.0;    
    int begin_x = mid_x + FACE_RADIUS * sin(angle);
    int end_x = mid_x + (FACE_RADIUS + HOUR_IND) * sin(angle);
    int begin_y = mid_y - FACE_RADIUS * cos(angle);
    int end_y = mid_y - (FACE_RADIUS + HOUR_IND) * cos(angle);
    tft.drawLine(begin_x, begin_y, end_x, end_y, FACE_COLOR);
  }    

  for (int m = 0; m < 60; m++) {
    float angle = m * 2.0 * PI / 60.0;    
    int begin_x = mid_x + FACE_RADIUS * sin(angle);
    int end_x = mid_x + (FACE_RADIUS + MIN_IND) * sin(angle);
    int begin_y = mid_y - FACE_RADIUS * cos(angle);
    int end_y = mid_y - (FACE_RADIUS + MIN_IND) * cos(angle);
    tft.drawLine(begin_x, begin_y, end_x, end_y, FACE_COLOR);
  }    

}



void ProcessClock() {
  if(!getLocalTime(&timeinfo)) {
    tft.println("wait4sync");
    delay(400);
    return;
  }

  if (last.tm_sec != timeinfo.tm_sec) {
    
    ///////// SECONDS
    float angle = last.tm_sec * 2.0 * PI / 60.0;
    DrawHand(angle, SECONDS_HAND_LENGTH, true, FACE_BG_COLOR); //erase previous seconds-hand
    angle = timeinfo.tm_sec * 2.0 * PI / 60.0;
    DrawHand(angle, SECONDS_HAND_LENGTH, true, SECONDS_HAND_COLOR);

    ///////// ERASE MINUTES AND HOURS
    if (last.tm_min != timeinfo.tm_min) {
      angle = last.tm_min * 2.0 * PI / 60.0;
      DrawHand(angle, MINUTES_HAND_LENGTH, false, FACE_BG_COLOR); //erase previous minutes-hand

      //since the hours hand also moves (slightly) as the minutes advance, we need to erase the hours hand as well
      angle = ((float) last.tm_hour + last.tm_min/60.0) * 2.0 * PI / 12.0;
      DrawHand(angle, HOURS_HAND_LENGTH, true, FACE_BG_COLOR); //erase previous hours-hand
    }

    ///////// MINUTES
    angle = timeinfo.tm_min * 2.0 * PI / 60.0;
    DrawHand(angle, MINUTES_HAND_LENGTH, false, MINUTES_HAND_COLOR); //always redraw minutes to make sure it wasn't erased by seconds hand

    ///////// HOURS
    angle = ((float) timeinfo.tm_hour + timeinfo.tm_min/60.0) * 2.0 * PI / 12.0;
    DrawHand(angle, HOURS_HAND_LENGTH, true, HOURS_HAND_COLOR); //always redraw hours to make sure it wasn't erased by seconds hand
  }

  last = timeinfo;
  delay(20);  
}
void DrawHand(float angle, int length, bool thic, uint16_t color) {
  int mid_x = tft.width() / 2;
  int mid_y = tft.height() / 2;
 
  int end_x = mid_x + length * sin(angle);
  int end_y = mid_y - length * cos(angle);
  tft.drawLine(mid_x, mid_y, end_x, end_y, color);

  if (thic) {
    if ((angle > (PI / 4.0) && angle < (3.0 * PI / 4.0))
      || (angle > (5.0 * PI / 4.0) && angle < (7.0 * PI / 4.0))) {

        tft.drawLine(mid_x, mid_y+1, end_x, end_y+1, color);
        tft.drawLine(mid_x, mid_y-1, end_x, end_y-1, color);
    }
    else {
      tft.drawLine(mid_x+1, mid_y, end_x+1, end_y, color);    
      tft.drawLine(mid_x-1, mid_y, end_x-1, end_y, color);    
    }
  }
}

int currentMenu = -1;
const char* menuItem[] = { "sleep", "sync", "exit" };
const int menuCount = 3;
#define LED_LOW 20
#define LED_HIGH 255
void ProcessMenu() {
    
  if (currentMenu < 0) {
    if(touchRead(27)<40){ //touch to activate menu
      currentMenu = 0;
      while (touchRead(27)<40) {
        delay(5); //debounce
      }
    }  
    else {
      return;
    }    
  }

  //set LEDs to indicate touch button functionality:
  strip.setPixelColor(0, strip.Color(0, LED_LOW, 0));
  strip.setPixelColor(1, strip.Color(0, 0, LED_LOW));
  strip.setPixelColor(2, strip.Color(LED_LOW, 0, 0));
  strip.show();

  //show colored arrows to indicate menu navigation:
  tft.drawLine(30, 150, 40, 150, ST77XX_GREEN);
  tft.drawLine(30, 150, 35, 145, ST77XX_GREEN);
  tft.drawLine(30, 150, 35, 155, ST77XX_GREEN);
  tft.drawLine(210, 150, 200, 150, ST77XX_RED);
  tft.drawLine(210, 150, 205, 145, ST77XX_RED);
  tft.drawLine(210, 150, 205, 155, ST77XX_RED);

  tft.setCursor(50, 140);
  tft.setTextColor(ST77XX_BLUE);
  tft.print(menuItem[currentMenu]);

  if(touchRead(27)<40){
    strip.setPixelColor(0, strip.Color(0, LED_HIGH, 0));
    strip.show();
    while (touchRead(27)<40) delay(10); //debounce
    strip.setPixelColor(0, strip.Color(0, LED_LOW, 0));
    strip.show();
    MoveMenu(-1);
  }
  if(touchRead(14)<40){
    strip.setPixelColor(1, strip.Color(0, 0, LED_HIGH));
    strip.show();
    while (touchRead(14)<40) delay(10); //debounce
    strip.setPixelColor(1, strip.Color(0, 0, LED_LOW));
    strip.show();
    ActivateMenu();
  }
  if(touchRead(13)<40){
    strip.setPixelColor(2, strip.Color(LED_HIGH, 0, 0));
    strip.show();
    while (touchRead(13)<40) delay(10); //debounce
    strip.setPixelColor(2, strip.Color(LED_LOW, 0, 0));
    strip.show();
    MoveMenu(1);
  }
}

void MoveMenu(int delta) {

  //remove previous text:
  tft.setCursor(50, 140);
  tft.setTextColor(FACE_BG_COLOR);
  tft.print(menuItem[currentMenu]);
  
  currentMenu = (currentMenu + delta + menuCount) % menuCount;

  //print current text:
  tft.setCursor(50, 140);
  tft.setTextColor(ST77XX_BLUE);
  tft.print(menuItem[currentMenu]);

}

void TouchCallBack(){
  //placeholder callback function
}

void ActivateMenu() {
  switch (currentMenu) {
    case (0): //sleep
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.setPixelColor(1, strip.Color(0, 0, 0));
      strip.setPixelColor(2, strip.Color(0, 0, 0));
      strip.show();
      touchAttachInterrupt(TOUCH0, TouchCallBack, 40);
      touchAttachInterrupt(TOUCH1, TouchCallBack, 40);
      touchAttachInterrupt(TOUCH2, TouchCallBack, 40);
      esp_sleep_enable_touchpad_wakeup();
      //esp_sleep_enable_timer_wakeup(30 * 1000000);
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(5, 100);
      tft.println("set backlight to AUTO");
      tft.disableBacklight();
      esp_deep_sleep_start();
      break;
    case (1): //sync time
      ConnectNTP();
      break;
    default:
      currentMenu = -1;
        //clear LEDs
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.setPixelColor(1, strip.Color(0, 0, 0));
      strip.setPixelColor(2, strip.Color(0, 0, 0));
      strip.show();
      DrawClockFace();
      break;
  }
}
