/*
 * Thermometer - read temperature using an LM35 sensor and display it on a PCD8544 LCD.
 *
 * Copyright (c) 2010 Carlos Rodrigues <cefrodrigues@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include <PCD8544.h>


static const byte sensorPin = 0;
static const byte ledPin = 13;

// The dimensions of the LCD (in pixels)...
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

// The number of lines for the temperature chart...
static const byte CHART_HEIGHT = 5;

// A custom "degrees" symbol...
static const byte DEGREES_CHAR = 1;
static const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };

// A bitmap graphic (10x2) of a thermometer...
static const byte THERMO_WIDTH = 10;
static const byte THERMO_HEIGHT = 2;
static const byte thermometer[] = { 0x00, 0x00, 0x48, 0xfe, 0x01, 0xfe, 0x00, 0x02, 0x05, 0x02,
                                    0x00, 0x00, 0x62, 0xff, 0xfe, 0xff, 0x60, 0x00, 0x00, 0x00};

static PCD8544 lcd;

int Vout;
char* charTc ;
float R1 = 10000; //otpornost na sobnoj temperaturi 10K ili 10000 oma
float R2, Tk, Tc;
float Ac = 1.009249522e-03, Bc = 2.378405444e-04, Cc = 2.019202697e-07;
char charBuffer[10];


void setup() {
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  
  // Register the custom symbol...
  lcd.createChar(DEGREES_CHAR, degrees_glyph);
  
  pinMode(ledPin, OUTPUT);

  // The internal 1.1V reference provides for better
  // resolution from the LM35, and is also more stable
  // when powered from either a battery or USB...
  //analogReference(INTERNAL);
}


void loop() {
  // Start beyond the edge of the screen...
  static byte xChart = LCD_WIDTH;
  
  digitalWrite(ledPin, HIGH);

  Vout = analogRead(sensorPin);
  R2 = R1 * (1023.0 / (float)Vout - 1.0); // konvertovanje iz analogne u digitalnu
  Tk = (1.0 / (Ac + Bc*log(R2) + Cc*log(R2)*log(R2)*log(R2))); // temperatura u Kelvinima (K)
  Tc = Tk - 273.15; //temperatura u stepenima Celzijusa

  charTc = dtostrf(Tc, 3, 2, charBuffer);
 


  // Read the temperature (in celsius)...
  float temp = Tc;//(1.1 * analogRead(sensorPin) * 100.0) / 1024.0;
  
  // Print the temperature (using the custom "degrees" symbol)...
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp, 1);
  lcd.print(" \001C ");

  // Draw the thermometer bitmap at the bottom left corner...
  lcd.setCursor(0, LCD_HEIGHT/8 - THERMO_HEIGHT);
  lcd.drawBitmap(thermometer, THERMO_WIDTH, THERMO_HEIGHT);

  // Wrap the chart's current position...
  if (xChart >= LCD_WIDTH) {
    xChart = THERMO_WIDTH + 2;
  }

  // Update the temperature chart...  
  lcd.setCursor(xChart, 1);
  lcd.drawColumn(CHART_HEIGHT, map(temp, 0, 45, 0, CHART_HEIGHT*8));  // ...clipped to the 0-45C range.
  lcd.drawColumn(CHART_HEIGHT, 0);         // ...with a clear marker to see the current chart position.
  
  xChart++;

  digitalWrite(ledPin, LOW);  
  delay(500);
}


/* EOF - Thermometer.ino  */
