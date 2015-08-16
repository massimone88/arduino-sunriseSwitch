// sunRiseSetSwitcher.ino
#include <LiquidCrystal.h>
#include <Time.h>
#include <sunriseset.h>
#include <EEPROM.h>
#include <DS3231.h>
#include <Wire.h>
// initialize the library with the numbers of the interface pins
#define CUSTOM_BOARD 1
#define RTC 1
#define INIT 1
//#define SAVE_EEPROM 0
DS3231 Clock;
bool Century=false;
bool h12;
bool PM;


//pins
int pinButtons = A0;
#ifdef CUSTOM_BOARD
int pinLcd = A1; //to connect to pin 15 of lcd after 220 ohm resistor
int pinRele1 = 10;
#else
int pinLcd = 10;
int pinRele1 = 12;
#endif

int pinRele2 = 11;

//state
const int STATE_DAY = 0;
const int STATE_MONTH = 1;
const int STATE_YEAR = 2;
const int STATE_HOUR = 3;
const int STATE_MINUTE = 4;
const int STATE_RELE = 5;
const int STATE_SAVE = 6;
const int STATE_CALC = 7;
const int STATE_RUN = 8;
const int STATE_CHANGE_OPTIONS = 9;
const int NO_BUTTON = -1;
const int BUTTON_OK = 1;
const int BUTTON_BACK = 2;
const int BUTTON_LEFT = 3;
const int BUTTON_RIGHT = 4;
const int BUTTON_UP = 5;
const int BUTTON_DOWN = 6;
const int seconds_threshold = 10;

const float LAT = 45.3166;
const float LONG = 11.916;
const int OFFSET = 2;
const int DAYLIGHTSAVINGS = 0;

const int BOTH_RELE = 0;
const int RELE1 = 1;
const int RELE2 = 2;
const int N_RELE_ANSWERS = 3;


struct SettingsObject{
  float time;
  int stateRele;
};

//global variables
//LCD Keypad shield
#ifndef CUSTOM_BOARD
int SELECT[]  =  {720,760};
int LEFT[]    =  {480,520};
int RIGTH[]   =  {0,20};
int UP[]      =  {120,160};
int DOWN[]    =  {300,350};
int BACK[] = {5000,5001}; //there isn't
int NO_BUTTON_VALUE[] = {1022,1024};
LiquidCrystal lcd(8,9,4,5,6,7);
#else
//custom board
int SELECT[]  =  {5,15};
int LEFT[]    =  {1023,1024};
int RIGTH[]   =  {500,600};
int UP[]      =  {950,980};
int DOWN[]    =  {650,720};
int BACK[] = {980,1010}; //there isn't
int NO_BUTTON_VALUE[] = {0,5};
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
#endif


int var_day = 1;
int var_month = 1;
int var_year = 2014;
int var_hour = 0;
int var_minute = 0;
int var_second = 0;
int sunrise_hour = 1;
int sunrise_minute = 2;
int sunset_hour = 3;
int sunset_minute = 4;
int state = 0;
int buttonPressed = false;
int lastButtonPressed = true;
int last_minute = 0;
int last_day = 0;
int saveSettings = false;
int lcdOn = false;
int lastSecond = 0;
int stateRele = 2;
int positionButton = 0;
bool is_running = false;
SettingsObject settings; //Variable to store custom object read from EEPROM.

//helper_function

String int2string(int integer, int digit) {
  String a;
  if (digit == 2) {
    if (integer < 10) {
      a = String(0) + String(integer);
    }
    else {
      a = String(integer);
    }

  }
  else {
    a = String(integer);
  }
  return a;
}

int getButtonPressed() {
  int keyVal = analogRead(pinButtons);
  //Serial.println(keyVal);
  if (!buttonPressed) {
    if (LEFT[0] <= keyVal && keyVal <= LEFT[1]) {
      buttonPressed = true;
      Serial.println("button left pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_LEFT;
    }
    else if (BACK[0] <= keyVal && keyVal <= BACK[1]) {
      Serial.println("button back pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      buttonPressed = true;
      return BUTTON_BACK;
    }
    else if (UP[0] <= keyVal && keyVal <= UP[1]) {
      buttonPressed = true;
      Serial.println("button up pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_UP;
    }
    else if (DOWN[0] <= keyVal && keyVal <= DOWN[1]) {
      buttonPressed = true;
      Serial.println("button down pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_DOWN;
    }
    else if (RIGTH[0] <= keyVal && keyVal <= RIGTH[1]) {
      buttonPressed = true;
      Serial.println("button right pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_RIGHT;
    }
    else if (SELECT[0] <= keyVal && keyVal <= SELECT[1]) {
      buttonPressed = true;
      Serial.println("button OK pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_OK;
    } 
  }
  //Serial.println(keyVal);
  if (NO_BUTTON_VALUE[0] <= keyVal && keyVal <= NO_BUTTON_VALUE[1]){
    //Serial.println("button released!");
    buttonPressed = false;
  }
  return NO_BUTTON;
}

void adjustDay() {
  if (var_day <= 0) {
    if (var_month == 11 || var_month == 4 || var_month == 6 || var_month == 9) {
      var_day = 30;
    }
    else if (var_month == 2) {
      if (var_year % 4 == 0) {
        var_day = 29;
      }
      else {
        var_day = 28;
      }
    }
    else {
      var_day = 31;
    }
  }
  else if (var_day > 28 and var_month == 2 and var_year % 4 != 0) {
    var_day = 1;
  }
  else if (var_day == 29 and var_month == 2 and var_year % 4 == 0) {
    var_day == 29;
  }
  else if (var_day == 31 and (var_month == 11 || var_month == 4 || var_month == 6 || var_month == 9)) {
    var_day = 1;
  }
  else if (var_day > 31) {
    var_day = 1;
  }
}

void adjustMonth() {
  if (var_month <= 0) {
    var_month = 12;
  }
  if (var_month > 12) {
    var_month = 1;
  }
}

void adjustHour() {
  if (var_hour < 0) {
    var_hour = 23;
  }
  if (var_hour > 23) {
    var_hour = 0;
  }
}

void adjustMinute() {
  if (var_minute < 0) {
    var_minute = 59;
  }
  if (var_minute > 59) {
    var_minute = 0;
  }
}


void printDayTime() {
  lcd.setCursor(0, 0);
  String line1 = "Date: " + int2string(var_day, 2) + "/" + int2string(var_month, 2) + "/" +  int2string(var_year, 4);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  String line2 = "Time: " + int2string(var_hour, 2) + ":" + int2string(var_minute, 2);
  lcd.print(line2);
}
void printSaveSettings() {
  lcd.setCursor(0, 0);
  String line1 = "Save Settings?";
  lcd.print(line1);
  lcd.setCursor(0, 1);
  String line2 = "Yes/No";
  lcd.print(line2);
}

void printReleaySettings() {
  lcd.setCursor(0, 0);
  String line1 = "Which reles use?";
  lcd.print(line1);
  lcd.setCursor(0, 1);
  String line2 = "Rele1/Rele2/Both";
  lcd.print(line2);
}

void printMainScreen() {
  lcd.setCursor(0, 0);
  String line1;
  #ifdef RTC
  line1 =  int2string(Clock.getHour(h12,PM), 2) + ":" + int2string(Clock.getMinute(), 2) + ":" +  int2string(Clock.getSecond(), 2) + "   ";
  line1 += int2string(Clock.getDate(), 2) + "/" + int2string(Clock.getMonth(Century), 2);
  #else
  line1 =  int2string(hour(), 2) + ":" + int2string(minute(), 2) + ":" +  int2string(second(), 2) + "   ";
  line1 += int2string(day(), 2) + "/" + int2string(month(), 2);
  #endif
  lcd.print(line1);
  lcd.setCursor(0, 1);
  String line2 =  "A:" + int2string(sunrise_hour, 2) + ":" + int2string(sunrise_minute, 2) + "  ";
  line2 += "T:" + int2string(sunset_hour, 2) + ":" + int2string(sunset_minute, 2);
  lcd.print(line2);
}
void printChangeQuestion(){
  lcd.setCursor(0, 0);
  String line1 = "What U change?";
  lcd.print(line1);
  lcd.setCursor(0, 1);
  String line2 = "DateTime/Releays";
  lcd.print(line2);

}

void setCursor() {
  switch (state) {
    case STATE_DAY:
      lcd.setCursor(7, 0);
      break;
    case STATE_MONTH:
      lcd.setCursor(10, 0);
      break;
    case STATE_YEAR:
      lcd.setCursor(15, 0);
      break;
    case STATE_HOUR:
      lcd.setCursor(7, 1);
      break;
    case STATE_MINUTE:
      lcd.setCursor(10, 1);
      break;
    case STATE_SAVE:
      if (saveSettings) {
        lcd.setCursor(2, 1);
      }
      else {
        lcd.setCursor(5, 1);
      }
      break;
    case STATE_RELE:
      if (stateRele == RELE1) {
        lcd.setCursor(4, 1);
      }
      else if (stateRele == RELE2) {
        lcd.setCursor(10, 1);
      }
      else if (stateRele == BOTH_RELE) {
        lcd.setCursor(15, 1);
      }
      break;
    case STATE_CHANGE_OPTIONS:
      if (positionButton == 0) {
          lcd.setCursor(7, 1);
        }
        else if (positionButton == 1) {
          lcd.setCursor(15, 1);
        }

  }
}

void calculateSunRiseSunSet() {
  //TODO
  float result = calculateSunRise(var_year, var_month, var_day, LAT, LONG, OFFSET, DAYLIGHTSAVINGS);
  float localT = fmod(24 + result, 24.0); //in printSunrise function
  double hours;
  float minutes = modf(localT, &hours) * 60;
  sunrise_minute = int(minutes);
  sunrise_hour = int(hours);

  result = calculateSunSet(var_year, var_month, var_day, LAT, LONG, OFFSET, DAYLIGHTSAVINGS);
  localT = fmod(24 + result, 24.0); //in printSunrise function
  minutes = modf(localT, &hours) * 60;
  sunset_minute = int(minutes);
  sunset_hour = int(hours);
  Serial.print("sunrise : " + int2string(sunrise_hour, 2) + ":" + int2string(sunrise_minute, 2));
  Serial.println(" \t sunset: " + int2string(sunset_hour, 2) + ":" + int2string(sunset_minute, 2));

}

int isThereDayLight() {
  #ifdef RTC
  return ((sunrise_hour < Clock.getHour(h12, PM) && Clock.getHour(h12, PM) < sunset_hour) ||
          (sunrise_hour == Clock.getHour(h12, PM) && Clock.getMinute() >= sunrise_minute) ||
          (sunset_hour == Clock.getHour(h12, PM) && Clock.getMinute() <= sunset_minute));
  #else
  return ((sunrise_hour < hour() && hour() < sunset_hour) ||
          (sunrise_hour == hour() && minute() >= sunrise_minute) ||
          (sunset_hour == hour() && minute() <= sunset_minute));
  #endif
}

void turnOffReleays(){
  digitalWrite(pinRele1, LOW);
  digitalWrite(pinRele2, LOW);
  Serial.println("light OFF!");
}

void turnOnReleays(){
  Serial.println("turnOnReleays");
  Serial.println("stateRele:" + int2string(stateRele,2));
  switch(stateRele){
    case BOTH_RELE:
      Serial.println("light ON both releays!");
      digitalWrite(pinRele1, HIGH);
      digitalWrite(pinRele2, HIGH);
      break;
    case RELE1:
      Serial.println("light ON Releay1!");
      digitalWrite(pinRele1, HIGH);
      digitalWrite(pinRele2, LOW);
      break;
    case RELE2:
      Serial.println("light ON Releay2");
      digitalWrite(pinRele1, LOW);
      digitalWrite(pinRele2, HIGH);
      break;
  }
}

void doBatchDay(){
  #ifdef RTC
  if (last_day != Clock.getDate()) {
        last_day = Clock.getDate();
        var_month = Clock.getMonth(Century);
        var_year = Clock.getYear();
        var_hour = Clock.getHour(h12, PM);
        var_minute = Clock.getMinute();
        state = STATE_CALC;
      }
  #else
  
  if (last_day != day()) {
        last_day = day();
        var_month = month();
        var_year = year();
        var_hour = hour();
        var_minute = minute();
        state = STATE_CALC;
      }
  #endif    
}

void doBatchMinute(){
  int currentMinute;
  #ifdef RTC
  currentMinute = Clock.getMinute();
  #else
  currentMinute = minute();
  #endif
  if ( last_minute != currentMinute) {
        Serial.println("check if there is sun light or not...");
        last_minute = currentMinute;
        //check sunrisesunset
        if (isThereDayLight()) {
          turnOffReleays();
        }
        else {
          turnOnReleays();
        }
        #ifdef SAVE_EEPROM
        Serial.println("Update the EEPROM..");
        SettingsObject settings2 = {
              now(),
              stateRele
            };
        EEPROM.put(0, settings2);
        Serial.println("Saved!");
        delay(1000);
        Serial.println("Now the value on EEPROM address is..");
        SettingsObject settings3;
        EEPROM.get( 0, settings3);
        Serial.println( settings3.time );
        Serial.println( settings3.stateRele );
        #endif
      }
}

void setup() {
  var_day = var_month = 6;
  var_hour = 19;
  var_minute = 56;
  // Start the I2C interface
  Wire.begin();
  lcd.begin(16, 2);
  pinMode(pinLcd, OUTPUT);
  pinMode(pinRele1, OUTPUT);
  pinMode(pinRele2, OUTPUT);
  digitalWrite(pinLcd, HIGH);
  Serial.begin(9600);
  lcd.blink();
  lcd.setCursor(0, 0);
  lcd.setCursor(0, 1);
  delay(1000);
  lcd.clear();
  printDayTime();
  #ifdef RTC
    var_second=Clock.getSecond();
    var_minute=Clock.getMinute();
    var_hour=Clock.getHour(h12, PM);
    var_day=Clock.getDate();
    var_month=Clock.getMonth(Century);
    var_year=Clock.getYear();
    EEPROM.get(0, stateRele);
    if (var_year != 0 && INIT == 0){
      state= STATE_CALC;
    }
    var_second=00;
    var_minute=30;
    var_hour=10;
    var_day=16;
    var_month=8;
    var_year=2015;
  #else
    EEPROM.get( 0, settings);
    Serial.println( "Read custom object from EEPROM: " );
    Serial.println( settings.time );
    Serial.println( settings.stateRele );
    stateRele = settings.stateRele;
    setTime(settings.time);
    if (stateRele != -1){
      var_day = day();
      var_month = month();
      var_year = year();
      var_hour = hour();
      var_minute = minute();
      var_second = second();
      state= STATE_CALC;
    }
  #endif
}

void loop() {
  lcd.blink();
  if (state < STATE_RELE) {
    if (lastButtonPressed && lastButtonPressed != buttonPressed) {
      //Serial.println("updating lcd strings...");
      printDayTime();
    }
    lastButtonPressed = buttonPressed;
    switch (state) {
      case STATE_DAY:
        //do something
        switch (getButtonPressed()) {
          case BUTTON_DOWN:
            var_day -= 1;
            break;
          case BUTTON_UP:
            var_day += 1;
            break;
          case BUTTON_RIGHT:
            state = STATE_MONTH;
            Serial.println("day set, go to month setting...");
            break;
          case BUTTON_OK:
            state = STATE_SAVE;
            Serial.println("settings saved, go to save/no save question...");
            lcd.clear();
            printSaveSettings();
            break;
          case BUTTON_BACK:
            if( is_running){
              state = STATE_RUN;
            }
            break;
        }
        break;
      case STATE_MONTH:
        //do something
        switch (getButtonPressed()) {
          case BUTTON_DOWN:
            var_month -= 1;
            break;
          case BUTTON_UP:
            var_month += 1;
            break;
          case BUTTON_LEFT:
            state = STATE_DAY;
            Serial.println("return to day setting...");
            break;
          case BUTTON_RIGHT:
            state = STATE_YEAR;
            Serial.println("month set, go to year setting...");
            break;
          case BUTTON_OK:
            state = STATE_SAVE;
            Serial.println("settings saved, go to save/no save question...");
            lcd.clear();
            printSaveSettings();
            break;
          case BUTTON_BACK:
            if( is_running){
              state = STATE_RUN;
            }
            break;
        }
        break;
      case STATE_YEAR:
        //do something
        switch (getButtonPressed()) {
          case BUTTON_DOWN:
            var_year -= 1;
            break;
          case BUTTON_UP:
            var_year += 1;
            break;
          case BUTTON_LEFT:
            state = STATE_MONTH;
            Serial.println("return to month setting...");
            break;
          case BUTTON_RIGHT:
            state = STATE_HOUR;
            Serial.println("year set go to hour setting...");
            break;
          case BUTTON_OK:
            state = STATE_SAVE;
            Serial.println("settings saved, go to save/no save question...");
            lcd.clear();
            printSaveSettings();
            break;
          case BUTTON_BACK:
            if( is_running){
              state = STATE_RUN;
            }
            break;
        }
        break;
      case STATE_HOUR:
        //do something
        switch (getButtonPressed()) {
          case BUTTON_DOWN:
            var_hour -= 1;
            break;
          case BUTTON_UP:
            var_hour += 1;
            break;
          case BUTTON_LEFT:
            state = STATE_YEAR;
            Serial.println("return to year setting...");
            break;
          case BUTTON_RIGHT:
            state = STATE_MINUTE;
            Serial.println("hour set go to minute setting...");
            break;
          case BUTTON_OK:
            state = STATE_SAVE;
            Serial.println("settings saved, go to save/no save question...");
            lcd.clear();
            printSaveSettings();
            break;
          case BUTTON_BACK:
            if( is_running){
              state = STATE_RUN;
            }
            break;
        }
        break;
      case STATE_MINUTE:
        switch (getButtonPressed()) {
          case BUTTON_DOWN:
            var_minute -= 1;
            break;
          case BUTTON_UP:
            var_minute += 1;
            break;
          case BUTTON_LEFT:
            state = STATE_HOUR;
            Serial.println("return to hour setting...");
            break;
          case BUTTON_RIGHT:
            state = STATE_RELE;
            Serial.println("minute set go to releays setting...");
            printReleaySettings();
            break;  
          case BUTTON_OK:
            state = STATE_SAVE;
            Serial.println("settings saved, go to save/no save question...");
            lcd.clear();
            printSaveSettings();
            break;
          case BUTTON_BACK:
            if( is_running){
              state = STATE_RUN;
            }
            break;
        }
        break;
    }
    adjustDay();
    adjustMonth();
    adjustMinute();
    adjustHour();
    setCursor();
  }
  switch (state) {
    case STATE_RELE:
        switch (getButtonPressed()) {
          case BUTTON_DOWN:
            break;
          case BUTTON_UP:
            break;
          case BUTTON_LEFT:
            if (stateRele > 0){
              stateRele = (stateRele - 1) % N_RELE_ANSWERS;
              Serial.println(stateRele);
            }
            else{
              stateRele = N_RELE_ANSWERS - 1;
              Serial.println(stateRele);
              }
            break;
          case BUTTON_RIGHT:  
            stateRele = (stateRele + 1) % N_RELE_ANSWERS;
            Serial.println(stateRele);
            break;
          case BUTTON_OK:
            state = STATE_SAVE;
            Serial.println("settings saved, go to save/no save question...");
            lcd.clear();
            printSaveSettings();
            break;
          case BUTTON_BACK:
            if (is_running){
              state = STATE_RUN;
              Serial.println("return to run state...");  
            }
            else{
              state = STATE_MINUTE;
              Serial.println("return to hour setting...");
            }
            lcd.clear();
            break;
        }
        setCursor();
        break;
    case STATE_SAVE:
      switch (getButtonPressed()) {
        case BUTTON_LEFT:
          saveSettings = true;
          break;
        case BUTTON_RIGHT:
          saveSettings = false;
          break;
        case BUTTON_OK:
          if (saveSettings) {
            Serial.println("settings saved!");
            #ifdef RTC
              Clock.setSecond(00);//Set the second 
              Clock.setMinute(var_minute);//Set the minute 
              Clock.setHour(var_hour);  //Set the hour 
              Clock.setDoW(1);    //Set the day of the week
              Clock.setDate(var_day);  //Set the date of the month
              Clock.setMonth(var_month);  //Set the month of the year
              Clock.setYear(var_hour - 2000);  //Set the year (Last two digits of the year)
            #else
              setTime(var_hour, var_minute, 0, var_day, var_month, var_year);
            #endif

            Serial.println("time set!");
            Serial.println("today is  : " + int2string(var_day, 2) + "/" + int2string(var_month, 2) + "/" + int2string(var_year, 2) + " at " + int2string(var_hour, 2) + ":" + int2string(var_minute, 2));
            Serial.println("Rele option: " + int2string(stateRele,1));
            settings = {
              now(),
              stateRele
            };
            #ifdef SAVE_EEPROM
            Serial.println("Saving the timestamp to EEPROM..!");
            EEPROM.put( 0, settings);
            Serial.println("Saved to address 0");
            #endif

            state = STATE_CALC;
            Serial.println("go to calculating state...");
          }
          else {
            if (is_running){
              state =  STATE_RUN;
            }else {
              state =  STATE_DAY;
            }
          }
          break;
        case BUTTON_BACK:
          state =  STATE_DAY;
          break;
      }
      setCursor();
      break;
    case STATE_CALC:
      lcd.noBlink();
      Serial.println("calculating sunrise and sunset...");
      calculateSunRiseSunSet();
      if (isThereDayLight()) {
        turnOffReleays();

      }
      else {
        turnOnReleays();
      }
      //do something
      state = STATE_RUN;
      Serial.println("change state to run");
      #ifdef RTC
      lastSecond = Clock.getSecond();
      var_second = lastSecond;
      last_minute = Clock.getMinute();
      last_day = Clock.getDate();
      #else
      lastSecond = second();
      var_second = second();
      last_minute = minute();
      last_day = day();
      #endif
      lcdOn = true;
      lcd.clear();
      printMainScreen();
      is_running = true;
      break;
    case STATE_CHANGE_OPTIONS:
      lcdOn = true;
      
      switch (getButtonPressed()) {
        case BUTTON_LEFT:
          positionButton = 0;
          #ifdef RTC
          lastSecond = Clock.getSecond();
          var_second = lastSecond;
          #else
          lastSecond = second();
          var_second = second();
          #endif
          break;
        case BUTTON_RIGHT:
          positionButton = 1;
          #ifdef RTC
          lastSecond = Clock.getSecond();
          var_second = lastSecond;
          #else
          lastSecond = second();
          var_second = second();
          #endif
          break;
        case BUTTON_OK:
          switch(positionButton){
            case 0:
              state = STATE_DAY;
              var_day = last_day;
              var_minute = last_minute;
              lcd.clear();
              printDayTime();
              break;
            case 1:
              state = STATE_RELE;
              lcd.clear();
              printReleaySettings();
              break;
          }
          break;
        case BUTTON_BACK:
          state = STATE_RUN;
          break;
      }
      setCursor();
      doBatchMinute();
      if (((var_second < lastSecond) && ((lastSecond + var_second) % 60 > seconds_threshold)) ||
            ((var_second >= lastSecond) && ((var_second - lastSecond) > seconds_threshold))) {
          state = STATE_RUN;
        }
      break;
    case STATE_RUN:
      if (lcdOn and state == STATE_RUN) {
        if (((var_second < lastSecond) && ((lastSecond + var_second) % 60 > seconds_threshold)) ||
            ((var_second >= lastSecond) && ((var_second - lastSecond) > seconds_threshold))) {
          Serial.println("lcd turning off...");
          lcdOn = false;
          digitalWrite(pinLcd, LOW);
        }
        if (var_second != second() and state == STATE_RUN) {
          #ifdef RTC
            var_second = Clock.getSecond();
          #else
            var_second = second();
          #endif
          //Serial.println("updating lcd printing...");
          printMainScreen();
        }
      }
      
      //do something
      switch (getButtonPressed()) {
        case NO_BUTTON:
          break;
        case BUTTON_BACK:
          state = STATE_CHANGE_OPTIONS;
          lcdOn = true;
          lcd.clear();
          Serial.println("turning on lcd..");
          digitalWrite(pinLcd, HIGH);
          printChangeQuestion();
          #ifdef RTC
          lastSecond = Clock.getSecond();
          var_second = lastSecond;
          #else
          lastSecond = second();
          var_second = second();
          #endif
          break;
        default:
          printMainScreen();
          lcdOn = true;
          Serial.println("turning on lcd..");
          digitalWrite(pinLcd, HIGH);
          #ifdef RTC
          lastSecond = Clock.getSecond();
          var_second = lastSecond;
          #else
          lastSecond = second();
          var_second = second();
          #endif
          break;
      }
      break;
    break;
  }
  if (is_running){
      doBatchDay();
      doBatchMinute();
  }
}      
