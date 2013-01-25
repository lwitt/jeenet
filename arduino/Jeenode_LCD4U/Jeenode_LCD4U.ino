//#define DEBUG 1

#include <Ports.h>
#include <RF12.h>
#include <avr/wdt.h>

#include <Jeenet.h>

#include <Wire.h> 
#include <RTClib.h>

#include <GLCD_ST7565.h>
#include "utility/font_clR4x6.h"
#include "utility/font_helvB12.h"
#include "utility/font_helvB24.h"
#include "utility/font_helvR08.h"

#define ACK_TIME        10000

DHTxx dht (5);

class RTC_Plug : public DeviceI2C {
    // shorthand
    static uint8_t bcd2bin (uint8_t val) { return RTC_DS1307::bcd2bin(val); }
    static uint8_t bin2bcd (uint8_t val) { return RTC_DS1307::bin2bcd(val); }
public:
    RTC_Plug (const PortI2C& port) : DeviceI2C (port, 0x68) {}

    void begin() {}
    
    void adjust(const DateTime& dt) {
        send();
        write(0);
        write(bin2bcd(dt.second()));
        write(bin2bcd(dt.minute()));
        write(bin2bcd(dt.hour()));
        write(bin2bcd(0));
        write(bin2bcd(dt.day()));
        write(bin2bcd(dt.month()));
        write(bin2bcd(dt.year() - 2000));
        write(0);
        stop();
    }

    DateTime now() {
      	send();
      	write(0);	
        stop();

        receive();
        uint8_t ss = bcd2bin(read(0));
        uint8_t mm = bcd2bin(read(0));
        uint8_t hh = bcd2bin(read(0));
        read(0);
        uint8_t d = bcd2bin(read(0));
        uint8_t m = bcd2bin(read(0));
        uint16_t y = bcd2bin(read(1)) + 2000;
    
        return DateTime (y, m, d, hh, mm, ss);
    }
};

PortI2C i2cBus (3);

RTC_Plug RTC (i2cBus);


GLCD_ST7565 glcd;

volatile int backlightTempo=1;

//ISR(WDT_vect) { Sleepy::watchdogEvent(); } 
ISR(PCINT1_vect) {backlightTempo=1; glcd.backLight(255);}

jeenetMessageWelcome msgWelcome;
jeenetMessageAlive msgAlive;
//jeenetMessageTempHumi msgTempHumi;
jeenetUMessage msgU;
jeenetMessageNTP msgNtp;
jeenetMessageWeather msgWeather;

byte jeenetMessageSize=0;
void* jeenetMessagePointer;

byte needToSend=0;

MilliTimer mesureTimer,aliveTimer;
char textBuffer[32];
long ntpLastUpdate=0;

static byte myNodeID=0;

static byte msgUsequence=0;
static byte msgUsize=0;

int t,h,tmin,tmax;

Port port(2);

// prototypes

static byte waitForAck();
void mesureTempHumi();
void drawScreen();
int readVcc();
byte encodeUMsg(byte,void*,byte);


void setup () {
    wdt_enable(WDTO_8S);
    Serial.begin(57600);
    Serial.print("jeenode_lcd");
    
    port.mode2(INPUT);
    port.digiWrite2(1);
    
    PCMSK1 = bit(1);
    PCICR |= bit(PCIE1);
    
    Wire.begin();
    RTC.begin();
    
    myNodeID = rf12_config();
    
    glcd.begin();
    glcd.backLight(255);
    //glcd.fillRect(0,0,128,64,BLACK);
    glcd.refresh();
     
    msgWelcome.msgId=JEENET_MSG_WELCOME;
    msgWelcome.source=myNodeID;
    rf12_sendStart(RF12_HDR_ACK | RF12_HDR_DST | JEENET_MASTER_ID,&msgWelcome, sizeof msgWelcome);
    
    waitForAck();

    mesureTempHumi();
    drawScreen();
    
  //  rf12_sleep(RF12_SLEEP);
}

void loop () {
     if (rf12_recvDone() && rf12_crc ==0) {
     if (rf12_len>=JEENET_MSG_HEADER ) {
        
            byte message=rf12_data[0],source=rf12_data[1];
            #ifdef DEBUG
            Serial.println(message);
            #endif
            switch (message) {   
          
              case JEENET_MSG_NTP:
                if (rf12_len==sizeof (jeenetMessageNTP)) {
                  memcpy(&msgNtp,(void*)rf12_data,sizeof(jeenetMessageNTP));
                  
                  #ifdef DEBUG
                  //DateTime now = RTC.now();
                  //sprintf(textBuffer,"[%.2d:%.2d:%.2d] ",now.hour(),now.minute(),now.second());
                  //Serial.print(textBuffer);
                  Serial.print("JEENET_MSG_NTP\t");
                  sprintf(textBuffer,"node %d : %.2d:%.2d:%.2d %.2d/%.2d/%.4d",source,msgNtp.hour,msgNtp.minute,msgNtp.second,msgNtp.day,msgNtp.month,msgNtp.year);
                  Serial.println(textBuffer);
                  #endif
                  
                  ntpLastUpdate=millis();
                  DateTime t(msgNtp.year,msgNtp.month,msgNtp.day,msgNtp.hour,msgNtp.minute,msgNtp.second);
                  RTC.adjust(t);
            }
            break;
            
            case JEENET_MSG_WEATHER:
                if (rf12_len==sizeof (jeenetMessageWeather)) {
                  memcpy(&msgWeather,(void*)rf12_data,sizeof(jeenetMessageWeather));
                  
                  #ifdef DEBUG
                  //DateTime now = RTC.now();
                  //sprintf(textBuffer,"[%.2d:%.2d:%.2d] ",now.hour(),now.minute(),now.second());
                  //Serial.print(textBuffer);
                  Serial.print("JEENET_MSG_WEATHER\t");
                  sprintf(textBuffer,"node %d : %s %d",source,msgWeather.weather,msgWeather.temp);
                  Serial.println(textBuffer);
                  sprintf(textBuffer,"%s %d %d",msgWeather.day1,msgWeather.low1,msgWeather.high1);
                  Serial.println(textBuffer);
                  sprintf(textBuffer,"%s %d %d",msgWeather.day2,msgWeather.low2,msgWeather.high2);
                  Serial.println(textBuffer);
                  #endif
            }
            break;
        }
     }
     }
   
   if (aliveTimer.poll(1000*45) ) {
              msgAlive.msgId=JEENET_MSG_ALIVE;
              msgAlive.source=myNodeID;
              msgAlive.batteryVcc=readVcc();
              jeenetMessageSize=sizeof msgAlive;
              jeenetMessagePointer=&msgAlive;
              needToSend=1;    
              if (backlightTempo>0) backlightTempo--;
              if (backlightTempo==0) glcd.backLight(0);  
            #ifdef DEBUG 
             Serial.println("JEENET_MSG_ALIVE"); 
             #endif
      }
   
    if (mesureTimer.poll(1000*60) && !needToSend){    
        
        mesureTempHumi();
        drawScreen();
   
        jeenetUTempHumi UTH;
        
        UTH.temperature=(byte)t;
        UTH.humidity=(byte)h;
        
        encodeUMsg(JEENET_MSG_TEMP_HUMI,&UTH,sizeof UTH);
        encodeUMsg(JEENET_MSG_TEMP_HUMI,&UTH,sizeof UTH);
        
//        byte debug[16];
//        memcpy(&debug,&msgU,16);
//        Serial.print("[");
//        for (int i=0;i<msgUsize;i++){
//          Serial.print(debug[i]);
//          Serial.print(" ");  
//      }
//        Serial.println("]");
        
        jeenetMessageSize=msgUsize;
        jeenetMessagePointer=&msgU;
        needToSend=1;   
    }
    
    if (needToSend) {
       //rf12_sleep(RF12_WAKEUP);
       int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}        
       needToSend = 0;
       msgUsize=0;
       rf12_sendStart(RF12_HDR_ACK | RF12_HDR_DST | JEENET_MASTER_ID, jeenetMessagePointer, jeenetMessageSize);
       //rf12_sendWait(2);
       //rf12_sleep(RF12_SLEEP);
    }
   
    //Sleepy::loseSomeTime(5*1000); 
    wdt_reset(); 
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

void mesureTempHumi(){
        dht.reading(t,h);
        t=t/10; h=h/10;
        if (tmin==0) tmin=t;
        if (tmax==0) tmax=t;
        if (t<tmin) tmin=t;
        if (t>tmax) tmax=t;
}

void drawScreen() {
  glcd.fillRect(0,0,128,64,BLACK);
  
  glcd.setFont(font_clR4x6);
  sprintf(textBuffer,"%d",myNodeID);
  glcd.drawString((128-((myNodeID/10)+1)*5),59, textBuffer);
  if (ntpLastUpdate!=0 && ((millis()-ntpLastUpdate)/1000)<1800)
    glcd.drawString(113,52,"NTP");
    
  DateTime now = RTC.now();
    
  sprintf(textBuffer,"%.2d:%.2d",now.hour(),now.minute());
  glcd.setFont(font_helvB24);
  glcd.drawString(0,5,textBuffer);
  
  sprintf(textBuffer,"%.2d/%.2d/%.4d",now.day(),now.month(),now.year());
  glcd.setFont(font_helvR08);
  glcd.drawString(12,32, textBuffer);
  
  glcd.drawLine(0,49,128,49,WHITE); 
  glcd.drawLine(80,0,80,49,WHITE); 
  
  if (isnan(t) || isnan(h)) 
    sprintf(textBuffer,"--C        -- %");
  else 
     sprintf(textBuffer,"%.2d.0C        %.2d%%",t,h);
         
  glcd.setFont(font_helvB12);
  glcd.drawString(0,51, textBuffer);
  glcd.setFont(font_clR4x6);
  sprintf(textBuffer,"MIN %.2dC",tmin);
  glcd.drawString(44,52,textBuffer);
  sprintf(textBuffer,"MAX %.2dC",tmax);
  glcd.drawString(44,59,textBuffer);
  
  if (strcmp(msgWeather.weather,"")!=0) {
    glcd.setFont(font_helvB12);
    sprintf(textBuffer,"%.2d.0C",msgWeather.temp);
    glcd.drawString(82,0,textBuffer);
    glcd.setFont(font_clR4x6);
    //glcd.drawString(82+(12-strlen(netTown))/2*4,14,netTown);
    glcd.drawString(82+(12-strlen(msgWeather.weather))/2*4,15,msgWeather.weather);
    sprintf(textBuffer,"%s %+.2d.0C",msgWeather.day1,msgWeather.low1);
    glcd.drawString(82,23,textBuffer);
    sprintf(textBuffer,"%+.2d.0C",msgWeather.high1);
    glcd.drawString(98,29,textBuffer);
    sprintf(textBuffer,"%s %+.2d.0C",msgWeather.day2,msgWeather.low2);
    glcd.drawString(82,36,textBuffer);
    sprintf(textBuffer,"%+.2d.0C",msgWeather.high2);
    glcd.drawString(98,42,textBuffer);
  }
  else {
    glcd.setFont(font_helvB12);
    glcd.drawString(95,18,"n/a");
  }
    
  glcd.refresh();
}

static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

static byte waitForAck() {
    MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
        if (rf12_recvDone() && rf12_crc == 0 &&
         rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
            return 1;
        //set_sleep_mode(SLEEP_MODE_IDLE);
        //sleep_mode();
    }
    return 0;
}

byte encodeUMsg(byte type,void* part,byte size){
  if (msgUsize==0) {
          msgU.msgId=JEENET_MSG_UNIVERSAL; // compatibility with old sketches!
          msgU.source=myNodeID;
          msgU.sequence=msgUsequence++;
          msgUsize=3; // compatibility with old sketches!
  }
  msgU.payload[msgUsize-3]=type;
  memcpy(&msgU.payload[msgUsize-3+1],part,size);
  msgUsize+=size+1;
  return msgUsize;
}
