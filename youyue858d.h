#ifndef youyoue858d_h
#define youyoue858d_h

/*
 * See the Docs folder for how to add a 1 Ohm current sense
 * resistor to meaure the fan-current.
 *
 * Some time in the future, this may be used to check for
 * true fan-speed via the commutation signal.
 *
 * This only requires an opamp (used as comparator) +
 * a few resistors / caps.
 *
 * http://dangerousprototypes.com/2014/03/01/app-note-fan-health-monitoring-and-the-mic502/
 * http://www.micrel.com/_PDF/App-Notes/an-34.pdf
 *
 */
//#define CURRENT_SENSE_MOD

typedef struct CPARAM {
	int16_t value_min;
	int16_t value_max;
	int16_t value_default;
	int16_t value;
	uint8_t eep_addr_high;
	uint8_t eep_addr_low;
} CPARAM;

#if ( defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) )
#define PB0 0
#define PB1 1
#define PB2 2
#define PB3 3
#define PB4 4
#define PB5 5
#define PB6 6
#define PB7 7

#define PC0 0
#define PC1 1
#define PC2 2
#define PC3 3
#define PC4 4
#define PC5 5
#define PC6 6
#define PC7 7

#define PD0 0
#define PD1 1
#define PD2 2
#define PD3 3
#define PD4 4
#define PD5 5
#define PD6 6
#define PD7 7
#endif

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
#define HEATER_ON TRIAC_ON
#define TRIAC_OFF ( PORTB |= _BV(PB1) )
#define HEATER_OFF TRIAC_OFF

#define SW0_PRESSED ( !(PINB & _BV(PB5)) )
#define SW1_PRESSED ( !(PINB & _BV(PB2)) )

#define REEDSW_CLOSED ( !(PINB & _BV(PB4)) )
#define REEDSW_OPEN ( PINB & _BV(PB4) )

#define SHOW_SETPOINT_TIMEOUT 2000L

#define HEATER_DUTY_CYCLE_MAX 512L
#define PWM_CYCLES 512L

#define P_GAIN_DEFAULT 650.0
#define I_GAIN_DEFAULT 15.0
#define D_GAIN_DEFAULT 500.0
#define I_THRESH_DEFAULT 45
#define P_GAIN_SCALING 100.0
#define I_GAIN_SCALING 10000.0
#define D_GAIN_SCALING 25.0

#define TEMP_OFFSET_CORR_DEFAULT 33
#define TEMP_SETPOINT_DEFAULT 75

#define TEMP_AVERAGES_DEFAULT 250L
#define TEMP_REACHED_MARGIN 3

#define MAX_TEMP_ERR 550L
#define SAFE_TO_TOUCH_TEMP 40

#define FAN_OFF_TEMP 45
#define FAN_ON_TEMP 60

#define FAN_SPEED_MIN_DEFAULT 150UL
#define FAN_SPEED_MAX_DEFAULT 315UL

#define FAN_CURRENT_MIN_DEFAULT 30UL
#define FAN_CURRENT_MAX_DEFAULT 71UL

#define SLP_TIMEOUT_DEFAULT 10

#endif
