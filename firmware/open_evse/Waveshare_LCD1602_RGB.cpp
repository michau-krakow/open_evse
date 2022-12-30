#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Wire.h"

#include "Waveshare_LCD1602_RGB.h"

Waveshare_LCD1602_RGB::Waveshare_LCD1602_RGB(uint8_t lcd_cols, uint8_t lcd_rows)
{
  _cols = lcd_cols;
  _rows = lcd_rows;
}

void Waveshare_LCD1602_RGB::init()
{
  Wire.begin();
  _showfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  begin(_cols, _rows);
}

void Waveshare_LCD1602_RGB::begin(uint8_t cols, uint8_t lines)
{
  if (lines > 1) {
    _showfunction |= LCD_2LINE;
  }
  _numlines = lines;
  _currline = 0;

  ///< SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  ///< according to datasheet, we need at least 40ms after power rises above 2.7V
  ///< before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
  delay(50);

  ///< this is according to the hitachi HD44780 datasheet
  ///< page 45 figure 23

  ///< Send function set command sequence
  command(LCD_FUNCTIONSET | _showfunction);
  delay(5); // wait more than 4.1ms

  ///< second try
  command(LCD_FUNCTIONSET | _showfunction);
  delay(5);

  ///< third go
  command(LCD_FUNCTIONSET | _showfunction);

  ///< turn the display on with no cursor or blinking default
  _showcontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  display();

  ///< clear it off
  clear();

  ///< Initialize to default text direction (for romance languages)
  _showmode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  ///< set the entry mode
  command(LCD_ENTRYMODESET | _showmode);

  ///< backlight init
  setReg(REG_MODE1, 0);
  ///< set LEDs controllable by both PWM and GRPPWM registers
  setReg(REG_OUTPUT, 0xFF);
  ///< set MODE2 values
  ///< 0010 0000 -> 0x20  (DMBLNK to 1, ie blinky mode)
  setReg(REG_MODE2, 0x20);

  setColorWhite();
}

inline void Waveshare_LCD1602_RGB::command(uint8_t value)
{
  uint8_t data[3] = {0x80, value};
  send(data, 2);
}

void Waveshare_LCD1602_RGB::send(uint8_t *data, uint8_t len)
{
  Wire.beginTransmission(LCD_ADDRESS); // transmit to device #4
  for (int i = 0; i < len; i++)
  {
    Wire.write(data[i]);
    delay(5);
  }
  Wire.endTransmission(); // stop transmitting
}

void Waveshare_LCD1602_RGB::display()
{
  _showcontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _showcontrol);
}

void Waveshare_LCD1602_RGB::clear()
{
  command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
  delayMicroseconds(2000);   // this command takes a long time!
}

void Waveshare_LCD1602_RGB::setReg(uint8_t addr, uint8_t data)
{
  Wire.beginTransmission(RGB_ADDRESS); // transmit to device #4
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission(); // stop transmitting
}

void Waveshare_LCD1602_RGB::setRGB(uint8_t r, uint8_t g, uint8_t b)
{
  setReg(REG_RED, r);
  setReg(REG_GREEN, g);
  setReg(REG_BLUE, b);
}

void Waveshare_LCD1602_RGB::setCursor(uint8_t col, uint8_t row)
{
  col = (row == 0 ? col | 0x80 : col | 0xc0);
  uint8_t data[3] = {0x80, col};

  send(data, 2);
}

void Waveshare_LCD1602_RGB::write_char(uint8_t value)
{
  uint8_t data[3] = {0x40, value};
  send(data, 2);
}

void Waveshare_LCD1602_RGB::send_string(const char *str)
{
  uint8_t i;
  for (i = 0; str[i] != '\0'; i++)
    write_char(str[i]);
}

void Waveshare_LCD1602_RGB::BlinkLED(void)
{
  ///< blink period in seconds = (<reg 7> + 1) / 24
  ///< on/off ratio = <reg 6> / 256
  setReg(0x07, 0x17); // blink every second
  setReg(0x06, 0x7f); // half on, half off
}

void Waveshare_LCD1602_RGB::noBlinkLED(void)
{
  setReg(0x07, 0x00);
  setReg(0x06, 0xff);
}

void Waveshare_LCD1602_RGB::stopBlink()
{
  _showcontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _showcontrol);
}
void Waveshare_LCD1602_RGB::blink()
{
  _showcontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _showcontrol);
}

void Waveshare_LCD1602_RGB::noCursor()
{
  _showcontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _showcontrol);
}

void Waveshare_LCD1602_RGB::cursor()
{
  _showcontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _showcontrol);
}

void Waveshare_LCD1602_RGB::scrollDisplayLeft(void)
{
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void Waveshare_LCD1602_RGB::scrollDisplayRight(void)
{
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void Waveshare_LCD1602_RGB::leftToRight(void)
{
  _showmode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _showmode);
}

void Waveshare_LCD1602_RGB::rightToLeft(void)
{
  _showmode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _showmode);
}

void Waveshare_LCD1602_RGB::noAutoscroll(void)
{
  _showmode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _showmode);
}

void Waveshare_LCD1602_RGB::autoscroll(void)
{
  _showmode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _showmode);
}

void Waveshare_LCD1602_RGB::customSymbol(uint8_t location, uint8_t charmap[])
{
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));

  uint8_t data[9];
  data[0] = 0x40;
  for (int i = 0; i < 8; i++) {
    data[i + 1] = charmap[i];
  }
  send(data, 9);
}

void Waveshare_LCD1602_RGB::home()
{
  command(LCD_RETURNHOME); // set cursor position to zero
  delayMicroseconds(2000); // this command takes a long time!
}
