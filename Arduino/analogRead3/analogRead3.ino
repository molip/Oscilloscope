#include <TimerOne.h>

const int Clock_Pin = 4;
const int Data_Pin = 5;
const int CS_Pin = 6;

void setup() 
{ 
//  Serial.begin(460800);
  Serial.begin(230400);

  pinMode(CS_Pin, OUTPUT);
  pinMode(Clock_Pin, OUTPUT);

  digitalWrite(Clock_Pin, LOW);
  digitalWrite(CS_Pin, HIGH);

  Timer1.initialize(50);
  Timer1.attachInterrupt(timer);
  
//  while(1) timer();
} 

void timer()
{
  byte val = 0;

  PORTD = B00000000; // CS_Pin = LOW
  delayMicroseconds(2);

  for (int i = 0; i < 8; ++i)
  {
    val = val << 1;
    val |= ((PIND & B00100000) != 0); // Data_Pin

    PORTD = B00010000; // Clock_Pin = HIGH
    PORTD = B00000000; // Clock_Pin = LOW
  }

  PORTD = B01000000; // CS_Pin = HIGH
  
  Serial.write(val);
}

void loop() 
{ 
}

