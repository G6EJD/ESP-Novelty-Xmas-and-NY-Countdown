/* ESP8266 novelty / fun countdown clock
 * The 'MIT License (MIT) Copyright (c) 2016 by David Bird'. Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish,  
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the 
 * following conditions:    
 * The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the   
 * software use is visible to an end-user.  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER    
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * See more at http://dsbird.org.uk
*/

#include <Wire.h>
#include "SH1106.h"
#include "OLEDDisplayUi.h"
#include "time.h"
#include "NTPClient.h"
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>

const char* ssid     = "your_SSID";
const char* password = "your_PASSWORD";

String time_str;
int update_cnt=0,newyear=0,xmas=0;
int T1days=0,T1hours=0,T1minutes=0,T1seconds=0;
int T2days=0,T2hours=0,T2minutes=0,T2seconds=0;

SH1106 display(0x3c, D3,D4); // OLED display object definition (address, SDA, SCL) Connect OLED SDA pin to ESP GPIO-5 and OLED SCL pin to GPIO-4 on X-board
OLEDDisplayUi ui ( &display );

WiFiUDP ntpUDP; //** NTP client class
NTPClient timeClient(ntpUDP);

/////////////////////////////////////////////////////////////////////////
// What's displayed along the top line
void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,0, time_str.substring(0,8));  //HH:MM:SS Sat 05-07-17
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128,0, time_str.substring(9));
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
}

// This frame draws a message
void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
   display->drawString(x+64,y+22,"Xmas is coming");
}

// This frame draws a count-down to the big event
void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (T1days <= 0 && T1hours <= 0 && T1minutes <= 0)
  {
    display->drawString(x+64,y+22,"Merry Xmas");
  }
  else
  {
    display->drawString(x+64,y+12,(T1days<10?"0"+String(T1days):String(T1days))+" Days");
    display->drawString(x+64,y+32,(T1hours%24<10?"0"+String(T1hours%24):String(T1hours%24))+":"+(T1minutes<10?"0"+String(T1minutes):String(T1minutes))+"Hr to go!");
  }
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->drawString(x+64,y+22,"New Year 2018"); 
}

// This frame draws a count-down to the big event
void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  if (T2days <= 0 && T2hours <= 0 && T2minutes <= 0)
  {
    display->drawString(x+64,y+22,"Happy New Year!");
  }
  else
  {
    display->drawString(x+64,y+12,(T2days<10?"0"+String(T2days):String(T2days))+" Days");
    display->drawString(x+64,y+32,(T2hours%24<10?"0"+String(T2hours%24):String(T2hours%24))+":"+(T2minutes<10?"0"+String(T2minutes):String(T2minutes))+"Hr to go!");
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// This array keeps function pointers to all frames are the single views that slide in
FrameCallback frames[] = {drawFrame1, drawFrame2, drawFrame3, drawFrame4};

// how many frames are there?
int frameCount = 4;

// Overlays are drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void setup() {
  Serial.begin(115200);
  Wire.begin(D3,D4); // SDA, SCL
  if (!StartWiFi(ssid,password)) Serial.println("Failed to start WiFi Service after 20 attempts");;
  SetupTime();
  while (!UpdateTime());  //Get the latest time
  // An ESP is capable of rendering 60fps in 80Mhz mode but leaves little time for anything else, run at 160Mhz mode or just set it to about 30 fps
  ui.setTargetFPS(10);
  ui.setIndicatorPosition(BOTTOM);         // You can change this to TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorDirection(LEFT_RIGHT);    // Defines where the first frame is located in the bar
  ui.setFrameAnimation(SLIDE_LEFT);        // You can change the transition that is used SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrames(frames, frameCount);        // Add frames
  ui.setOverlays(overlays, overlaysCount); // Add overlays
  ui.init(); // Initialising the UI will init the display too.
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void loop() {
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) 
  { // Do some work here if required, may as well!
    UpdateTime();
    update_cnt++;
    if (update_cnt%20 == 0) timeClient.forceUpdate();     /* Don't need t odo this that often */
    xmas        = 1514160000 - timeClient.getEpochTime(); /* Total seconds to go to xmas day and UNIX time for xmas day is 1514160000 */
    T1minutes   = xmas      / 60;                         /* Calculate minutes */ 
    T1seconds  -= T1minutes * 60;                         /* Correct seconds */ 
    T1hours     = T1minutes / 60;                         /* Calculate hours */ 
    T1minutes  -= T1hours   * 60;                         /* Correct minutes */
    T1days      = T1hours   / 24;                         /* Calculate days */ 
    newyear     = 1514678400 - timeClient.getEpochTime(); /* Total seconds to go to new years day and UNIX time for new years day is 1514678400 */
    T2minutes   = newyear   / 60;                         /* Calculate minutes */ 
    T2seconds  -= T2minutes * 60;                         /* Correct seconds */ 
    T2hours     = T2minutes / 60;                         /* Calculate hours   */ 
    T2minutes  -= T2hours   * 60;                         /* Correct minutes   */
    T2days      = T2hours   / 24;                         /* Calculate days    */ 
    delay(remainingTimeBudget);
  }
}

void SetupTime(){
  configTime(0 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  // Change this line to suit your time zone, e.g. USA EST configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  // Change this line to suit your time zone, e.g. AUS configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println(F("\nWaiting for time"));
  while (!time(nullptr)) {
    delay(500);
  }
  Serial.println("Time set");
  timeClient.begin();
}

String UpdateTime(){
  time_t now = time(nullptr);
  struct tm *now_tm;
  int hour, min, second, day, month, year, dow;
  now = time(NULL);
  now_tm = localtime(&now);
  hour   = now_tm->tm_hour;
  min    = now_tm->tm_min; 
  second = now_tm->tm_sec; 
  day    = now_tm->tm_mday;
  month  = now_tm->tm_mon;
  year   = now_tm->tm_year + 1900;
  String days[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  dow    = ((timeClient.getEpochTime()/ 86400L)+4)%7; 
  time_str = (hour<10?"0"+String(hour):String(hour))+":"+(min<10?"0"+String(min):String(min))+":"+(second<10?"0"+String(second):String(second));
  time_str = time_str+" "+days[dow]+"  "+(day<10?"0"+String(day):String(day))+":"+
                        (month<10?"0"+String(month):String(month))+":"+
                        (year<10?"0"+String(year):String(year));      // HH:MM:SS Sat 05-07-17
  Serial.println(time_str);
  return time_str;
}

byte calc_dow(int y, int m, int d) {
       static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
       y -= m < 3;
       return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

int StartWiFi(const char* ssid, const char* password){
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: "+String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if(connAttempts > 20) return false;
    connAttempts++;
  }
  Serial.print("WiFi connected\r\nIP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

