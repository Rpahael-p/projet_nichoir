#include "M5TimerCAM.h"

#define CAM_EXT_WAKEUP_PIN 13
volatile bool print = false;
bool mode = 0;
bool AP = 0;
unsigned long start = 0;

void IRAM_ATTR onInterrupt() {
	print = true;
}

void change_mode() {
	detachInterrupt(CAM_EXT_WAKEUP_PIN);
	pinMode(CAM_EXT_WAKEUP_PIN, OUTPUT);
	mode = 1;
}


void setup() {
	TimerCAM.begin(true);
	Serial.println("Wake up!!!");

	pinMode(CAM_EXT_WAKEUP_PIN, INPUT);
	attachInterrupt(CAM_EXT_WAKEUP_PIN, onInterrupt, RISING);

	start = millis();
	Serial.println("Appuiyer dans les 10 secondes pour démarrer le mode AP");
}

void loop() {
	if (!AP) {
		if (millis() - start > 10000) {
			if (mode == 0) {
				change_mode();
			}
			delay(250);
			digitalWrite(CAM_EXT_WAKEUP_PIN, HIGH);
			Serial.println("Prend une photo");
			delay(250);
			digitalWrite(CAM_EXT_WAKEUP_PIN, LOW);
		}
		if (print) {
			Serial.println("Passage en mode AP");
			AP = 1;
			change_mode();
			print = false;
		}
	} else {
		Serial.println("Configuré la TimerCam");
		TimerCAM.Power.setLed(128);
		delay(1000);
	}
}