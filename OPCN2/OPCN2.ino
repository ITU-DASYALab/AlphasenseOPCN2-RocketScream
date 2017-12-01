#include <opcn2m.h>
#include <SPI.h>
#define CS A2

OPCN2 alpha(CS);
HistogramData hist;
ConfigVars vars;

void setup() {
  pinMode(CS, OUTPUT);
    Serial.begin(9600);

    Serial.println("Testing OPC-N2 v" + String(alpha.firm_ver.major) + "." + String(alpha.firm_ver.minor));

    // Read and print the configuration variables
    vars = alpha.read_configuration_variables();

    Serial.println("\nConfiguration Variables");
    Serial.print("\tGSC:\t"); Serial.println(vars.gsc);
    Serial.print("\tSFR:\t"); Serial.println(vars.sfr);
    Serial.print("\tLaser DAC:\t"); Serial.println(vars.laser_dac);
    Serial.print("\tFan DAC:\t"); Serial.println(vars.fan_dac);
    Serial.print("\tToF-SFR:\t"); Serial.println(vars.tof_sfr);

    // Turn on the OPC
    alpha.on();
    delay(1000);
}

void loop() {
  delay(4000);

    hist = alpha.read_histogram();

    // Print out the histogram data
    Serial.print("\nSampling Period:\t"); Serial.println(hist.period);
    Serial.print("PM1: "); Serial.println(hist.pm1);
    Serial.print("PM2.5: "); Serial.println(hist.pm25);
    Serial.print("PM10: "); Serial.println(hist.pm10);
}
