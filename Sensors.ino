#include <Arduino.h>
#include <LiquidCrystal/src/LiquidCrystal.h>
#include "avr/sleep.h"

/**
 * using LCD library
 * Columns: 16 (0 - 15)
 * Rows: 2 (0 - 1)
 * GND/1
 * Vdd/2
 * V0/3 -> POTI || GND
 * RS/4 -> 13
 * R/W/5 -> GND
 * E/6 -> 12
 * DB4/11 -> 11
 * DB5/12 -> 10
 * DB6/13 -> 9
 * DB7/14 -> 8
 * A/15 -> R220 -> Vdd
 * K/16 -> GND
 */

#define HORN A0                    //horn output (used as digital output)
#define TEMPSENSOR A1              //temperatur sensor
#define FUELSENSOR A2              //fuel sender / sensor
#define BREAKFLUID_WARNLIGHT A3    //break fluid warning light (used as digital output)
#define BTN_FAN 2                  //fan enable button
#define ENGINE_ON 3                //power on key switched
#define FAN 4                      //fan on/off control
#define BTN_BREAKFLUID_WARNLIGHT 5 //button to enable break fluid warn light
#define BTN_HORN 6                 //button to enable horn
#define FUEL_WARN_LAMP 7           //fuel warn lamp
#define FUEL_WARN_THRESHOLD 5      //number of display digits before warning lamp is turned on

#define MODULO 5
#define SLEEP_TIME 10              //delay before Atmega goes to sleep in minutes
#define DELAY 100

LiquidCrystal lcd(13, 12, 11, 10, 9, 8); //define the LCD Pins
long fuelLevel = 0;
long time = 0;
long powerOffTime = 0;
int enableSleep = 0;
int count = 0;

//fuel symbol
byte fuelSymbol[8] = {
	0b11100,
	0b10110,
	0b10111,
	0b10101,
	0b11101,
	0b11101,
	0b11111,
	0b11100
};

byte mediumChar[8] = {
        0b10101,
        0b01010,
        0b10101,
        0b01010,
        0b10101,
        0b01010,
        0b10101,
        0b01010
};

/**
 * wakeup routine
 */
void wakeUp() {
    enableSleep = 0;
}

/**
 * send atmega to power off mode
 */
void gotoSleep() {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(0, wakeUp, LOW); //wakeup on button fan turned on
    attachInterrupt(1, wakeUp, HIGH); //wakeup on power turned on

    sleep_mode();
    sleep_disable();
    detachInterrupt(0);
    detachInterrupt(1);
}

/**
 * write the temp bargraph
 */
void handleTempReading(int sensorValue) {
    lcd.setCursor(0, 1); //second row
    lcd.print("C:");
}

/**
 * write the Fuel bargraph
 */
void handleFuelReading(int sensorValue) {

    //map the value to the lcd fields
    fuelLevel = map(sensorValue, 130, 650, 15, 0);

    //reset lcd row
    lcd.setCursor(1, 0);
    lcd.print("               ");

    //write the first 5 fields with 'mediumChar'
    for (unsigned int i = 1; i<= fuelLevel; i++) {
        lcd.setCursor(i, 0);
        lcd.write(byte(1));
        if (i>= 5)
            break;
    }

    //write the 
    for (unsigned int i=1; i<=fuelLevel - 5; i++) {
        lcd.setCursor(i + 5, 0);
        lcd.print('\377');
    }

    //enable the fuel warn lamp
    if (fuelLevel <= FUEL_WARN_THRESHOLD) {
        //start blinking
        digitalWrite(FUEL_WARN_LAMP, HIGH);        
        if (fuelLevel <= 2 && count % MODULO == 0) {
            digitalWrite(FUEL_WARN_LAMP, LOW);
        }
    } else {
        digitalWrite(FUEL_WARN_LAMP, LOW);
    }
}

/**
 * handle break fluid warning lamp
 */
void handleBreakFluidWarnlamp(int state) {
    if (state == LOW) {
        digitalWrite(BREAKFLUID_WARNLIGHT, HIGH);
    } else {
        digitalWrite(BREAKFLUID_WARNLIGHT, LOW);
    }
}

/**
 * handle fan
 */
void handleFan(int state) {
    if (state == LOW) {
        digitalWrite(FAN, HIGH);
    } else {
        digitalWrite(FAN, LOW);
    }
}

/**
 * handle Horn
 */
void handleHorn(int state) {
    if (state == LOW) {
        digitalWrite(HORN, HIGH);
    } else {
        digitalWrite(HORN, LOW);
    }
}

void setup() {
    //Serial.begin(9600);
    lcd.begin(16, 2); //setup Display 16 columns, 2 rows
    lcd.createChar(0, fuelSymbol); //init fuel char
    lcd.createChar(1, mediumChar); //init mediumChar

    //Init first row and write Fuel Symbol
    lcd.setCursor(0, 0); //first row
    lcd.write(byte(0)); //write fuel symbol

    pinMode(BTN_FAN, INPUT); //btn fan
    digitalWrite(BTN_FAN, HIGH); //enable internal pullup
    pinMode(ENGINE_ON, INPUT); //setup power wakeup pin = interrupt enabeld
    pinMode(FAN, OUTPUT);
    pinMode(BTN_BREAKFLUID_WARNLIGHT, INPUT);
    digitalWrite(BTN_BREAKFLUID_WARNLIGHT, HIGH); //enable internal pullup
    pinMode(BTN_HORN, INPUT);
    digitalWrite(BTN_HORN, HIGH); //enable internal pullup
    pinMode(FUEL_WARN_LAMP, OUTPUT);
    pinMode(HORN, OUTPUT);
    pinMode(BREAKFLUID_WARNLIGHT, OUTPUT);

    attachInterrupt(0, wakeUp, LOW); //wake up on 0 == PIN 2 == fan button
    attachInterrupt(1, wakeUp, HIGH); //wake up on 1 == PIN 2 == power switch
}

void loop() {
    time = millis();

    //check power mode and sleep mode to enable sleep
    int readPowerOn = digitalRead(ENGINE_ON);
    if (readPowerOn == LOW && enableSleep == 1) {
        if (time > (powerOffTime + (SLEEP_TIME * 60 * 1000))) {
            gotoSleep();
        }
    }

    count++;

    //start reading
    int fuelSensorValue = analogRead(FUELSENSOR);
    int tempSensorValue = analogRead(TEMPSENSOR);

    int readBtnFan = digitalRead(BTN_FAN);
    int readBtnHorn = digitalRead(BTN_HORN);
    int readBtnBreak = digitalRead(BTN_BREAKFLUID_WARNLIGHT);

    handleTempReading(tempSensorValue);
    handleFuelReading(fuelSensorValue);
    //handle button actions
    handleBreakFluidWarnlamp(readBtnBreak);
    handleFan(readBtnFan);
    handleHorn(readBtnHorn);

    //handle power off mode
    if (readPowerOn == LOW && enableSleep == 0 && readBtnFan == HIGH) {
        powerOffTime = time;
        enableSleep = 1;
    }

    delay(DELAY);
}

