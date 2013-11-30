/*
 * This is a custom firmware for my 'Youyue 858D+' hot-air sodlering station.
 * It may or may not be useful to you, always double check if you use it.
 *
 * PCB version: 858D V4.1
 * Date code:   20130421
 *
 * Other identifiers (see images)
 *
 * V1.05 PID temperature control + heater indicator + persistent setpoint storage + better button handling
 *
 * 2013 - Robert Spitzenpfeil
 *
 * Licence: GNU GPL v2
 *
 */

/*
 * PC5: FAN-speed (A5 in Arduino lingo) - NOT USED SO FAR (OK)
 * PC3: TIP122.base --> FAN (OK)
 * PC0: ADC <-- amplif. thermo couple voltage (A0 in Arduino lingo) (OK)
 * #21: AREF <--- about 2.5V as analog reference for ADC
 * PB1: opto-triac driver !! THIS IS DANGEROUS TO USE !! (OK)
 *
 * PB0: 7-seg digit 0 (OK)
 * PB7: 7-seg digit 1 (OK)
 * PB6: 7-seg digit 2 (OK)
 *
 * PD0: 7-seg top (OK)
 * PD1: 7-seg bottom left (OK)
 * PD2: 7-seg bottom (OK)
 * PD3: 7-seg top left (OK)
 * PD4: 7-seg dot (OK)
 * PD5: 7-seg bottom right (OK)
 * PD6: 7-seg middle (OK)
 * PD7: 7-seg top right (OK)
 *
 * PB5: SW1 (button1) (OK)
 * PB2: SW2 (button2) (OK)
 * PB4: reed switch (wand craddle sensor) (OK)
 *
 */

#include <EEPROM.h>

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

// P_GAIN ( HEATER_DUTY_CYCLE_MAX / (MAX_TEMPERATURE_SP - 35) )
#define P_GAIN 1.10
#define I_GAIN 0.0
#define D_GAIN 0.0

#define HEATER_DUTY_CYCLE_MAX 512L
#define PWM_CYCLES 512L
#define TEMPERATURE_CALIB_OFFSET 33

#define TEMPERATURE_AVERAGES 1000L
#define TEMPERATURE_MAX_OVERSHOOT 8
#define TEMPERATURE_REACHED_MARGIN 4
#define MIN_TEMPERATURE_SP 65
#define MAX_TEMPERATURE_SP 500L
#define MAX_TEMPERATURE_ERR 600L

#define SAFE_TO_TOUCH_TEMPERATURE 40
#define FAN_OFF_TEMPERATURE 45
#define FAN_ON_TEMPERATURE 60

uint8_t framebuffer[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };	// dig0, dig1, dig2, dot0, dot1, dot2 - couting starts from right side

void setup(void)
{
	HEATER_OFF;
	DDRB |= _BV(PB1);	// set as output for TRIAC control

	DDRB &= ~(_BV(PB5) | _BV(PB2));	// set as inputs (switches)
	PORTB |= (_BV(PB5) | _BV(PB2));	// pull-up on

	DDRB &= ~_BV(PB4);	// set as input (reed sensor)
	PORTB |= _BV(PB4);	// pull-up on

	FAN_OFF;
	DDRC |= _BV(PC3);	// set as output (FAN control)

	DDRD |= 0xFF;		// all as outputs (7-seg segments)
	DDRB |= (_BV(PB0) | _BV(PB6) | _BV(PB7));	// 7-seg digits 1,2,3

	analogReference(EXTERNAL);	// use external 2.5V as ADC reference voltage (VCC / 2)
}

void loop(void)
{
	//segm_test();
	fan_test();
	//char_test();

	setup_timer1_ctc();

	uint16_t tmp = (EEPROM.read(0) << 8) | EEPROM.read(1);
	uint16_t temperature_setpoint;

	if ((tmp >= MIN_TEMPERATURE_SP) && (tmp <= MAX_TEMPERATURE_SP)) {
		temperature_setpoint = tmp;
	} else {
		temperature_setpoint = MIN_TEMPERATURE_SP;
	}

	while (1) {
		static uint16_t temperature_inst = 0;
		static uint16_t temperature_inst_previous = 0;
		static uint32_t temperature_accu = 0;
		static uint16_t temperature_average = 0;
		static uint16_t temperature_average_previous = 0;

		static uint32_t button_input_time = 0;

		static uint16_t heater_ctr = 0;
		static int32_t heater_duty_cycle = 0;
		static int16_t error = 0;
		static int32_t error_accu = 0;
		static int16_t velocity = 0;
		static float PID_drive = 0;

		static uint16_t button_counter = 0;

		static uint8_t temperature_setpoint_saved = 0;
		static uint32_t temperature_setpoint_saved_time = 0;

		temperature_inst_previous = temperature_inst;
		temperature_inst = analogRead(A0) + TEMPERATURE_CALIB_OFFSET;	// approx. temp in °C

		if (REEDSW_OPEN && (temperature_setpoint >= MIN_TEMPERATURE_SP)
		    && (temperature_average < MAX_TEMPERATURE_ERR)) {
			// !! DANGER !!
			FAN_ON;

			error = (int16_t)(temperature_setpoint) - (int16_t)(temperature_average);
			error_accu += (int32_t)(error);
			velocity = (int16_t)(temperature_average_previous) - (int16_t)(temperature_average);

			PID_drive = (float)(error) * (float)(P_GAIN) + (float)(error_accu) * (float)(I_GAIN) + (float)(velocity) * (float)(D_GAIN);

			heater_duty_cycle = (int32_t) (PID_drive);

			if (heater_duty_cycle > HEATER_DUTY_CYCLE_MAX) {
				heater_duty_cycle = HEATER_DUTY_CYCLE_MAX;
			}

			if (heater_duty_cycle < 0) {
				heater_duty_cycle = 0;
			}

			if (heater_ctr < heater_duty_cycle) {
				set_dot();
			//	if (temperature_average < (temperature_setpoint + TEMPERATURE_MAX_OVERSHOOT)) {	// hard limit for top temperature
					HEATER_ON;
			//	} else {
			//		HEATER_OFF;
			//		clear_dot();
			//	}
			} else {
				HEATER_OFF;
				clear_dot();
			}

			heater_ctr++;
			if (heater_ctr == PWM_CYCLES) {
				heater_ctr = 0;
			}

		} else {
			HEATER_OFF;
			clear_dot();
		}

		static uint16_t temp_avg_ctr = 0;

		temperature_accu += (uint32_t) (temperature_inst);
		temp_avg_ctr++;

		if (temp_avg_ctr == TEMPERATURE_AVERAGES) {
			temperature_average_previous = temperature_average;
			temperature_average =
			    (uint16_t) (temperature_accu /
					TEMPERATURE_AVERAGES);
			temperature_accu = 0;
			temp_avg_ctr = 0;
		}

		if (temperature_average >= FAN_ON_TEMPERATURE) {
			FAN_ON;
		} else if (REEDSW_CLOSED
			   && (temperature_average <= FAN_OFF_TEMPERATURE)) {
			FAN_OFF;
		} else if (REEDSW_OPEN) {
			FAN_ON;
		}

		if (SW0_PRESSED) {
			button_input_time = millis();
			button_counter++;

			if (button_counter == 200) {

				if (temperature_setpoint < MAX_TEMPERATURE_SP) {
					temperature_setpoint++;
					temperature_setpoint_saved = 0;
				}

			}

			if (button_counter == 800) {

				if (temperature_setpoint <
				    (MAX_TEMPERATURE_SP - 10)) {
					temperature_setpoint += 10;
					temperature_setpoint_saved = 0;
				}

				button_counter = 201;

			}

		} else if (SW1_PRESSED) {
			button_input_time = millis();
			button_counter++;

			if (button_counter == 200) {

				if (temperature_setpoint >= MIN_TEMPERATURE_SP) {	// allows for cold air
					temperature_setpoint--;
					temperature_setpoint_saved = 0;
				}

			}

			if (button_counter == 800) {

				if (temperature_setpoint >
				    (MIN_TEMPERATURE_SP + 10)) {
					temperature_setpoint -= 10;
					temperature_setpoint_saved = 0;
				}

				button_counter = 201;

			}

		} else {
			button_counter = 0;
		}

		if ((millis() - button_input_time) < SHOW_SETPOINT_TIMEOUT) {
			display_number(temperature_setpoint);	// show temperature setpoint
		} else {
			if (temperature_setpoint_saved == 0) {
				set_eeprom_saved_dot();
				EEPROM.write(0, highByte(temperature_setpoint));
				EEPROM.write(1, lowByte(temperature_setpoint));
				temperature_setpoint_saved_time = millis();
				temperature_setpoint_saved = 1;
			} else if (temperature_average <=
				   SAFE_TO_TOUCH_TEMPERATURE) {
				display_number(6666);
			} else if (temperature_average > MAX_TEMPERATURE_ERR) {
				display_number(9999);	// probably the wand is not connected or thermo couple has failed
			} else
			    if (abs
				((int16_t) (temperature_average) -
				 (int16_t) (temperature_setpoint)) <
				TEMPERATURE_REACHED_MARGIN) {
				display_number(temperature_setpoint);	// avoid showing insignificant fluctuations on the display (annoying)
			} else {
				display_number(temperature_average);
				//display_number(temperature_inst);
			}
		}

		if ((millis() - temperature_setpoint_saved_time) > 500) {
			clear_eeprom_saved_dot();
		}
	}

}

void set_dot(void)
{
	framebuffer[3] = '.';
}

void clear_dot(void)
{
	framebuffer[3] = 255;
}

void set_eeprom_saved_dot(void)
{
	framebuffer[5] = '.';
}

void clear_eeprom_saved_dot(void)
{
	framebuffer[5] = 255;
}

void display_number(uint16_t number)
{

	uint16_t temp1 = 0;
	uint16_t temp2 = 0;

	uint8_t dig0 = 0;
	uint8_t dig1 = 0;
	uint8_t dig2 = 0;

	if (number == 9999) {
		dig0 = 'N';
		dig1 = 'A';
		dig2 = 'F';
	} else if (number == 6666) {
		dig0 = '-';
		dig1 = '-';
		dig2 = '-';
	} else {

		temp1 = number - (number / 100) * 100;
		temp2 = temp1 - (temp1 / 10) * 10;

		dig0 = (uint8_t) (temp2);
		dig1 = (uint8_t) ((temp1 - temp2) / 10);
		dig2 = (uint8_t) ((number - temp1) / 100);
	}

	framebuffer[0] = dig0;
	framebuffer[1] = dig1;
	framebuffer[2] = dig2;

}

void display_char(uint8_t digit, uint8_t character)
{
	// clear digit-bits in PORTB - all digits OFF (set LOW, as current source)
	DIG0_OFF;
	DIG1_OFF;
	DIG2_OFF;
	// all segments OFF (set HIGH, as current sinks)
	SEGS_OFF;

	switch (digit) {
	case 0:
		DIG0_ON;	// turn on digit #0 (from right)
		break;
	case 1:
		DIG1_ON;	// #1
		break;
	case 2:
		DIG2_ON;	// #2
		break;
	case 3:
		DIG0_ON;	// #0 for the dot
		break;
	case 4:
		DIG1_ON;	// #1 for the dot
		break;
	case 5:
		DIG2_ON;	// #2 for the dot
		break;

	default:
		break;
	}

	switch (character) {
	case 0:
		PORTD = ~0xAF;	// activate segments for displaying a '0'
		break;
	case 1:
		PORTD = ~0xA0;	// '1'
		break;
	case 2:
		PORTD = ~0xC7;	// '2'
		break;
	case 3:
		PORTD = ~0xE5;	// '3'
		break;
	case 4:
		PORTD = ~0xE8;	// '4'
		break;
	case 5:
		PORTD = ~0x6D;	// '5'              
		break;
	case 6:
		PORTD = ~0x6F;	// '6'              
		break;
	case 7:
		PORTD = ~0xA1;	// '7'              
		break;
	case 8:
		PORTD = ~0xEF;	// '8'              
		break;
	case 9:
		PORTD = ~0xE9;	// '9'              
		break;
	case '-':
		PORTD = ~0x40;	// '-'              
		break;
	case 'o':
		PORTD = ~0x66;	// 'o'
		break;
	case '.':
		PORTD = ~0x10;	// '.'
		break;
	case 'F':
		PORTD = ~0x4B;	// 'F'
		break;
	case 'A':
		PORTD = ~0xEB;	// 'A'
		break;
	case 'N':
		PORTD = ~0xAB;	// 'N'
		break;
	case 255:
		PORTD = 0xFF;	// segments OFF
		break;
	default:
		PORTD = ~0x10;	// '.'              
		break;
	}
}

void segm_test(void)
{
	uint8_t ctr;

	PORTB |= (_BV(PB0) | _BV(PB6) | _BV(PB7));

	for (ctr = 0; ctr <= 7; ctr++) {
		SEGS_OFF;
		PORTD &= ~_BV(ctr);
		delay(200);
	}
}

void char_test(void)
{
	uint16_t ctr;

	for (ctr = 0; ctr <= 255; ctr++) {
		display_char(1, ctr);
		delay(100);
	}

	for (ctr = 0; ctr <= 255; ctr++) {
		display_char(2, ctr);
		delay(10);
	}

	for (ctr = 0; ctr <= 255; ctr++) {
		display_char(3, ctr);
		delay(10);
	}
}

void fan_test(void)
{
	FAN_ON;
	delay(2000);
	FAN_OFF;
}

void setup_timer1_ctc(void)
{
	// ATmega168 running at 8MHz internal RC oscillator
	// Timer1 (16bit) Settings:
	// prescaler (frequency divider) values:   CS12    CS11   CS10
	//                                           0       0      0    stopped
	//                                           0       0      1      /1  
	//                                           0       1      0      /8  
	//                                           0       1      1      /64
	//                                           1       0      0      /256 
	//                                           1       0      1      /1024
	//                                           1       1      0      external clock on T1 pin, falling edge
	//                                           1       1      1      external clock on T1 pin, rising edge
	//
	uint8_t _sreg = SREG;	/* save SREG */
	cli();			/* disable all interrupts while messing with the register setup */

	/* set prescaler to 8 */
	TCCR1B |= (_BV(CS11));
	TCCR1B &= ~(_BV(CS11) | _BV(CS12));

	/* set WGM mode 4: CTC using OCR1A */
	TCCR1A &= ~(_BV(WGM10) | _BV(WGM11));
	TCCR1B |= _BV(WGM12);
	TCCR1B &= ~_BV(WGM13);

	/* normal operation - disconnect PWM pins */
	TCCR1A &= ~(_BV(COM1A1) | _BV(COM1A0) | _BV(COM1B1) | _BV(COM1B0));

	/* set top value for TCNT1 */
	OCR1A = 8192;

	/* enable COMPA isr */
	TIMSK1 |= _BV(OCIE1A);

	/* restore SREG with global interrupt flag */
	SREG = _sreg;
}

ISR(TIMER1_COMPA_vect)
{
	static uint8_t digit = 0;

	DIG0_OFF;
	DIG1_OFF;
	DIG2_OFF;

	display_char(digit, framebuffer[digit]);
	digit++;

	if (digit == 6) {
		digit = 0;
	}
}
