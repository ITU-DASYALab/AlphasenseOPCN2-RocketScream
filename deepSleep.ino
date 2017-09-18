#include <RTCZero.h>
#include <LowPower.h>
#include <SPI.h>
#include <RH_RF95.h>
#define RF95_CS 5
#define RF95_RST 2
RH_RF95 rf95(RF95_CS, RF95_RST); // Rocket Scream Mini Ultra Pro with the RFM95W
#include <SerialFlash.h> // need to add Teensy 3s crc16.h in the SerialFlash/util folder (https://github.com/arduino/ArduinoCore-samd/issues/118)
#define FLASH_CS 4 // digital pin for flash chip CS pin

RTCZero rtc;

unsigned char count = 10;

const byte seconds = 0;
const byte minutes = 00;
const byte hours = 17;

const byte day = 17;
const byte month = 11;
const byte year = 15;

void setup()
{
SerialUSB.println("***** ATSAMD21 Standby Mode Example *****");

// ***** IMPORTANT *****
// Delay is required to allow the USB interface to be active during
// sketch upload process

// Timer is set to activate once every minute

 rtc.begin();

  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  rtc.setAlarmTime(17, 00, 15);
  rtc.enableAlarm(rtc.MATCH_SS);

  rtc.attachInterrupt(alarmMatch);

SerialUSB.println("Entering standby mode in:");
for (count; count > 0; count--)
{
SerialUSB.print(count);
SerialUSB.println(" s");
delay(1000);
}
// *********************

rf95.init();

pinMode(RF95_CS, INPUT_PULLUP); // required since RFM95W is also on the SPI bus
SerialFlash.begin(FLASH_CS);
}

void loop()
{
SerialUSB.println("Entering standby mode.");
SerialUSB.println("Alarm will wake up the device.");
SerialUSB.println("Zzzz…");

// Enter standby mode
sleepNow();
// Wake up
wakeUp();
// Wait for serial USB port to open
while(!SerialUSB);
// Serial USB is blazing fast, you might miss the messages
delay(1000);
SerialUSB.println("Awake!");
SerialUSB.println("Send any character to enter standby mode again");
// Wait for user response
while(!SerialUSB.available());
while(SerialUSB.available() > 0)
{
SerialUSB.read();
}
}

void alarmMatch()
{
  SerialUSB.println("ALARM!");
}

void sleepNow(){
  rf95.sleep();
  SerialFlash.sleep();
  rtc.standbyMode();
  }

void wakeUp(){
  rf95.available();
  SerialFlash.wakeup();
  }  
