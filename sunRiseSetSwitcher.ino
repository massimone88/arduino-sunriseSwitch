// sunRiseSetSwitcher.ino
#include <LiquidCrystal.h>
#include <Time.h>
#include <sunriseset.h>
#include <EEPROM.h>

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
//pins
int pinButtons = A0;
int pinLcd = A1; //to connect to pin 15 of lcd after 220 ohm resistor
int pinRele1 = 10;
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
const int STATE_CHANGE = 9;
const int STATE_CHANGE_OPTIONS = 10;
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
int saveSettings = false;
int lcdOn = false;
int lastSecond = 0;
int stateRele = 2;
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
    if (keyVal == 1023) {
      buttonPressed = true;
      Serial.println("button left pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_LEFT;
    }
    else if (keyVal >= 980 && keyVal <= 1010) {
      Serial.println("button back pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      buttonPressed = true;
      return BUTTON_BACK;
    }
    else if (keyVal >= 950 && keyVal <= 980) {
      buttonPressed = true;
      Serial.println("button up pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_UP;
    }
    else if (keyVal >= 650 && keyVal <= 720) {
      buttonPressed = true;
      Serial.println("button down pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_DOWN;
    }
    else if (keyVal >= 500 && keyVal <= 600) {
      buttonPressed = true;
      Serial.println("button right pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_RIGHT;
    }
    else if (keyVal >= 5 && keyVal <= 15) {
      buttonPressed = true;
      Serial.println("button OK pressed!");
      String valueStr = "value: " + String(keyVal);
      Serial.println(valueStr);
      return BUTTON_OK;
    }
    
  }
  if (keyVal < 5) {
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
  String line1 =  int2string(hour(), 2) + ":" + int2string(minute(), 2) + ":" +  int2string(second(), 2) + "   ";
  line1 += int2string(day(), 2) + "/" + int2string(month(), 2);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  String line2 =  "A:" + int2string(sunrise_hour, 2) + ":" + int2string(sunrise_minute, 2) + "  ";
  line2 += "T:" + int2string(sunset_hour, 2) + ":" + int2string(sunset_minute, 2);
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
  return ((sunrise_hour < hour() && hour() < sunset_hour) ||
          (sunrise_hour == hour() && minute() >= sunrise_minute) ||
          (sunset_hour == hour() && minute() <= sunset_minute));
}

void turnOffReleays(){
  digitalWrite(pinRele1, LOW);
  digitalWrite(pinRele2, LOW);
  Serial.println("light OFF!");
}

void turnOnReleays(){
  if (stateRele == BOTH_RELE || stateRele == RELE1){
      Serial.println("light ON Rele1!");
      digitalWrite(pinRele1, HIGH);
  }
  if (stateRele == BOTH_RELE || stateRele == RELE2){
    Serial.println("light ON Rele2!");
    digitalWrite(pinRele2, HIGH);
  }
}

void setup() {
  var_day = var_month = 6;
  var_hour = 19;
  var_minute = 56;
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
  EEPROM.get( 0, settings);
  Serial.println( "Read custom object from EEPROM: " );
  Serial.println( settings.time );
  Serial.println( settings.stateRele );
  stateRele = settings.stateRele;
  if (stateRele != -1){
    setTime(settings.time);
    var_day = day();
    var_month = month();
    var_year = year();
    var_hour = hour();
    var_minute = minute();
    var_second = second();
    state= STATE_CALC;
  }
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
        }
        break;
      case STATE_MINUTE:
        //do something
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
      //do something
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
            state = STATE_MINUTE;
            lcd.clear();
            Serial.println("return to hour setting...");
            break;
        }
        setCursor();
        break;
    case STATE_SAVE:
      //do something
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
            setTime(var_hour, var_minute, 0, var_day, var_month, var_year);
            Serial.println("time set!");
            Serial.println("today is  : " + int2string(var_day, 2) + "/" + int2string(var_month, 2) + "/" + int2string(var_year, 2) + " at " + int2string(var_hour, 2) + ":" + int2string(var_minute, 2));
            Serial.println("Rele option: " + int2string(stateRele,1));
            settings = {
              now(),
              stateRele
            };
            Serial.println("Saving the timestamp to EEPROM..!");
            EEPROM.put( 0, settings);
            Serial.println("Saved to address 0");
            state = STATE_CALC;
            Serial.println("go to calculating state...");
          }
          else {
            state =  STATE_DAY;
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
      lastSecond = second();
      var_second = second();
      lcdOn = true;
      lcd.clear();
      printMainScreen();
      break;
    case STATE_CHANGE_OPTIONS:
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
          var_second = second();
          //Serial.println("updating lcd printing...");
          printMainScreen();
        }
      }
      if (var_day != day()) {
        var_day = day();
        var_month = month();
        var_year = year();
        var_hour = hour();
        var_minute = minute();
        state = STATE_CALC;
        break;
      }

      if ( var_minute != minute()) {
        Serial.println("check if there is sun light or not...");
        var_minute = minute();
        //check sunrisesunset
        if (isThereDayLight()) {
          turnOffReleays();
        }
        else {
          turnOnReleays();
        }
        Serial.println("Update the EEPROM..");
        SettingsObject settings2 = {
              now(),
              stateRele
            };
        EEPROM.put(0, settings2);
        EEPROM.put(0, settings2);
        Serial.println("Saved!");
        Serial.println("Now the value on EEPROM address is..");
        SettingsObject settings3;
        EEPROM.get( 0, settings3);
        Serial.println( settings3.time );
        Serial.println( settings3.stateRele );
      }
      //do something
      switch (getButtonPressed()) {
        case NO_BUTTON:
          break;
        default:
          printMainScreen();
          lcdOn = true;
          Serial.println("turning on lcd..");
          digitalWrite(pinLcd, HIGH);
          lastSecond = second();
          var_second = second();
          break;
      }
      break;
  }
}
