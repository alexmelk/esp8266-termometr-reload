//for temp sensor
#include <OneWire.h>
#include <DallasTemperature.h>
//core by alexmelk
#include <SmartHomeCore.h>
using namespace shCore;

int tempPin = 4;
OneWire oneWire(tempPin);
DallasTemperature sensors(&oneWire);

void getTemperature() {
	sensors.requestTemperatures();
	String temp = String(sensors.getTempCByIndex(0));
	Serial.println(temp);
	sendToServer(200, "text/html", temp);
	blink(1, 100);
}

void setup(void) {
	setSSIDwifiAP("Temperature");
	registrateEvent("/getTemperature",getTemperature);
	
	coreInit();
}

void loop(void) {
	coreHandle();
	delay(1);
}
