#include <Ports.h>
#include <RF12.h>

#include <Jeenet.h>

enum { ALIVE, MESURE, TASK_LIMIT };

static word schedBuf[TASK_LIMIT];
Scheduler scheduler (schedBuf, TASK_LIMIT);

ISR(WDT_vect) { Sleepy::watchdogEvent(); } 



Port msgled (2);
Port countled (4);

jeenetMessageWelcome msgWelcome;
jeenetMessageAlive msgAlive;
jeenetMessagePulseCount msgPulseCount;

byte jeenetMessageSize=0;
void* jeenetMessagePointer;

byte needToSend=0;

static byte myNodeID=0;

volatile int pulseCount = 0;
volatile boolean intLoop=false;

// prototypes

void onPulse();
int readVcc();


//ISR(PCINT2_vect) {
//    pulseCount++;
//    
//    countled.digiWrite(HIGH); delay(2); countled.digiWrite(LOW);
//}

void setup () {
    Serial.begin(57600);
    Serial.print("jeenode3");
    
    myNodeID = rf12_config();
    //rf12_initialize(3, RF12_868MHZ, JEENET_GROUP_ID);
    
    msgled.mode(OUTPUT); 
    countled.mode(OUTPUT); 
    //led.mode2(INPUT);
    msgWelcome.msgId=JEENET_MSG_WELCOME;
    msgWelcome.source=myNodeID;
    rf12_sendStart(RF12_HDR_ACK | RF12_HDR_DST | JEENET_MASTER_ID,&msgWelcome, sizeof msgWelcome);
    
    while (!rf12_recvDone()) {} 
    if (rf12_crc == 0 && RF12_ACK_REPLY){
       // ...    
    }
    //rf12_sleep(RF12_SLEEP);
    
    attachInterrupt(1, onPulse, FALLING);
    
    scheduler.timer(ALIVE, 300);
    scheduler.timer(MESURE, 600);
    
    Serial.println("end of setup"); Serial.flush(); delay(2);
    
    //PCMSK1 = bit(1);
    //PCICR |= bit(PCIE1);
}

void loop () {
//     if (rf12_recvDone() && rf12_crc == 0) {
//       
//           if (RF12_ACK_REPLY) {
//             //Serial.println("ok");
//             byte addr = rf12_hdr & RF12_HDR_MASK;
//             //Serial.println(addr);
//           }
//    }
   
   switch (scheduler.pollWaiting()) {
     
     case ALIVE: 
              msgAlive.msgId=JEENET_MSG_ALIVE;
              msgAlive.source=myNodeID;
              msgAlive.batteryVcc=readVcc();
              jeenetMessageSize=sizeof msgAlive;
              jeenetMessagePointer=&msgAlive;
              Serial.println("alive"); Serial.flush(); delay(2);
              needToSend=1;
              scheduler.timer(ALIVE, 300);
      break;
      
      case MESURE:  
              msgPulseCount.msgId=JEENET_MSG_PULSE_COUNT;
              msgPulseCount.source=myNodeID;
              msgPulseCount.pulseCount=pulseCount;
              pulseCount=0;
              jeenetMessageSize=sizeof msgPulseCount; 
              jeenetMessagePointer=&msgPulseCount;
              Serial.println("mesure"); Serial.flush(); delay(2);
              needToSend=1;
              scheduler.timer(MESURE, 600);
       break;     
    }

    
    if (needToSend) {
       rf12_sleep(RF12_WAKEUP);
       int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}        
       needToSend = 0;        
       rf12_sendStart(RF12_HDR_ACK | RF12_HDR_DST | JEENET_MASTER_ID, jeenetMessagePointer, jeenetMessageSize);
       rf12_sendWait(2);
       rf12_sleep(RF12_SLEEP);
       msgled.digiWrite(HIGH); delay(2); msgled.digiWrite(LOW);
    }
      
    //Sleepy::loseSomeTime(5*1000); 
      
    
}

void onPulse()                  
{
  pulseCount++;
  intLoop=true;
  countled.digiWrite(HIGH); delay(2); countled.digiWrite(LOW);
  //digitalWrite(5,HIGH); delay(2); digitalWrite(5,LOW);
}

int readVcc() {
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result;
  return result;
}


