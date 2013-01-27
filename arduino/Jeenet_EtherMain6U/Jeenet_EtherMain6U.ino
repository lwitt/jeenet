
//#define DEBUG 0
#define DEBUG2 0

#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <RF12.h>
#include <Ports.h>
#include <Jeenet.h>
#include <EtherCard.h>

// ****************************************************************
// prototypes
// ****************************************************************
byte decodeUMSG(jeenetUMessage*,byte*);
static int freeRam (); 


Port hfled (3);
Port netled (4);
DHTxx dht (7);

static uint8_t mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x25};
static uint8_t myip[4] = { 192,168,1,126 };
static uint8_t mynetmask[4] = { 255,255,255,0 };
static uint8_t gwip[4] = { 192,168,1,1 };

#define BUFFER_SIZE 700
byte Ethernet::buffer[BUFFER_SIZE];

jeenetMessageTemp2 msgTemp2;
jeenetMessageTempHumi msgTempHumi;
jeenetMessagePulseCount msgPulseCount;
jeenetMessageNTP msgNtp;
jeenetMessageWeather msgWeather;
//jeenetMessageRelayState msgRelayState[26];
jeenetUMessage UMSG;

byte jeenetMessageSize=0;
void* jeenetMessagePointer;

bool needToSend=false;
char textBuffer[64];
static int clock_iterations=0;

const char website[] PROGMEM = "PC6.home";

MilliTimer clockTimer,aliveTimer;


// ****************************************************************
// Ethernet callback
// ****************************************************************

static void my_callback (byte status, word off, word len) {
	char * start;
	
	Ethernet::buffer[off+BUFFER_SIZE] = 0;  

	netled.digiWrite2(HIGH); delay(2); netled.digiWrite2(LOW);

	if ( start = strstr((const char*)Ethernet::buffer+off,"TIME")) {
		strncpy(textBuffer,start+5,19);  
		textBuffer[19]=0;
		msgNtp.hour=atoi(strtok(textBuffer,":"));
		msgNtp.minute=atoi(strtok(NULL,":"));
		msgNtp.second=atoi(strtok(NULL,":"));
		msgNtp.day=atoi(strtok(NULL,":"));
		msgNtp.month=atoi(strtok(NULL,":"));
		msgNtp.year=atoi(strtok(NULL,":"))+2000;

		msgNtp.msgId=JEENET_MSG_NTP;
		msgNtp.source=JEENET_MASTER_ID;
		jeenetMessageSize=sizeof msgNtp; 
		jeenetMessagePointer=&msgNtp;
		needToSend=true;            
	}

	if ( start = strstr((const char*)Ethernet::buffer+off,"MET:")) {
		strncpy(textBuffer,start+4,64);  
		textBuffer[64]=0;
		strcpy(msgWeather.weather,strtok(textBuffer,":"));
		msgWeather.temp=atoi(strtok(NULL,":"));

		strcpy(msgWeather.day1,strtok(NULL,":"));
		msgWeather.low1=atoi(strtok(NULL,":"));
		msgWeather.high1=atoi(strtok(NULL,":"));
		//strcpy(msgWeather.weather1,strtok(NULL,":"));

		strcpy(msgWeather.day2,strtok(NULL,":"));
		msgWeather.low2=atoi(strtok(NULL,":"));
		msgWeather.high2=atoi(strtok(NULL,":"));
		//strcpy(msgWeather.weather2,strtok(NULL,":"));

		#ifdef DEBUG
		sprintf(textBuffer,"%s %d",msgWeather.weather,msgWeather.temp);
		Serial.println(textBuffer);
		sprintf(textBuffer,"%s %d %d",msgWeather.day1,msgWeather.low1,msgWeather.high1);
		Serial.println(textBuffer);
		sprintf(textBuffer,"%s %d %d",msgWeather.day2,msgWeather.low2,msgWeather.high2);
		Serial.println(textBuffer);
		#endif

		msgWeather.msgId=JEENET_MSG_WEATHER;
		msgWeather.source=JEENET_MASTER_ID;
		jeenetMessageSize=sizeof msgWeather; 
		jeenetMessagePointer=&msgWeather;

		needToSend=true;            
	}

	if (needToSend) {
		needToSend=false;

		int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}        

		// broadcast (!!) ntp+weather  
		rf12_sendStart(0, jeenetMessagePointer, jeenetMessageSize);
		hfled.digiWrite2(HIGH); delay(2); hfled.digiWrite2(LOW);
	}
	start = strstr((const char*)Ethernet::buffer+off,"Server");
	Serial.println(start);

}


void setup(){
	Serial.begin(57600);
	Serial.println( F("Jeenet EtherMain" ) );

	hfled.mode(OUTPUT);
	hfled.mode2(OUTPUT); 
	netled.mode2(OUTPUT);

	for (int i=0;i<26;i++)
		jeenet_nodes_watchdog[i]=0;

	rf12_initialize(JEENET_MASTER_ID, RF12_868MHZ, JEENET_GROUP_ID);

	uint8_t rev = ether.begin(sizeof Ethernet::buffer, mymac);
	Serial.print( F("\nENC28J60 Revision ") );
	Serial.println( rev, DEC );

	if ( rev == 0) {
		Serial.println( F( "Failed to access Ethernet controller" ) );
		for (;;);
	}
	else {
		ether.staticSetup(myip, gwip,gwip);
		ether.printIp("IP: ", ether.myip);

		if (!ether.dnsLookup(website))
		Serial.println("DNS failed");
	  
		//Serial.println(freeRam());
		  
		rf12_sleep(RF12_SLEEP);

		ether.browseUrl(PSTR("/rest/jeenet.php"), "?f=init", website, my_callback);
	}
	
	wdt_enable(WDTO_8S);  
}

void loop(){
	ether.packetLoop(ether.packetReceive());

	if (rf12_recvDone()  && rf12_crc == 0 && ((rf12_hdr&RF12_HDR_MASK) == JEENET_MASTER_ID)) {          
		if (rf12_len>=JEENET_MSG_HEADER ) {

		hfled.digiWrite(HIGH); delay(2); hfled.digiWrite(LOW);
		byte message=rf12_data[0],source=rf12_data[1];
		int nodeIndex=searchNode(source);
		
		switch (message) { 

			case JEENET_MSG_UNIVERSAL:
				if (rf12_len>2) {
					memcpy(&UMSG,(void*)rf12_data,rf12_len);
				  
					if (nodeIndex<0) 
						addNode(source);
					else 
						updateWatchdogIndex(nodeIndex);
				
					sprintf(textBuffer,"?f=addnode&v=%d",source);
					//ether.browseUrl(PSTR("/rest/jeenet.php"), textBuffer, website, my_callback);
					
					byte UMSGtype;
					byte index;
					index=decodeUMSG(&UMSG,&UMSGtype);
				 
					switch (UMSGtype) {
						case JEENET_MSG_TEMP_HUMI:
							setAbility(source,JEENET_ABILITY_TEMP_HUMI);
							jeenetUTempHumi uth;
							memcpy(&uth,&UMSG.payload[index],sizeof uth);
							
							#ifdef DEBUG2
							Serial.print("[");
							Serial.print(textBuffer);
							Serial.println("]");
							Serial.println("UMSG_TEMP_HUMI");
							sprintf(textBuffer,"t=%02dC h=%02d%%",uth.temperature,uth.humidity);
							Serial.println(textBuffer);
							#endif
							
							sprintf(textBuffer,"?f=store&n=%d&t=h&v=%d&v2=%d",source,uth.temperature,uth.humidity);
							ether.browseUrl(PSTR("/rest/jeenet.php"), textBuffer, website, my_callback);
							
						break;
					}
					
					#ifdef DEBUG2
					//Serial.println("UNIVERSAL");
					//sprintf(textBuffer,"source=%d",UMSG.source);
					//Serial.println(textBuffer);
					//sprintf(textBuffer,"seq=%d",UMSG.sequence);
					//Serial.println(textBuffer);
					#endif
				}
			break;
  
			case JEENET_MSG_WELCOME:
				if (rf12_len==sizeof (jeenetMessageWelcome)) {
					#ifdef DEBUG
					//DateTime now = RTC.now();
					//sprintf(textBuffer,"[%.2d:%.2d:%.2d] ",now.hour(),now.minute(),now.second());
					//Serial.print(textBuffer);
					
					sprintf(textBuffer,"JEENET_MSG_WELCOME\tnode %d",source);
					Serial.println(textBuffer);
					#endif
				  
					if (nodeIndex==-1){
						addNode(source);
						sprintf(textBuffer,"?f=addnode&v=%d",source);
						ether.browseUrl(PSTR("/rest/jeenet.php"), textBuffer, website, my_callback);
						//nodesChanged=true;
					}
				}
			break;
//            
//            case JEENET_MSG_TEMP_2:
//            if (rf12_len==sizeof (jeenetMessageTemp2) && nodeIndex>=0) {
//                memcpy(&msgTemp2[nodeIndex],(void*)rf12_data,sizeof(jeenetMessageTemp2));
//                updateWatchdog(source);
//                setAbility(source,JEENET_ABILITY_TEMP_2);
//                
//                #ifdef DEBUG
////                DateTime now = RTC.now();
////                sprintf(textBuffer,"[%.2d:%.2d:%.2d] ",now.hour(),now.minute(),now.second());
////                Serial.print(textBuffer);          
//                Serial.print("JEENET_MSG_TEMP_2\t");          
//                sprintf(textBuffer,"node %d : t=%02d.%02d C     ",source,msgTemp2[nodeIndex].temp[0],msgTemp2[nodeIndex].temp[1]);
//                Serial.println(textBuffer);
//                #endif
//            }
//            break;
//            
			case JEENET_MSG_TEMP_HUMI:
				if (rf12_len==sizeof (jeenetMessageTempHumi) && nodeIndex>=0) {
					memcpy(&msgTempHumi,(void*)rf12_data,sizeof(jeenetMessageTempHumi));
					updateWatchdog(source);
					setAbility(source,JEENET_ABILITY_TEMP_HUMI);

					#ifdef DEBUG
					//               DateTime now = RTC.now();
					//                sprintf(textBuffer,"[%.2d:%.2d:%.2d] ",now.hour(),now.minute(),now.second());
					//                Serial.print(textBuffer);
					Serial.print("JEENET_MSG_TEMP_HUMI\t");
					sprintf(textBuffer,"node %d : t=%02dC h=%02d%%    ",source,msgTempHumi.temperature,msgTempHumi.humidity);
					Serial.println(textBuffer);
					#endif
					
					sprintf(textBuffer,"?f=store&n=%d&t=h&v=%d&v2=%d",source,msgTempHumi.temperature,msgTempHumi.humidity);
					ether.browseUrl(PSTR("/rest/jeenet.php"), textBuffer, website, my_callback);
				}
			break;

			case JEENET_MSG_PULSE_COUNT:
				if (rf12_len==sizeof (jeenetMessagePulseCount) && nodeIndex>=0) {
					memcpy(&msgPulseCount,(void*)rf12_data,sizeof(jeenetMessagePulseCount));

					updateWatchdog(source);
					setAbility(source,JEENET_ABILITY_PULSE_COUNT);
					#ifdef DEBUG
					Serial.print("JEENET_MSG_PULSE_COUNT\t");

					float kwh = msgPulseCount.pulseCount*60/1000;
					int kwh_high=(int)kwh;
					int kwh_low=(msgPulseCount.pulseCount*60)-(kwh_high*1000);
					sprintf(textBuffer,"node %d : c=%d p=%d.%d kW/h    ",source,msgPulseCount.pulseCount,kwh_high,kwh_low);
					Serial.println(textBuffer);
					#endif

					sprintf(textBuffer,"?f=store&n=%d&t=w&v=%d",source,msgPulseCount.pulseCount);
					ether.browseUrl(PSTR("/rest/jeenet.php"), textBuffer, website, my_callback);
				}
			break;
//                                 
//            case JEENET_MSG_RELAY_STATE:
//            if (rf12_len==sizeof (jeenetMessageRelayState) && nodeIndex>=0) {
//                memcpy(&msgRelayState[nodeIndex],(void*)rf12_data,sizeof(jeenetMessageRelayState));
//                updateWatchdog(source);
//                setAbility(source,JEENET_ABILITY_RELAY);
//                #ifdef DEBUG
////                DateTime now = RTC.now();
////                sprintf(textBuffer,"[%.2d:%.2d:%.2d] ",now.hour(),now.minute(),now.second());
////                Serial.print(textBuffer);
//                
//                Serial.print("JEENET_MSG_RELAY_STATE\t");
//              
//                sprintf(textBuffer,"node %d : state=%d",source,msgRelayState[nodeIndex].value);
//           
//                Serial.println(textBuffer);
//                #endif
//            }
//            break;
//            
			case JEENET_MSG_ALIVE:
				if (rf12_len==sizeof (jeenetMessageAlive)) {
					boolean noderespawned=false;

					sprintf(textBuffer,"?f=addnode&v=%d",source);
					ether.browseUrl(PSTR("/rest/jeenet.php"), textBuffer, website, my_callback);

					if (nodeIndex<0) {
						addNode(source);
						noderespawned=true;}
					else {
						#ifdef DEBUG
						long oldWatchDog=jeenet_nodes_watchdog[nodeIndex];
						#endif
						updateWatchdogIndex(nodeIndex);

						#ifdef DEBUG
						int vcc=0;
						vcc |=  rf12_data[2];
						vcc |=  rf12_data[3]<<8;
						if (noderespawned)
							Serial.println("respawned!");
						//                DateTime now = RTC.now();
						//                sprintf(textBuffer,"[%.2d:%.2d:%.2d] ",now.hour(),now.minute(),now.second());
						//                Serial.print(textBuffer);
						sprintf(textBuffer,"JEENET_MSG_ALIVE\tnode %d : d=%lu vcc=%d",jeenet_nodes[nodeIndex],(jeenet_nodes_watchdog[nodeIndex]-oldWatchDog),vcc);
						Serial.println(textBuffer);
						#endif
					}
				}
			break;

		}

		if (RF12_WANTS_ACK) 
			rf12_sendStart(RF12_HDR_CTL | RF12_HDR_DST | source,0,0);
		} 
	}

	if (aliveTimer.poll(1000*50)) {
		unsigned long aliveDelay = 0;
		for (int i=0;i<jeenet_nodes_nbr;i++) {
			aliveDelay = millis()-jeenet_nodes_watchdog[i];
			if (aliveDelay>240000L) {
				#ifdef DEBUG
				sprintf(textBuffer,"RIP node %d! (%lu:%lu:%lu)",jeenet_nodes[i],millis(),jeenet_nodes_watchdog[i],aliveDelay);
				Serial.println(textBuffer);
				#endif        
				removeNode(jeenet_nodes[i]);
				sprintf(textBuffer,"?f=removenode&v=%d",jeenet_nodes[i]);
				ether.browseUrl(PSTR("/rest/jeenet.php"), textBuffer, website, my_callback);
			}
		} 
	}

	if (clockTimer.poll(1000*30)) {
		#ifdef DEBUG
		showNodes(textBuffer,32);
		Serial.println(textBuffer);
		Serial.println(freeRam());
		#endif

		if (clock_iterations%2==0) {
			int t,h;
			dht.reading(t,h);
			t=t/10; h=h/10;
			sprintf(textBuffer,"?f=store&n=%d&t=h&v=%d&v2=%d",JEENET_MASTER_ID,t,h);
			ether.browseUrl(PSTR("/rest/jeenet.php"), textBuffer, website, my_callback);
		}

		if (clock_iterations==3){
			#ifdef DEBUG
			Serial.println("call meteo");
			#endif
			ether.browseUrl(PSTR("/rest/jeenet.php"), "?f=meteo", website, my_callback);
		}
		
		if (clock_iterations==5) {
			#ifdef DEBUG
			Serial.println("call ntp");
			#endif
			ether.browseUrl(PSTR("/rest/jeenet.php"), "?f=time", website, my_callback);  
		}
		
	clock_iterations++; if (clock_iterations==120) clock_iterations=0;
	}  

	wdt_reset();   
}

byte decodeUMSG(jeenetUMessage* msg,byte* type) {
	//Serial.println(strlen((char*)msg->payload));
	//int i=0; 
	*type=msg->payload[0];
	return(1);
}

static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

