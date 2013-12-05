#ifndef youyoue858d_h
#define youyoue858d_h

typedef struct CPARAM {
	int16_t value_min;
	int16_t value_max;
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
#define D_GAIN_DEFAULT 750.0
#define I_THRESH_DEFAULT 45
#define P_GAIN_SCALING 100.0
#define I_GAIN_SCALING 10000.0
#define D_GAIN_SCALING 50.0

#define TEMP_OFFSET_CORR_DEFAULT 33
#define TEMP_SETPOINT_DEFAULT 75

#define TEMP_AVERAGES 1000L
#define TEMP_REACHED_MARGIN 3

#define MAX_TEMP_ERR 600L
#define SAFE_TO_TOUCH_TEMP 40

#define FAN_OFF_TEMP 45
#define FAN_ON_TEMP 60

#define SLP_TIMEOUT_DEFAULT 5

#endif
