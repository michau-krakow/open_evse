#include "OnboardDisplay.h"

OnboardDisplay g_OBD;


const CustomCharDef_t CustomCharDefs[] PROGMEM = 
{
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0}, // clock
#if defined(DELAYTIMER)
    {0x0, 0xe, 0x15, 0x17, 0x11, 0xe, 0x0, 0x0}, // clock
    {0x0, 0x0, 0xe, 0xe, 0xe, 0x0, 0x0, 0x0}, // stop (cube)
    {0x0, 0x8, 0xc, 0xe, 0xc, 0x8, 0x0, 0x0}, // play
#endif // DELAYTIMER
#if defined(DELAYTIMER) || defined(CHARGE_LIMIT)
    {0x0, 0xe, 0xc, 0x1f, 0x3, 0x6, 0xc, 0x8}, // lightning
#endif
#ifdef AUTH_LOCK
    {// padlock
     0b00000,
     0b01110,
     0b01010,
     0b11111,
     0b11011,
     0b11011,
     0b01110,
     0b00000},
#endif // AUTH_LOCK
#ifdef TIME_LIMIT
    {0b00000,
     0b01110,
     0b10001,
     0b11101,
     0b10101,
     0b01110,
     0b00000,
     0b00000},
#endif // TIME_LIMIT
};


void Waveshare_Display::LcdBegin(uint8_t, uint8_t) {
  m_Lcd.init();
  m_Lcd.setColorWhite();
}

void Waveshare_Display::LcdSetBacklightColor(BackliteColor color) {
  switch (color)
  {
  case RED: m_Lcd.setRGB(255, 0, 0); break;
  case GREEN: m_Lcd.setRGB(0, 255, 0); break;
  case BLUE: m_Lcd.setRGB(0, 0, 255); break;
  case YELLOW: m_Lcd.setRGB(255, 255, 0); break;
  case TEAL: m_Lcd.setRGB(0, 192, 192); break;
  case VIOLET: m_Lcd.setRGB(148, 0, 211); break;
  case WHITE: m_Lcd.setRGB(255, 255, 255); break;
  default:  m_Lcd.setColorWhite(); break;
  }
}


template <bool Color>
void Adafruit_Display<Color>::LcdBegin(uint8_t cols, uint8_t rows) {
  if (Color) {
    Base::m_Lcd.setMCPType(LTI_TYPE_MCP23017);
    Base::m_Lcd.begin(cols, rows, 2);
    Base::m_Lcd.setBacklight(WHITE);
  } else {
    Base::m_Lcd.setMCPType(LTI_TYPE_MCP23008);
    Base::m_Lcd.begin(cols, rows);
  }
}

template <>
void Adafruit_Display<true>::LcdSetBacklightColor(BackliteColor color) {
  if (IsLcdBacklightMono()) {
    color = WHITE;
  }
  m_Lcd.setBacklight(color);
}


void PCF8574_Display::LcdBegin(uint8_t cols, uint8_t rows) {
  m_Lcd.begin(cols, rows);
  m_Lcd.setBacklight(HIGH);
}
