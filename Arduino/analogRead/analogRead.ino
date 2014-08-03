unsigned long _samples, _start;

void setup() 
{ 
  Serial.begin(115200);
  tone(12, 100);
} 

short _valA;
bool _gotValA;

void loop() 
{ 
  unsigned long now = micros(); 

  if (!_start)
    _start = now;

  int period = 160; // 6250 Hz
//  int period = 150; // 6667 Hz
//  int period = 80; // 12500 Hz

  if (now - _start >= _samples * period) 
  {
    SendSample(analogRead(A0));
    ++_samples;
  }
} 

void SendSample(short val)
{
  if (!_gotValA)
  {
    _valA = val;
    byte data = _valA >> 3; // High 7 bits.
    data |= 0x80; // Start flag.
    Serial.write(data);
    _gotValA = true;
  }
  else
  {
    byte data1 = (_valA & 7) << 4; // Low 3 bits.
    data1 |= val >> 6; // High 4 bits.
    Serial.write(data1);

    byte data2 = (val & 0x3f); // Low 6 bits.
    Serial.write(data2);

    _gotValA = false;
  }
}
