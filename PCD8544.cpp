/*
 * PCD8544 - Interface with Philips PCD8544 (or compatible) LCDs.
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


#include "PCD8544.h"

#include <Arduino.h>

#if defined (__XTENSA__)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif


#define PCD8544_CMD  LOW
#define PCD8544_DATA HIGH


/*
 * If this was a ".h", it would get added to sketches when using
 * the "Sketch -> Import Library..." menu on the Arduino IDE...
 */
#include "charset.cpp"


PCD8544::PCD8544(uint8_t sclk, uint8_t sdin, uint8_t dc, uint8_t reset, uint8_t sce):
    pin_sclk(sclk),
    pin_sdin(sdin),
    pin_dc(dc),
    pin_reset(reset),
    pin_sce(sce)
{}


void PCD8544::begin(uint8_t width, uint8_t height, uint8_t model)
{
    this->width = width;
    this->height = height;

    // Only two chip variants are currently known/supported...
    this->model = (model == CHIP_ST7576) ? CHIP_ST7576 : CHIP_PCD8544;

    this->column = 0;
    this->line = 0;

    // Sanitize the custom glyphs...
    memset(this->custom, 0, sizeof(this->custom));

    // All pins are outputs (these displays cannot be read)...
    pinMode(this->pin_sclk, OUTPUT);
    pinMode(this->pin_sdin, OUTPUT);
    pinMode(this->pin_dc, OUTPUT);
    pinMode(this->pin_reset, OUTPUT);
    pinMode(this->pin_sce, OUTPUT);

    // Reset the controller state...
    digitalWrite(this->pin_reset, HIGH);
    digitalWrite(this->pin_sce, HIGH);
    digitalWrite(this->pin_reset, LOW);
    delay(100);
    digitalWrite(this->pin_reset, HIGH);

    // Set the LCD parameters...
    this->send(PCD8544_CMD, 0x21);  // extended instruction set control (H=1)
    this->send(PCD8544_CMD, 0x13);  // bias system (1:48)

    if (this->model == CHIP_ST7576) {
        this->send(PCD8544_CMD, 0xe0);  // higher Vop, too faint at default
        this->send(PCD8544_CMD, 0x05);  // partial display mode
    } else {
        this->send(PCD8544_CMD, 0xc2);  // default Vop (3.06 + 66 * 0.06 = 7V)
    }

    this->send(PCD8544_CMD, 0x20);  // extended instruction set control (H=0)
    this->send(PCD8544_CMD, 0x09);  // all display segments on

    // Clear RAM contents...
    this->clear();

    // Activate LCD...
    this->send(PCD8544_CMD, 0x08);  // display blank
    this->send(PCD8544_CMD, 0x0c);  // normal mode (0x0d = inverse mode)
    delay(100);

    // Place the cursor at the origin...
    this->send(PCD8544_CMD, 0x80);
    this->send(PCD8544_CMD, 0x40);
}


void PCD8544::stop()
{
    this->clear();
    this->setPower(false);
}


void PCD8544::clear()
{
    this->setCursor(0, 0);

    for (uint16_t i = 0; i < this->width * (this->height/8); i++) {
        this->send(PCD8544_DATA, 0x00);
    }

    this->setCursor(0, 0);
}


void PCD8544::clearLine()
{
    this->setCursor(0, this->line);

    for (uint8_t i = 0; i < this->width; i++) {
        this->send(PCD8544_DATA, 0x00);
    }

    this->setCursor(0, this->line);
}


void PCD8544::setPower(bool on)
{
    this->send(PCD8544_CMD, on ? 0x20 : 0x24);
}


inline void PCD8544::display()
{
    this->setPower(true);
}


inline void PCD8544::noDisplay()
{
    this->setPower(false);
}


void PCD8544::setInverse(bool enabled)
{
    this->send(PCD8544_CMD, enabled ? 0x0d : 0x0c);
}


void PCD8544::setInverseOutput(bool enabled)
{
    this->inverse_output = enabled;
}


void PCD8544::setContrast(uint8_t level)
{
    // The PCD8544 datasheet specifies a maximum Vop of 8.5V for safe
    // operation in low temperatures, which limits the contrast level.
    if (this->model == CHIP_PCD8544 && level > 90) {
        level = 90;  // Vop = 3.06 + 90 * 0.06 = 8.46V
    }

    // The ST7576 datasheet specifies a minimum Vop of 4V.
    if (this->model == CHIP_ST7576 && level < 36) {
        level = 36;  // Vop = 2.94 + 36 * 0.03 = 4.02V
    }

    this->send(PCD8544_CMD, 0x21);  // extended instruction set control (H=1)
    this->send(PCD8544_CMD, 0x80 | (level & 0x7f));
    this->send(PCD8544_CMD, 0x20);  // extended instruction set control (H=0)
}


void PCD8544::home()
{
    this->setCursor(0, this->line);
}


void PCD8544::setCursor(uint8_t column, uint8_t line)
{
    this->column = (column % this->width);
    this->line = (line % (this->height/9 + 1));

    this->send(PCD8544_CMD, 0x80 | this->column);
    this->send(PCD8544_CMD, 0x40 | this->line);
}


void PCD8544::createChar(uint8_t chr, const uint8_t *glyph)
{
    // ASCII 0-31 only...
    if (chr >= ' ') {
        return;
    }

    this->custom[chr] = glyph;
}


size_t PCD8544::write(uint8_t chr)
{
    // ASCII 7-bit only...
    if (chr >= 0x80) {
        return 0;
    }

    uint8_t textHeight       = readFontData(fontData, HEIGHT_POS);
    uint8_t firstChar        = readFontData(fontData, FIRST_CHAR_POS);
    uint16_t sizeOfJumpTable = readFontData(fontData, CHAR_NUM_POS)  * JUMPTABLE_BYTES;
    
    if (chr >= firstChar) {
      byte charCode = chr - firstChar;
      // 4 Bytes per char 
      byte msbJumpToChar    = readFontData( fontData, JUMPTABLE_START + charCode * JUMPTABLE_BYTES );                  // MSB  \ JumpAddress
      byte lsbJumpToChar    = readFontData( fontData, JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_LSB);   // LSB /
      byte charByteSize     = readFontData( fontData, JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_SIZE);  // Size
      byte currentCharWidth = readFontData( fontData, JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_WIDTH); // Width

      // Test if the char is drawable
      if (!(msbJumpToChar == 255 && lsbJumpToChar == 255)) {
        // Get the position of the char data
        uint16_t charDataPosition = JUMPTABLE_START + sizeOfJumpTable + ((msbJumpToChar << 8) + lsbJumpToChar);
  
          //get number of lines from font height
        uint8_t lines = (int) ceil(textHeight / 8.0);

        for(uint8_t j=0; j < lines; j++){
            // Update the cursor position...            
            this->send(PCD8544_CMD, 0x40 | ((this->line + j) % (this->height/8)) );
            this->send(PCD8544_CMD, 0x80 | this->column);
            //((this->line + j) % this->height) 
            for (uint16_t i = 0; i < currentCharWidth; i++) {
                byte currentByte = readFontData(fontData, charDataPosition + i + j * currentCharWidth);
                this->send(PCD8544_DATA, this->inverse_output ? ~currentByte : currentByte);
                yield();
            }
        }
      }      
        // Update the cursor position...
        this->column = (this->column + currentCharWidth) % this->width;

        if (this->column == 0) {
            this->line = (this->line + 1) % (this->height/8);
        }


    }


    return 1;
}


void PCD8544::drawBitmap(const uint8_t *data, uint8_t columns, uint8_t lines)
{
    uint8_t scolumn = this->column;
    uint8_t sline = this->line;

    // The bitmap will be clipped at the right/bottom edge of the display...
    uint8_t mx = (scolumn + columns > this->width) ? (this->width - scolumn) : columns;
    uint8_t my = (sline + lines > this->height/8) ? (this->height/8 - sline) : lines;

    for (uint8_t y = 0; y < my; y++) {
        this->setCursor(scolumn, sline + y);

        for (uint8_t x = 0; x < mx; x++) {
            this->send(PCD8544_DATA, data[y * columns + x]);
        }
    }

    // Leave the cursor in a consistent position...
    this->setCursor(scolumn + columns, sline);
}


void PCD8544::drawColumn(uint8_t lines, uint8_t value)
{
    uint8_t scolumn = this->column;
    uint8_t sline = this->line;

    // Keep "value" within range...
    if (value > lines*8) {
        value = lines*8;
    }

    // Find the line where "value" resides...
    uint8_t mark = (lines*8 - 1 - value)/8;

    // Clear the lines above the mark...
    for (uint8_t line = 0; line < mark; line++) {
        this->setCursor(scolumn, sline + line);
        this->send(PCD8544_DATA, 0x00);
    }

    // Compute the byte to draw at the "mark" line...
    uint8_t b = 0xff;
    for (uint8_t i = 0; i < lines*8 - mark*8 - value; i++) {
        b <<= 1;
    }

    this->setCursor(scolumn, sline + mark);
    this->send(PCD8544_DATA, b);

    // Fill the lines below the mark...
    for (uint8_t line = mark + 1; line < lines; line++) {
        this->setCursor(scolumn, sline + line);
        this->send(PCD8544_DATA, 0xff);
    }

    // Leave the cursor in a consistent position...
    this->setCursor(scolumn + 1, sline);
}


void PCD8544::send(uint8_t type, uint8_t data)
{
    digitalWrite(this->pin_dc, type);

    digitalWrite(this->pin_sce, LOW);
    shiftOut(this->pin_sdin, this->pin_sclk, MSBFIRST, data);
    digitalWrite(this->pin_sce, HIGH);
}


void PCD8544::setFont(const uint8_t* font) {
    this->fontData = font;
}

uint8_t PCD8544::readFontData(const uint8_t* fontData, uint32_t offset) {
    return pgm_read_byte(fontData + offset);
}

/* vim: set expandtab ts=4 sw=4: */
