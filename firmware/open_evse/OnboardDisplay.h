#pragma once
#include "open_evse.h"

#if defined(I2CLCD_Adafruit_Mono)
#define MCP23008    // Adafruit I2C Backpack
#elif defined(I2CLCD_Adafruit_RGB)
#define MCP23017    // Adafruit RGB LCD (PANELOLU2 is now supported without additional define)
#endif

// Using LiquidTWI2 for both types of I2C LCD's
// see http://blog.lincomatic.com/?p=956 for installation instructions
#include "./Wire.h"
#include "./LiquidCrystal_I2C.h"
#include "./LiquidTWI2.h"

#include "Waveshare_LCD1602_RGB.h"

using CustomCharDef_t = const char[8];
extern const CustomCharDef_t CustomCharDefs[];

enum CustomChars {
  INITIAL,
#ifdef DELAYTIMER
  CLOCK,
  STOP,
  PLAY,
#endif // DELAYTIMER
#if defined(DELAYTIMER) || defined(CHARGE_LIMIT)
  LIGHTNING,
#endif
#ifdef AUTH_LOCK
  PADLOCK,
#endif
#ifdef TIME_LIMIT
  TIMELIMIT,
#endif
  LAST
};

enum BackliteType {
  MONO, RGB
};

enum BackliteColor {
 OFF, RED, GREEN, YELLOW, BLUE, VIOLET, TEAL, WHITE
};

struct NoLeds {
  // empty definitions
  void SetGreenLed(uint8_t state) {}
  void SetRedLed(uint8_t state) {}
};

struct RedGreenLeds {
  RedGreenLeds() {
#ifdef GREEN_LED_REG
    pinGreenLed.init(GREEN_LED_REG, GREEN_LED_IDX, DigitalPin::OUT);
    SetGreenLed(0);
#endif
#ifdef RED_LED_REG
    pinRedLed.init(RED_LED_REG, RED_LED_IDX, DigitalPin::OUT);
    SetRedLed(0);
#endif
  }

#ifdef GREEN_LED_REG
public:
  void SetGreenLed(uint8_t state) { pinGreenLed.write(state); }
private:
  DigitalPin pinGreenLed;
#endif

#ifdef RED_LED_REG
public:
  void SetRedLed(uint8_t state) { pinRedLed.write(state); }
private:
  DigitalPin pinRedLed;
#endif
};


template <typename DISPLAY_TYPE, typename LEDS_TYPE>
struct Display : public LEDS_TYPE {

  template <typename... T>
  Display(T... args) : m_Lcd(args...) {
    m_bFlags = Flags::OBDF_MONO_BACKLIGHT;
  };

  void Init() {
    WDT_RESET();
    this->LcdBegin(LCD_MAX_CHARS_PER_LINE, 2);

    // for (int i = 0; i < CustomChars::LAST; i++) {
    //   this->MakeCustomChar(i, CustomCharDefs[i]);
    // }
    this->MakeCustomChar(0, CustomCharDefs[0]);

    this->LcdSetBacklightColor(WHITE);
    LcdClear();
    WDT_RESET();    
  }

  void LcdSetBacklightType(uint8_t type) {
    if (type == BackliteType::MONO) m_bFlags &= ~Flags::OBDF_MONO_BACKLIGHT;
    else m_bFlags |= Flags::OBDF_MONO_BACKLIGHT;
  }
  bool IsLcdBacklightMono() {
    return (m_bFlags & Display::Flags::OBDF_MONO_BACKLIGHT);
  }
  virtual void LcdSetBacklightColor(BackliteColor color) { /*empty by default*/};

  void LcdClear() { m_Lcd.clear(); }

  void LcdPrint(const char *s) { this->LcdPrint_impl(s); }
  void LcdPrint(int i) { m_Lcd.print(i); }

  // print at (0,y), filling out the line with trailing spaces
  void LcdPrint(int y, const char *s) {
    m_Lcd.setCursor(0,y);
    uint8_t i,len = strlen(s);
    if (len > LCD_MAX_CHARS_PER_LINE)
      len = LCD_MAX_CHARS_PER_LINE;
    for (i=0;i < len;i++) {
      m_Lcd.write(s[i]);
    }
    for (i=len;i < LCD_MAX_CHARS_PER_LINE;i++) {
      m_Lcd.write(' ');
    }
  }
  void LcdPrint(int x, int y, const char *s) { 
    m_Lcd.setCursor(x,y);
    m_Lcd.print(s); 
  }

  void LcdPrint_P(PGM_P s) {
    strncpy_P(m_strBuf,s,LCD_MAX_CHARS_PER_LINE);
    m_strBuf[LCD_MAX_CHARS_PER_LINE] = 0;
    m_Lcd.print(m_strBuf);
  }
  void LcdPrint_P(int y, PGM_P s) {
    strncpy_P(m_strBuf,s,LCD_MAX_CHARS_PER_LINE);
    m_strBuf[LCD_MAX_CHARS_PER_LINE] = 0;
    LcdPrint(y,m_strBuf);
  }
  void LcdPrint_P(int x, int y, PGM_P s) {
    strncpy_P(m_strBuf,s,LCD_MAX_CHARS_PER_LINE);
    m_strBuf[LCD_MAX_CHARS_PER_LINE] = 0;
    m_Lcd.setCursor(x,y);
    m_Lcd.print(m_strBuf);
  }
  
  void LcdMsg_P(PGM_P line1,PGM_P line2) {
    LcdPrint_P(0, line1);
    LcdPrint_P(1, line2);
  }
  void LcdMsg(const char *line1, const char *line2) {
    LcdPrint(0, line1);
    LcdPrint(1, line2);
  }

  void LcdCustomChar(enum CustomChars c) { this->LcdCustomChar_impl(c); }

  void LcdSetCursor(int col, int row) { m_Lcd.setCursor(col, row); }
  void LcdClearLine(int row) {
    m_Lcd.setCursor(0, row);
    for (uint8_t i=0; i < LCD_MAX_CHARS_PER_LINE;i++) {
      m_Lcd.write(' ');
    }
    m_Lcd.setCursor(0, row);
  }

  void DisableUpdate(bool on) {
    if (on) m_bFlags |= Flags::OBDF_UPDATE_DISABLED;
    else m_bFlags &= ~Flags::OBDF_UPDATE_DISABLED;
  }
  bool UpdateDisabled() { return (m_bFlags & Flags::OBDF_UPDATE_DISABLED); }

#ifdef AMMETER
  void SetAmmeterDirty(bool b) {
    if (b) m_bFlags |= Flags::OBDF_AMMETER_DIRTY;
    else m_bFlags &= ~Flags::OBDF_AMMETER_DIRTY;
  }
  bool AmmeterIsDirty() { return (m_bFlags & Flags::OBDF_AMMETER_DIRTY); }
#endif // AMMETER

protected:
  virtual void LcdPrint_impl(const char* s) { m_Lcd.print(s); }
  virtual void MakeCustomChar(uint8_t n, PGM_P bytes) = 0;
  virtual void LcdCustomChar_impl(enum CustomChars c) = 0;
  virtual void LcdBegin(uint8_t, uint8_t) = 0;

protected:
  DISPLAY_TYPE m_Lcd;

private:
  enum Flags {
    OBDF_MONO_BACKLIGHT = 0x01,
    OBDF_AMMETER_DIRTY  = 0x80,
    OBDF_UPDATE_DISABLED = 0x40
  };

  uint8_t m_bFlags;
  char m_strBuf[LCD_MAX_CHARS_PER_LINE+1];
};

template <bool Color>
struct Adafruit_Display : public Display<LiquidTWI2, RedGreenLeds> {
  using Base = Display<LiquidTWI2, RedGreenLeds>;
  Adafruit_Display(): Base(LCD_I2C_ADDR, 1) {};

  virtual void LcdSetBacklightColor(BackliteColor color);

protected:
  virtual void LcdBegin(uint8_t, uint8_t);
  virtual void MakeCustomChar(uint8_t n, PGM_P bytes) {
    memcpy_P(g_sTmp, bytes, 8);
    m_Lcd.createChar(n, (uint8_t*)g_sTmp);
  };
  virtual void LcdCustomChar_impl(enum CustomChars c) {
    m_Lcd.write(c);
  };

private:
  bool IsLcdBacklightMono() {}
  static constexpr uint8_t LCD_I2C_ADDR  = 0x20;
};

struct PCF8574_Display : public Display<LiquidCrystal_I2C, NoLeds> {
  using Base = Display<LiquidCrystal_I2C, NoLeds>;

  PCF8574_Display() // Set the pins on the I2C chip used for LCD connections:
                    // addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
        : Base(LCD_I2C_ADDR, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE) {};

protected:
  virtual void LcdBegin(uint8_t, uint8_t);
  virtual void MakeCustomChar(uint8_t n, PGM_P bytes) {
    memcpy_P(g_sTmp, bytes, 8);
    m_Lcd.createChar(n, (uint8_t*)g_sTmp);
  };
  virtual void LcdCustomChar_impl(enum CustomChars c) {
    m_Lcd.write(c);
  };

private:
  static constexpr uint8_t LCD_I2C_ADDR  = 0x27;
};

struct Waveshare_Display : public Display<Waveshare_LCD1602_RGB, NoLeds> {
  using Base = Display<Waveshare_LCD1602_RGB, NoLeds>;
  Waveshare_Display() : Base(16, 2) {}

  virtual void LcdSetBacklightColor(BackliteColor color);

protected:
  virtual void LcdBegin(uint8_t, uint8_t);
  virtual void MakeCustomChar(uint8_t n, PGM_P bytes) {
    uint8_t buffer[8];
    memcpy_P(buffer, bytes, 8);
    m_Lcd.customSymbol(n, buffer);
  };

  virtual void LcdCustomChar_impl(enum CustomChars c) { 
    m_Lcd.write_char(c);
  };
  virtual void LcdPrint_impl(const char *s) { m_Lcd.send_string(s); }
};


#if defined(I2CLCD_Adafruit_Mono)
using OnboardDisplay = Adafruit_Display<false>;
#elif defined(I2CLCD_Adafruit_RGB)
using OnboardDisplay = Adafruit_Display<true>;
#elif defined(I2CLCD_PCF8574)
using OnboardDisplay = PCF8574_Display;
#elif defined(I2CLCD_WAVESHARE)
using OnboardDisplay = Waveshare_Display;
#elif
using OnboardDisplay = void; 
#endif

extern OnboardDisplay g_OBD;
