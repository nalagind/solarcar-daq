#include <STM32RTC.h>


/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

void RTC_Init(const byte hours, const byte minutes, const byte seconds, const byte day, const byte month, const byte year)
{
  rtc.begin(); // initialize RTC 24H format

  // Set the time
  rtc.setHours(hours);
  rtc.setMinutes(minutes);
  rtc.setSeconds(seconds);
  rtc.setDay(day);
  rtc.setMonth(month);
  rtc.setYear(year);
  
	Serial.printf("RTC READY!...");
}

void alarmMatch(void *data)
{

UNUSED(data);
Serial.println("Alarm Match!");

}

void timeStamp()
{
  Serial.printf("%02d/%02d/%02d ", rtc.getDay(), rtc.getMonth(), rtc.getYear());

  Serial.printf("%02d:%02d:%02d.%03d\n", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds(), rtc.getSubSeconds());

}


void stopWatch(int sec){
  rtc.attachSecondsInterrupt(alarmMatch);
  rtc.setAlarmSeconds(sec);
  rtc.enableAlarm(rtc.MATCH_DHHMMSS);
}

