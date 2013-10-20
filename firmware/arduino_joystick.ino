#include <Wire.h>
#define DEBUG 0

int slaveAddr = 0x20;
int intPin = 13;
int aPin = 3;
int bPin = 4;
int cPin = 5;
int dPin = 6;
int xPin = 0;
int yPin = 1;
int a, aPrev;
int b, bPrev;
int c, cPrev;
int d, dPrev;
int x, xPrev;
int y, yPrev;
int genInt = 0;

void receiveEvent(int howMany) 
{
  //nothing to do...
}

void requestEvent() 
{
    byte sendData[8];
    
    #if DEBUG
    Serial.print("request\n"); 
    #endif
    
    sendData[0] = (byte)a;
    sendData[1] = (byte)b;
    sendData[2] = (byte)c;
    sendData[3] = (byte)d;
    sendData[4] = (byte)(x & 0xff);
    sendData[5] = (byte)((x >> 8) & 0xff);
    sendData[6] = (byte)(y & 0xff);
    sendData[7] = (byte)((y >> 8) & 0xff);
    Wire.write(sendData, 8);
}

void generateInterrupt()
{  
  genInt = 1;
  digitalWrite(intPin, HIGH);
  delay(1);
  #if DEBUG
  Serial.print(a);
  Serial.print(",");
  Serial.print(b);
  Serial.print(",");
  Serial.print(c);
  Serial.print(",");
  Serial.print(d);
  Serial.print(",");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.println("");
  #endif
  digitalWrite(intPin, LOW);
}

void setup()
{
     #if DEBUG
     Serial.begin(9600) ;               
     Serial.print("setup\n");
     #endif
     
     Wire.begin(slaveAddr) ;                    
     Wire.onRequest(requestEvent) ;     
     Wire.onReceive(receiveEvent) ; 
     
     pinMode(intPin, OUTPUT);
     digitalWrite(intPin, LOW);

     pinMode(aPin, INPUT);
     digitalWrite(aPin, HIGH);
     pinMode(bPin, INPUT);
     digitalWrite(bPin, HIGH);
     pinMode(cPin, INPUT);
     digitalWrite(cPin, HIGH);
     pinMode(dPin, INPUT);
     digitalWrite(dPin, HIGH);
     
}

void loop()
{
  int dx, dy;
  
  aPrev = a;
  bPrev = b;
  cPrev = c;
  dPrev = d;
  xPrev = x;
  yPrev = y;
  a = digitalRead(aPin);
  b = digitalRead(bPin);
  c = digitalRead(cPin);
  d = digitalRead(dPin);
  x = analogRead(xPin);
  y = analogRead(yPin);

  dx = x - xPrev;
  dy = y - yPrev;
  
  if(a != aPrev || b != bPrev || c != cPrev || d != dPrev)
    generateInterrupt();
  else if(dx > 5 || dx < -5 || dy > 5 || dy < -5)
    generateInterrupt();
  delay(20);
}
