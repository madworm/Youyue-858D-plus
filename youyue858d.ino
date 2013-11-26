/*
 * This is a custom firmware for my 'Youyue 858D+' hot-air sodlering station.
 * It may or may not be useful to you, always double check if you use it.
 *
 * PCB version: 858D V4.1
 * Date code:   20130421
 *
 * Other identifiers (see images)
 *
 * 2013 - Robert Spitzenpfeil
 *
 * Licence: GNU GPL v2
 *
 */

/*
 * PC5: FAN-speed (A5 in Arduino lingo)
 * PC3: TIP122.base --> FAN (OK)
 * PC0: ADC <-- amplif. thermo couple voltage (A0 in Arduino lingo)
 * #21: AREF <--- about 2.5V as analog reference for ADC
 * PB1: opto-triac driver (OK) !! THIS IS DANGEROUS TO USE !!
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

#define SHOW_SETPOINT_TIMEOUT 4000UL
#define BUTTON_LOCKOUT_TIME 50UL

#define FAN_SPEED_MIN 145
#define FAN_SPEED_MAX 347
#define FAN_SPEED_AVERAGES 500
#define SHOW_FAN_SPEED_TIMEOUT 1000UL

#define DUTY_CYCLE_INC_DELAY 150
#define DUTY_CYCLE_DEC_DELAY 50

#define TEMPERATURE_AVERAGES 100

uint16_t temperature_accu = 0;
uint16_t temperature_average = 0;
uint16_t temperature_setpoint = 0;
uint32_t button_input_time = 0;

uint32_t fan_speed_change_time = 0;
uint8_t fan_speed_average = 0;
uint8_t fan_speed_average_previous = 0;
uint16_t fan_speed_accu = 0;

uint32_t last_duty_cycle_inc_time = 0;
uint32_t last_duty_cycle_dec_time = 0;

uint8_t framebuffer[3] = { 0x00, 0x00, 0x00 };

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

	while (1) {
		HEATER_OFF;

		static uint8_t heater_ctr = 0;
		static uint8_t heater_duty_cycle = 0;

		if (REEDSW_OPEN) {
			// !! DANGER !!
			FAN_ON;

			if ((temperature_average < temperature_setpoint)
			    && ((millis() - last_duty_cycle_inc_time) >
				DUTY_CYCLE_INC_DELAY)) {
				if (heater_duty_cycle < 255) {
					heater_duty_cycle++;
					last_duty_cycle_inc_time = millis();
				}
			} else if ((temperature_average > temperature_setpoint)
				   && ((millis() - last_duty_cycle_dec_time) >
				       DUTY_CYCLE_DEC_DELAY)) {
				if (heater_duty_cycle > 0) {
					heater_duty_cycle--;
					last_duty_cycle_dec_time = millis();
				}
			}

			if (heater_ctr < heater_duty_cycle) {
				HEATER_ON;
			} else {
				HEATER_OFF;
			}

			heater_ctr++;
			if (heater_ctr == 255) {
				heater_ctr = 0;
			}

		} else {
			HEATER_OFF;
		}

		static uint16_t temp_avg_ctr = 0;
		static uint16_t fan_speed_avg_ctr = 0;

		temperature_accu += analogRead(A0);
		temp_avg_ctr++;

		if (temp_avg_ctr == TEMPERATURE_AVERAGES) {
			temperature_average =
			    temperature_accu / TEMPERATURE_AVERAGES;
			temperature_accu = 0;
			temp_avg_ctr = 0;
		}

		fan_speed_accu +=
		    map(analogRead(A5), FAN_SPEED_MIN, FAN_SPEED_MAX, 0, 100);
		fan_speed_avg_ctr++;

		if (fan_speed_avg_ctr == FAN_SPEED_AVERAGES) {
			fan_speed_average = fan_speed_accu / FAN_SPEED_AVERAGES;
			fan_speed_accu = 0;
			fan_speed_avg_ctr = 0;
		}

		if (fan_speed_average != fan_speed_average_previous) {
			fan_speed_change_time = millis();
			fan_speed_average_previous = fan_speed_average;
		}

		if (temperature_average > 55) {
			FAN_ON;
		} else if (REEDSW_CLOSED && (temperature_average < 40)) {
			FAN_OFF;
		} else if (REEDSW_OPEN) {
			FAN_ON;
		}

		if ((millis() - button_input_time) > BUTTON_LOCKOUT_TIME) {

			if (SW0_PRESSED && SW1_PRESSED) {
				HEATER_ON;
				button_input_time = millis();
			} else if (SW0_PRESSED
				   && (temperature_setpoint < 500UL)) {
				temperature_setpoint++;
				button_input_time = millis();
			} else if (SW1_PRESSED && (temperature_setpoint > 0)) {
				temperature_setpoint--;
				button_input_time = millis();
			}

		}

		if ((millis() - button_input_time) < SHOW_SETPOINT_TIMEOUT) {
			display_number(temperature_setpoint);	// show temperature setpoint
		} else
		    if (((millis() - fan_speed_change_time) <
			 SHOW_FAN_SPEED_TIMEOUT) && FAN_IS_ON) {
			display_number(fan_speed_average);
		} else {
			if (temperature_average < 35) {
				display_number(6666);
			} else {
				display_number(temperature_average);
			}
		}

	}

}

void display_number(uint16_t number)
{

	uint16_t temp1 = 0;
	uint16_t temp2 = 0;

	uint8_t dig0 = 0;
	uint8_t dig1 = 0;
	uint8_t dig2 = 0;

	if (number == 6666) {
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
		DIG0_ON;	// turn on digit #1
		break;
	case 1:
		DIG1_ON;	// #2
		break;
	case 2:
		DIG2_ON;	// #3
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
		PORTD = ~0x6E;	// '6'              
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

	if (digit == 3) {
		digit = 0;
	}
}
