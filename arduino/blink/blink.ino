#include <Ports.h>

Port hfled (3);

void setup() { 
	hfled.mode(OUTPUT);
	hfled.mode2(OUTPUT);
	Serial.begin(57600);
} 
void loop() { 
	
	hfled.digiWrite2(HIGH); delay(2); hfled.digiWrite2(LOW);
	delay(1000); 
	Serial.println("hello world");
}
