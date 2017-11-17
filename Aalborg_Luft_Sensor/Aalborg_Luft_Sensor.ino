
/*
 * https://github.com/matthijskooijman/arduino-lmic
This code intergrates an OPC-N2 air particle sensor, an SparkFun openlog data logger, 
and a LoRa radio into an Arduino Pro Mini (ATmega328P, 3.3 v, MHz). The Pro Mini will...
 */

// -----------------------------------------------------------------
// Include needed libaries. 
// -----------------------------------------------------------------
     
#include<SoftwareSerial.h>
#include <lmic.h>
#include <hal/hal.h>
#include "opcn2.h"  
#include <SPI.h>


// -----------------------------------------------------------------
// Initialise needed objects. 
// -----------------------------------------------------------------

OPCN2 alpha;
HistogramData hist;
SoftwareSerial logger(4,3);
//APPEUI needs to be in little-endian format, or in other words in least significant bit order
static const u1_t PROGMEM APPEUI[8]={ 0x19, 0x3F, 0x00, 0xF0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// DEVEUI needs to be in little-endian format, or in other words in least significant bit order
static const u1_t PROGMEM DEVEUI[8]={ 0x90, 0x2E, 0x21, 0x44, 0xF3, 0x83, 0x4F, 0x00 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// APPKEY needs to be in big-endian format, or in other words in most significant bit order
static const u1_t PROGMEM APPKEY[16] = { 0xA9, 0xBD, 0x61, 0x10, 0x3A, 0xC3, 0xAA, 0x0C, 0xAA, 0x91, 0xE7, 0x8D, 0xDD, 0x17, 0x66, 0xF7 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}
static uint8_t mydata[] = "Hello, world!";
static osjob_t sendJob;

// -----------------------------------------------------------------
// Define pins and sampleRate. SampleRate(ms) can be changed depending on need.
// -----------------------------------------------------------------

#define resetPin 8
#define particle_CS 9
#define sampleRate 10000

const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 2,
    .dio = {5, 6, 7},
};

boolean hasSend = false;

// -----------------------------------------------------------------
// Handle LoRa events
// -----------------------------------------------------------------


void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));

            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            Serial.println(F("Payload send"));
            hasSend = true;
            
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

// -----------------------------------------------------------------
// send data over LoRa to TTN
// -----------------------------------------------------------------   

void doSend(osjob_t* j, double data[]){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        /*for (int i = 0; i < 19; i = i + 1){
           String sdata = String(data[i]);
           static uint8_t mydata[] = "Hello, world!";
           static uint8_t value[] = {sdata.toInt()};
           LMIC_setTxData2(1, value, sizeof(value)-1, 0);
           Serial.println(F("Packet queued"));
        }*/
        String sdata = String(data[2]);
        static uint8_t mydata[] = "Hello, world!";
        static uint8_t value[] = {sdata.toInt()};
        LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

// -----------------------------------------------------------------
// Setup before run. If the OPC-N2 sensor does not respond, the Pro Mini will reset. 
// -----------------------------------------------------------------   

void setup(){
    
    Serial.begin(9600);
    logger.begin(9600);
    alpha.setup(particle_CS);

    Serial.println("Testing OPC-N2 v" + String(alpha.firm_ver.major) + "." + String(alpha.firm_ver.minor));
    
    // Read and print the configuration variables
   
    if(alpha.toggle_fan(true) == 0){
        pinMode(resetPin, OUTPUT);
        delay(5);
        digitalWrite(resetPin, HIGH);
        }
      
    delay(500);
    alpha.on();
    delay(500);

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();
    LMIC_setLinkCheckMode(1);
    LMIC_setAdrMode(1);
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    delay(500);
}

// -----------------------------------------------------------------
// The main code is now running. Depending on the sample rate, the readings will be written onto the logger.
// Readings seem to be affected if the sample rate is too short, so it is recommended to test a sample rate before using it. 
// There is a minimum natural sample rate of ~ 0.56 - 0.57 sec.  
// -----------------------------------------------------------------   
void loop(){
  
  alpha.beginSPI();

    hist = alpha.read_histogram();

    if ((hist.pm1 + hist.pm25 + hist.pm10) > 0 && hist.period > 0){
      delay(5000);
      // log the histogram data
      Serial.print("\nSampling Period:\t"); Serial.println(hist.period);
      Serial.println("");
      Serial.println("--PM values--");
      Serial.print("PM1: "); Serial.println(hist.pm1);
      Serial.print("PM2.5: "); Serial.println(hist.pm25);
      Serial.print("PM10: "); Serial.println(hist.pm10);
      Serial.println("");
      Serial.println("--bin values--");
      Serial.print("bin1: "); Serial.println(hist.bin0);
      Serial.print("bin2: "); Serial.println(hist.bin1);
      Serial.print("bin3: "); Serial.println(hist.bin2);
      Serial.print("bin4: "); Serial.println(hist.bin3);
      Serial.print("bin5: "); Serial.println(hist.bin4);
      Serial.print("bin6: "); Serial.println(hist.bin5);
      Serial.print("bin7: "); Serial.println(hist.bin6);
      Serial.print("bin8: "); Serial.println(hist.bin7);
      Serial.print("bin9: "); Serial.println(hist.bin8);
      Serial.print("bin10: "); Serial.println(hist.bin9);
      Serial.print("bin11: "); Serial.println(hist.bin10);
      Serial.print("bin12: "); Serial.println(hist.bin11);
      Serial.print("bin13: "); Serial.println(hist.bin12);
      Serial.print("bin14: "); Serial.println(hist.bin13);
      Serial.print("bin15: "); Serial.println(hist.bin14);
      Serial.print("bin16: "); Serial.println(hist.bin15);
      Serial.println("");
      
      /*
      logger.print("\nSampling Period:\t"); logger.println(hist.period);
      logger.println("");
      logger.println("--PM values--");
      logger.print("PM1: "); logger.println(hist.pm1); 
      logger.print("PM2.5: "); logger.println(hist.pm25);
      logger.print("PM10: "); logger.println(hist.pm10);
      logger.println("");
      logger.println("--bin values--");
      logger.print("bin1: "); logger.println(hist.bin0);
      logger.print("bin2: "); logger.println(hist.bin1);
      logger.print("bin3: "); logger.println(hist.bin2);
      logger.print("bin4: "); logger.println(hist.bin3);
      logger.print("bin5: "); logger.println(hist.bin4);
      logger.print("bin6: "); logger.println(hist.bin5);
      logger.print("bin7: "); logger.println(hist.bin6);
      logger.print("bin8: "); logger.println(hist.bin7);
      logger.print("bin9: "); logger.println(hist.bin8);
      logger.print("bin10: "); logger.println(hist.bin9);
      logger.print("bin11: "); logger.println(hist.bin10);
      logger.print("bin12: "); logger.println(hist.bin11);
      logger.print("bin13: "); logger.println(hist.bin12);
      logger.print("bin14: "); logger.println(hist.bin13);
      logger.print("bin15: "); logger.println(hist.bin14);
      logger.print("bin16: "); logger.println(hist.bin15);
      logger.println("");
      */
      delay(300);
      alpha.endSPI();
      delay(300);
      doSend(&sendJob, hist.allData);
      
      long now = millis();
      long future = 30000; //30 sec timeout
      while(hasSend == false){
        os_runloop_once();
        if (millis() > now + future){
          break;
          }
      }
      hasSend = false; 
      delay(1000);
    }
  }
