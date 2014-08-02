unsigned long _samples, _start;

void setup() 
{ 
  Serial.begin(115200);
  
  tone(12, 100);
} 

short _valA, _valB;
bool _gotValA;

//short _toneVal;

void loop() 
{ 
  unsigned long now = micros(); 

  if (!_start)
    _start = now;

//  int period = 173; // 5780 Hz
  int period = 160; // 6250 Hz
//    int period = 150; // 6667 Hz

  if (now - _start >= _samples * period) 
  {
    SendSample(analogRead(A0));
    //Serial.write(lowByte(_samples));
    ++_samples;

//    short toneVal = analogRead(A1);
//    if (toneVal != _toneVal)
//    {
//      noTone(12);
//      tone(12, toneVal);
//      _toneVal = toneVal;
//    }
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
    _valB = val;
    byte data1 = (_valA & 7) << 4; // Low 3 bits.
    data1 |= _valB >> 6; // High 4 bits.
    Serial.write(data1);

    byte data2 = (_valB & 0x3f); // Low 6 bits.
    Serial.write(data2);

//    short data = data1 | data2 << 8;
//    Serial.write((byte*)&data, 2);
    _gotValA = false;
  }
}
