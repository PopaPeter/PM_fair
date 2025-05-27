#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "lcd.h"
#include "twi.h"

#define BUTTON_ON_OFF    PB0  // D8
#define BUTTON_DIRECTIE  PB1  // D9

#define MOTOR_IN1_PIN    PD3  // D3
#define MOTOR_IN2_PIN    PD2  // D2

#define LED_ROSU         PB5  // D13
#define LED_VERDE        PB4  // D12

#define Voltage_5        PD7  // D7

#define ACS712_OUT       PC0  // A0

#define DEBOUNCE_TIME    20

volatile bool button_on_event = false;
volatile bool button_dir_event = false;
volatile uint8_t button_on_state = 0;
volatile uint8_t button_dir_state = 0;

volatile bool update_power_flag = false;

void timer1_init(void) {
  // CTC mode
  TCCR1B |= (1 << WGM12);
  // Prescaler 64 -> 16MHz / 64 = 250kHz -> 1ms = 250 counts
  OCR1A = 249;
  TIMSK1 |= (1 << OCIE1A);  // compare interrupt
  TCCR1B |= (1 << CS11) | (1 << CS10); // Start timer - prescaler 64
}

void adc_init(void) {
    ADMUX = (1 << REFS0); // referinta AVcc (5V), canal ADC0 (PC0)
    ADCSRA = (1 << ADEN)  // activeaza ADC
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128
}
uint16_t read_acs712(void) {
    ADMUX = (ADMUX & 0xF0) | (0x00);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}
float read_current_acs712(void) {
    uint16_t adc_val = read_acs712();
    float voltage = adc_val * 5.0 / 1023.0;
    float current = (voltage - 2.5) / 0.100; // pentru varianta de 20A (100mV/A)
    return current;
}

void uart_init(uint16_t ubrr) {
    UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}
void uart_send_char(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}
void uart_send_string(const char *s) {
    while (*s) {
        uart_send_char(*s++);
    }
}
void uart_send_float(float val) {
    char buffer[20];
    dtostrf(val, 6, 2, buffer);
    uart_send_string(buffer);
}

ISR(TIMER1_COMPA_vect) {
  // BUTTON_ON_OFF
  static uint8_t on_last = 0;
  static uint8_t on_counter = 0;
  uint8_t on_now = (PINB & (1 << BUTTON_ON_OFF)) ? 1 : 0;

  if (on_now == on_last) {
    if (on_counter < DEBOUNCE_TIME)
      on_counter++;
    else if (on_counter == DEBOUNCE_TIME && button_on_state != on_now) {
      button_on_state = on_now;
      if (on_now) button_on_event = true;
    }
  } else {
    on_counter = 0;
  }
  on_last = on_now;

  // BUTTON_DIRECTIE
  static uint8_t dir_last = 0;
  static uint8_t dir_counter = 0;
  uint8_t dir_now = (PINB & (1 << BUTTON_DIRECTIE)) ? 1 : 0;

  if (dir_now == dir_last) {
    if (dir_counter < DEBOUNCE_TIME)
      dir_counter++;
    else if (dir_counter == DEBOUNCE_TIME && button_dir_state != dir_now) {
      button_dir_state = dir_now;
      if (dir_now) button_dir_event = true;
    }
  } else {
    dir_counter = 0;
  }
  dir_last = dir_now;

  static uint16_t power_timer_counter = 0;

  power_timer_counter++;
  if (power_timer_counter >= 500) { // 500ms
      power_timer_counter = 0;
      update_power_flag = true;
  }
}
int main(void) {
  // Setare butoane ca input
  DDRB &= ~((1 << BUTTON_ON_OFF) | (1 << BUTTON_DIRECTIE));
  PORTB &= ~((1 << BUTTON_ON_OFF) | (1 << BUTTON_DIRECTIE));

  // Motoare ca output
  DDRD |= (1 << MOTOR_IN1_PIN) | (1 << MOTOR_IN2_PIN);
  PORTD &= ~((1 << MOTOR_IN1_PIN) | (1 << MOTOR_IN2_PIN));

  // Leduri ca output
  DDRB |= (1 << LED_ROSU) | (1 << LED_VERDE);
  PORTB |= (1 << LED_ROSU);
  
  //Setare alimentare ACS712
  DDRD |= (1 << Voltage_5);
  PORTD |= (1 << Voltage_5);

  // Setare Out ACS712
  DDRC &= ~(1 << ACS712_OUT);

  //Init timer1 pentru debounce
  timer1_init();
  sei();
  
  //Configurare A0 - pin ACS712 ca input
  adc_init();

  //Debug pe interfata seriala
  //uart_init(103); // 9600 baud la 16 MHz (UBRR = 103)

  //LCD Init
  lcd_init();
  LCD_clear_top_line();
  LCD_clear_bottom_line();
  LCD_printAt(0x00, "Piston READY");
  _delay_ms(1000);
  LCD_clear_top_line();

  bool motor_on = false;
  bool motor_dir = false;

  static float power = 0;
  static float curent = 0;

  while (1) {
  
    if (button_on_event) {
      button_on_event = false;
      motor_on = !motor_on;
      if (motor_on) {
        PORTB &= ~(1 << LED_ROSU);
        PORTB |= (1 << LED_VERDE);
      } else {
        PORTB |= (1 << LED_ROSU);
        PORTB &= ~(1 << LED_VERDE);
      }
    }

    if (button_dir_event) {
      button_dir_event = false;
      motor_dir = !motor_dir;
    }

    if (motor_on) {
      if (motor_dir) {
        PORTD |= (1 << MOTOR_IN1_PIN);
        PORTD &= ~(1 << MOTOR_IN2_PIN);
      } else {
        PORTD |= (1 << MOTOR_IN2_PIN);
        PORTD &= ~(1 << MOTOR_IN1_PIN);
      }
    } else {
      PORTD &= ~((1 << MOTOR_IN1_PIN) | (1 << MOTOR_IN2_PIN)); // motor oprit
    }

    if (update_power_flag) {
        update_power_flag = false;

        curent = read_current_acs712();
        if (curent <= 0.9) curent = 0;
        power = 10.65 * curent;
        char buf_curent[8];
        char buf_power[8];
        char line1[17];

        dtostrf(curent, 5, 2, buf_curent); // width=5, 2 decimale
        dtostrf(power, 5, 1, buf_power);   // width=5, 1 decimală

        snprintf(line1, sizeof(line1), "I:%sA P:%sW", buf_curent, buf_power);
        LCD_clear_top_line();
        LCD_printAt(0x00, line1);

        // Linie 2: stare motor
        LCD_clear_bottom_line();
        if (motor_on) {
            LCD_printAt(0x40, "Stare: ACTIV");
        } else {
            LCD_printAt(0x40, "Stare: OPRIT");
        }
    }
  }
}
