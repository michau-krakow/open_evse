#pragma once
/*
 * This file is part of Open EVSE.
 *
 * Copyright (c) 2011-2019 Sam C. Lin
 *
 * Open EVSE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.

 * Open EVSE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Open EVSE; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "open_evse.h"

static constexpr int PILOT_FREQ = 1000; // 1 kHz

#if (PILOT_TIMER_ID == 1)
#define PILOT_TCCRnA TCCR1A
#define PILOT_TCCRnB TCCR1B
#define PILOT_ICRn ICR1
#define PILOT_OCRnA OCR1A
#define PILOT_OCRnB OCR1B
#define PILOT_OCRnC OCR1C
#elif (PILOT_TIMER_ID == 3)
#define PILOT_TCCRnA TCCR3A
#define PILOT_TCCRnB TCCR3B
#define PILOT_ICRn ICR3
#define PILOT_OCRnA OCR3A
#define PILOT_OCRnB OCR3B
#define PILOT_OCRnC OCR3C
#else
#error This timer is not supported for Pilot (yet)
#endif

typedef enum
{
  P12, // steady +12V (VEHICLE NOT CONNECTED)
  PWM,
  N12 // steady -12V (FAULT)
} PILOT_STATE;

template <typename PilotType>
struct J1772PilotBase {
  PILOT_STATE m_State;

  void Init() {
    Serial.println("Pilot init...");
    static_cast<PilotType *>(this)->initialize();
    SetState(PILOT_STATE::P12);
  }

  void SetState(PILOT_STATE state) {
    AutoCriticalSection asc;
    m_State = state;
    static_cast<PilotType *>(this)->updatePwm(0);
  }

  PILOT_STATE GetState() {
    return m_State;
  }

  // set EVSE current capacity in Amperes
  // outputting a 1KHz square wave with correct duty cycle
  int SetPWM(int amps) { // 12V 1KHz PWM
    Serial.print("Setting PWM amps: ");
    Serial.println(amps);

    m_State = PILOT_STATE::PWM;

    int duty = 0;
    if ((amps >= 6) && (amps <= 51)) {
      // J1772 states "Available current = (duty cycle %) X 0.6"
      duty = 10 * amps / 6;
    }
    else if ((amps > 51) && (amps <= 80)) {
      // J1772 states "Available current = (duty cycle % - 64) X 2.5"
      duty = 25 * amps / 10 + 64;
    }

    if (duty > 1 && duty < 100)
      return static_cast<PilotType *>(this)->updatePwm(duty);
    else
      return 1;
  }
};

// using Phase-and-Frequency-Correct mode of PWM generator
struct J1772Pilot_PAFC : public J1772PilotBase<J1772Pilot_PAFC>
{
  static constexpr int TOP = ((F_CPU / 2000000) * PILOT_FREQ);

  void initialize() {
    PILOT_TCCRnA = 0; // set up Control Register A
    PILOT_ICRn = TOP;
    // WGM13 -> select P&F mode CS10 -> prescaler = 1
    PILOT_TCCRnB = _BV(WGM13) | _BV(CS10);
    PILOT_DDR |= _BV(PILOT_PWM_PIN);

#if (PILOT_OC_CHANNEL == 'A')
    PILOT_TCCRnA |= _BV(COM1A1);
#elif (PILOT_OC_CHANNEL == 'B')
    PILOT_TCCRnA |= _BV(COM1B1);
#elif (PILOT_OC_CHANNEL == 'C')
    PILOT_TCCRnA |= _BV(COM1C1);
#endif // PILOT_OC_CHANNEL
  }

  int updatePwm(int duty) {
    uint16_t ocr;
    switch (m_State)
    {
    case P12:
      ocr = TOP;
      break;
    case N12:
      ocr = 0;
      break;
    default: // PWM
      // dutycycle = OCRnA(B) / ICRn * 100 %
      ocr = duty * TOP / 100;
      break;
    }

#if (PILOT_OC_CHANNEL == 'A')
    PILOT_OCRnA = ocr;
#elif (PILOT_OC_CHANNEL == 'B')
    PILOT_OCRnB = ocr;
#elif (PILOT_OC_CHANNEL == 'C')
    PILOT_OCRnC = ocr;
#endif
    return 0;
  }
};

struct J1772Pilot_FastPwm : public J1772PilotBase<J1772Pilot_FastPwm>
{
  DigitalPin pin;

  void initialize() {
    pin.init(PILOT_REG, PILOT_PWM_PIN, DigitalPin::OUT);
  }

  int updatePwm(int duty) {
    switch (m_State)
    {
    case P12:
    case N12:
      PILOT_TCCRnA = 0; // disable pwm
      pin.write((m_State == PILOT_STATE::P12) ? 1 : 0);
      break;
    default: { // PWM
        AutoCriticalSection asc;

        // Select Fast PWM mode: TOP=OCRnA, Update of OCRnx at BOTTOM
        PILOT_TCCRnB = _BV(WGM13) | _BV(WGM12);
        PILOT_TCCRnA = _BV(WGM11) | _BV(WGM10);
        PILOT_TCCRnB |= _BV(CS10);    // Prescaler = 1

        uint32_t top = F_CPU / PILOT_FREQ;
        PILOT_OCRnA = top - 1;

#if (PILOT_OC_CHANNEL == 'A')
        PILOT_TCCRnA |= _BV(COM1A1);  // Clear OCnA on compare match, set OCnA at BOTTOM (non-inverting mode)
        PILOT_OCRnA = duty * top / 100;
#elif (PILOT_OC_CHANNEL == 'B')
        PILOT_TCCRnA |= _BV(COM1B1);  // Clear OCnB on compare match, set OCnB at BOTTOM (non-inverting mode)
        PILOT_OCRnB = duty * top / 100;
#elif (PILOT_OC_CHANNEL == 'C')
        PILOT_TCCRnA |= _BV(COM1C1);  // Clear OCnB on compare match, set OCnC at BOTTOM (non-inverting mode)
        PILOT_OCRnC = duty * top / 100;
#endif
      }
      break;
    }
  }
};

// enable either:
#ifdef PAFC_PWM
#define J1772Pilot J1772Pilot_PAFC
#else
#define J1772Pilot J1772Pilot_FastPwm
#endif
