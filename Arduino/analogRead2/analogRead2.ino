unsigned long _samples, _start;

const int Clock_Pin = 8;
const int Data_Pin = 9;
const int CS_Pin = 10;

void setup() 
{ 
//  Serial.begin(57600);
  Serial.begin(115200);
//  Serial.begin(230400);

  pinMode(CS_Pin, OUTPUT);
  pinMode(Clock_Pin, OUTPUT);

  digitalWrite(Clock_Pin, LOW);
  digitalWrite(CS_Pin, HIGH);
} 

void loop() 
{ 
   Serial.write(0x88);
   return;

 
  unsigned long now = micros(); 

  if (!_start)
    _start = now;

//  int period = 160; // 6250 Hz
  int period = 50; // 20000 Hz

  if (now - _start >= _samples * period) 
  {
    digitalWrite(CS_Pin, LOW);
    delayMicroseconds(2);

    byte val = 0;
    for (int i = 0; i < 8; ++i)
    {
      val = val << 1;
      val |= digitalRead(Data_Pin);

      digitalWrite(Clock_Pin, HIGH);
      delayMicroseconds(1);
      digitalWrite(Clock_Pin, LOW);
      delayMicroseconds(1);
    }
    digitalWrite(CS_Pin, HIGH);
    
    Serial.write(val);
    ++_samples;
  }
} 

