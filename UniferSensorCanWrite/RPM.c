#include "STM32_CAN.h" 
 
int  IRPin = PA10;        // This is our input pin (5th pin Rx for USC1 )
int UNIF_LED = PC3;
int counts_per_rev = 3;
int stat = 0;
int count = 0;
unsigned long time_start = micros();
unsigned long time_last_fired = micros();
unsigned long debounce_delay = 500;
static CAN_message_t CAN_TX_msg;

STM32_CAN Can( CAN1, DEF );  //Use PA11/12 pins for CAN1.

//                      RX    TX
HardwareSerial Serial1(PA3, PA2);
void setup() 
{
  Serial1.begin(115200);  
  pinMode(IRPin, INPUT);

  Can.begin();
  Can.setBaudRate(500000);
}
void loop() {
  int sensorStatus = digitalRead(IRPin);
  if (sensorStatus == 1) //Enter this block if nothing is detected
  {
    if (stat == 1) {
      time_last_fired = micros();
    }
    stat = 0;
  }
  else //Enter this block if something is detected
  { 
    digitalWrite(UNIF_LED, HIGH); 
    if (stat == 0 && (micros() - time_last_fired) > debounce_delay) {
      count++;
      time_last_fired = micros();
      //Serial1.print("count: "); Serial1.println(count);
    }
    stat = 1;
  }

  if (count == counts_per_rev) {
    unsigned long elapsed = micros() - time_start;
    //Serial1.print("elapsed: "); Serial1.println((double)elapsed / 1000000);
    uint16_t RPM = (double)60000000 / elapsed;
    Serial1.print("rpm: "); Serial1.println(RPM);

    CAN_TX_msg.id = (0xDB7);
    CAN_TX_msg.len = 2;
    CAN_TX_msg.buf[0] = RPM;
    Can.write(CAN_TX_msg);
    
    time_start = micros();
    
    count = 0;
  }
}