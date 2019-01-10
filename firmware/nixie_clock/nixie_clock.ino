/*
  Годинник на газорозрядних індикаторах
  Канал на YouTube: https://goo.gl/x8FL2o
  Відео з проектом: https://youtu.be/rWZkPMapb_8
  Схема: https://easyeda.com/Targaryen/Nixie_Clock
  Github: https://github.com/RomanButsiy/NixieClock
  Автори: Железняков Андрей, AlexGyver, Technologies, Roman
  2019 Roman
*/

// ************************** НАЛАШТУВАННЯ **************************
#define BRIGHT 100          // яскравість цифр у день, %
#define BRIGHT_N 20         // яскравість нічна, %
#define NIGHT_START 23      // година переходу на нічну підсвітку (BRIGHT_N)
#define NIGHT_END 7         // година переходу на денну підсвітку (BRIGHT)

// *********************** ДЛЯ РОЗРОБНИКІВ ***********************
#define BURN_TIME 200       // період обходу в режимі очищення
#define REDRAW_TIME 3000    // час циклу однієї цифри, мс
#define ON_TIME 2200        // час включеності однієї цифри, мс
#define CLOCK_TIME 10       // час (с), який відображається година

#define DECODER0 2
#define DECODER1 3
#define DECODER2 4
#define DECODER3 5

#define KEY0 7    // крапка
#define KEY1 13   // години 
#define KEY2 12    // години
#define KEY3 11    // хвилини
#define KEY4 10    // хвилини
#define KEY5 9    // секунди
#define KEY6 8    // секунди

#include "GyverTimer.h"
GTimer_us redrawTimer(REDRAW_TIME);
GTimer_ms modeTimer((long)CLOCK_TIME * 1000);
GTimer_ms dotTimer(500);
GTimer_ms blinkTimer(800);

#include "GyverButton.h"
GButton btnSet(3, LOW_PULL, NORM_OPEN);
GButton btnUp(3, LOW_PULL, NORM_OPEN);
GButton btnDwn(3, LOW_PULL, NORM_OPEN);

#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;

int opts[] = {KEY0, KEY1, KEY2, KEY3, KEY4, KEY5, KEY6};
byte counter;
byte digitsDraw[7];
boolean dotFlag;
int8_t hrs = 10, mins = 10, secs;
boolean indState;
int8_t mode = 0;    
boolean changeFlag;
boolean blinkFlag;
int on_time = ON_TIME;

void setup() {
  Serial.begin(9600);
  btnSet.setTimeout(400);
  btnSet.setDebounce(90);
  rtc.begin();
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  DateTime now = rtc.now();
  secs = now.second();
  mins = now.minute();
  hrs = now.hour();

  pinMode(DECODER0, OUTPUT);
  pinMode(DECODER1, OUTPUT);
  pinMode(DECODER2, OUTPUT);
  pinMode(DECODER3, OUTPUT);
  pinMode(KEY0, OUTPUT);
  pinMode(KEY1, OUTPUT);
  pinMode(KEY2, OUTPUT);
  pinMode(KEY3, OUTPUT);
  pinMode(KEY4, OUTPUT);
  pinMode(KEY5, OUTPUT);
  pinMode(KEY6, OUTPUT);
  sendTime();
  changeBright();
}

void sendTime() {
  digitsDraw[1] = (byte)hrs / 10;
  digitsDraw[2] = (byte)hrs % 10;

  digitsDraw[3] = (byte)mins / 10;
  digitsDraw[4] = (byte)mins % 10;

  digitsDraw[5] = (byte)secs / 10;
  digitsDraw[6] = (byte)secs % 10;
}

void changeBright() {
  // встановлення яскравості від часу доби
  if ( (hrs >= NIGHT_START && hrs <= 23)
       || (hrs >= 0 && hrs <= NIGHT_END) ) on_time = (float)ON_TIME * BRIGHT_N / 100;
  else on_time = (float)ON_TIME * BRIGHT / 100;
}

void loop() {
  if (redrawTimer.isReady()) showDigits();
  if (dotTimer.isReady() && (mode == 0 || mode == 1)) calculateTime();
  buttonsTick();

}

void buttonsTick() {
  int analog = analogRead(0);

  btnSet.tick(analog <= 1023 && analog > 1000);
  btnUp.tick(analog <= 820 && analog > 690);
  btnDwn.tick(analog <= 280 && analog > 120);

  if (mode == 3) {
    if (btnUp.isClick()) {
        if (!changeFlag) {
          mins++;
          if (mins > 59) {
            mins = 0;
            hrs++;
          }
          if (hrs > 23) hrs = 0;
        } else {
          hrs++;
          if (hrs > 23) hrs = 0;
        }
    }

    if (btnDwn.isClick()) {
        if (!changeFlag) {
          mins--;
          if (mins < 0) {
            mins = 59;
            hrs--;
          }
          if (hrs < 0) hrs = 23;
        } else {
          hrs--;
          if (hrs < 0) hrs = 23;
        }
    }

    if (blinkTimer.isReady()) {
      if (blinkFlag) blinkTimer.setInterval(800);
      else blinkTimer.setInterval(200);
      blinkFlag = !blinkFlag;
    }
      digitsDraw[1] = hrs / 10;
      digitsDraw[2] = hrs % 10;
      digitsDraw[3] = mins / 10;
      digitsDraw[4] = mins % 10;
    

    digitsDraw[5] = 0;  
    digitsDraw[6] = 0;  

    if (blinkFlag) {      
      if (changeFlag) {
        digitsDraw[1] = 10;
        digitsDraw[2] = 10;
      } else {
        digitsDraw[3] = 10;
        digitsDraw[4] = 10;
      }
    }
  }

  if (mode == 0 && btnSet.isHolded()) {
    mode = 3;
  }

  if (mode == 3 && btnSet.isHolded()) {
    sendTime();
    mode = 0;
    secs = 0;
    rtc.adjust(DateTime(2019, 1, 1, hrs, mins, 0));
    changeBright();
    modeTimer.setInterval((long)CLOCK_TIME * 1000);
  }

  if ((mode == 3) && btnSet.isClick()) {
    changeFlag = !changeFlag;
  }

}


void calculateTime() {
  dotFlag = !dotFlag;
  if (dotFlag) {
    secs++;
    if (secs > 59) {
      secs = 0;
      mins++;

      if (mins == 1 || mins == 10 || mins == 20 || mins == 30 || mins == 40 || mins == 50) {     
        burnIndicators();                 
        DateTime now = rtc.now();         // синхронізація з RTC
        secs = now.second();
        mins = now.minute();
        hrs = now.hour();
      }
    }
    if (mins > 59) {
      mins = 0;
      hrs++;
      if (hrs > 23) hrs = 0;
      changeBright();
    }


    if (mode == 0) sendTime();
  }
}

void burnIndicators() {
  for (byte ind = 0; ind < 7; ind++) {
    digitalWrite(opts[ind], 1);
  }
  for (byte dig = 0; dig < 10; dig++) {
    setDigit(dig);
    delayMicroseconds(BURN_TIME);
  }
}

void showDigits() {
  if (indState) {
    indState = false;
    redrawTimer.setInterval(on_time);   // переставляємо таймер, стільки індикатори будуть світити
    counter++;                          // лічильник бігає по індикаторах (0 - 6)
    if (counter > 6) counter = 0;

    if (counter != 0) {                   // якщо це не крапка
      setDigit(digitsDraw[counter]);      // відображаємо ЦИФРУ в її ІНДИКАТОР
      digitalWrite(opts[counter], 1);     // включаємо поточний індикатор
    } else {                              // якщо це крапка
      if (dotFlag)
        if (mode != 1) digitalWrite(opts[counter], 1);   // вмикаємо точку
        else
          digitalWrite(opts[counter], 0);   // вимикаємо точку
    }

  } else {
    indState = true;
    digitalWrite(opts[counter], 0);                 // вимикаємо поточний індикатор
    redrawTimer.setInterval(REDRAW_TIME - on_time); // переставляємо таймер, стільки індикаторів буде вимкнено
  }
}

// налаштовуємо декодер згідно цифри, яка відображається
void setDigit(byte digit) {
  switch (digit) {
    case 0: setDecoder(0, 0, 0, 0);
      break;
    case 1: setDecoder(1, 0, 0, 0);
      break;
    case 2: setDecoder(0, 0, 1, 0);
      break;
    case 3: setDecoder(1, 0, 1, 0);
      break;
    case 4: setDecoder(0, 0, 0, 1);
      break;
    case 5: setDecoder(1, 0, 0, 1);
      break;
    case 6: setDecoder(0, 0, 1, 1);
      break;
    case 7: setDecoder(1, 0, 1, 1);
      break;
    case 8: setDecoder(0, 1, 0, 0);
      break;
    case 9: setDecoder(1, 1, 0, 0);
      break;
    case 10: setDecoder(0, 1, 1, 1);    // вимкнути цифру!
      break;
  }
}

// функція налаштування декодера
void setDecoder(boolean dec0, boolean dec1, boolean dec2, boolean dec3) {
  digitalWrite(DECODER0, dec0);
  digitalWrite(DECODER1, dec1);
  digitalWrite(DECODER2, dec2);
  digitalWrite(DECODER3, dec3);
}
