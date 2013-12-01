#ifndef youyoue858d_h
#define youyoue858d_h

typedef struct CPARAM {
	int16_t limit_low;
	int16_t limit_high;
	int16_t value;
	uint8_t eep_addr_high;
	uint8_t eep_addr_low;
} CPARAM;

#define FAN_OFF ( PORTC |= _BV(PC3) )
#define FAN_ON  ( PORTC &= ~_BV(PC3) )
#define FAN_IS_ON ( !(PINC & _BV(PC3)) )
#define FAN_IS_OFF ( PINC & _BV(PC3) )

#define DIG0_OFF ( PORTB &= ~_BV(PB0) )
#define DIG1_OFF ( PORTB &= ~_BV(PB7) )
#define DIG2_OFF ( PORTB &= ~_BV(PB6) )

#define DIG0_ON ( PORTB |= _BV(PB0) )
#define DIG1_ON ( PORTB |= _BV(PB7) )
#define DIG2_ON ( PORTB |= _BV(PB6) )

#define SEGS_OFF ( PORTD = 0xFF )

// THIS IS WHERE IT GETS DANGEROUS
// YOU CAN START A FIRE AND DO A LOT OF HARM WITH
// THE HEATER / TRIAC COMMANDS
#define TRIAC_ON ( PORTB &= ~_BV(PB1) )
#define HEATER_ON TRIAC_OFF
#define TRIAC_OFF ( PORTB |= _BV(PB1) )
#define HEATER_OFF TRIAC_OFF

#define SW0_PRESSED ( !(PINB & _BV(PB5)) )
#define SW1_PRESSED ( !(PINB & _BV(PB2)) )

#define REEDSW_CLOSED ( !(PINB & _BV(PB4)) )
#define REEDSW_OPEN ( PINB & _BV(PB4) )

#define SHOW_SETPOINT_TIMEOUT 2000L

#define HEATER_DUTY_CYCLE_MAX 512L
#define PWM_CYCLES 512L

#define P_GAIN_DEFAULT 0
#define I_GAIN_DEFAULT 0
#define D_GAIN_DEFAULT 0
#define TEMP_OFFSET_CORR_DEFAULT 33

#define TEMPERATURE_AVERAGES 1000L
#define TEMPERATURE_MAX_OVERSHOOT 8
#define TEMPERATURE_REACHED_MARGIN 4
#define MIN_TEMPERATURE_SP 65
#define MAX_TEMPERATURE_SP 500L
#define MAX_TEMPERATURE_ERR 600L

#define SAFE_TO_TOUCH_TEMPERATURE 40
#define FAN_OFF_TEMPERATURE 45
#define FAN_ON_TEMPERATURE 60

#endif
