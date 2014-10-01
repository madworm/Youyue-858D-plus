/*
 * This is a custom firmware for my 'Youyue 858D+' hot-air soldering station.
 * It may or may not be useful to you, always double check if you use it.
 *
 * V1.32
 *
 * 2014 - Robert Spitzenpfeil
 *
 * License: GNU GPL v2
 *
 *
 * Developed for / tested on:
 * --------------------------
 *
 * PCB version: 858D V4.1
 * Date code:   20130421
 *
 * 
 * Reported to work with (I did not test these myself):
 * ----------------------------------------------------
 *
 * PCB version: 858D V4.3
 * Date code:   20130529
 * HW mods:     not tested!
 *
 * PCB version: 858D V4.10
 * Date code:	20140112
 * HW mods:	not tested!
 *
 */

/*
 *  Make sure to read and understand '/Docs/modes_of_operation.txt'
 *
 *
 * If you compile and upload using the Arduino-IDE + ISP, choose one of these target boards:
 *
 * LilyPad Arduino w/ ATmega168 (TESTED - WORKS)
 * LilyPad Arduino w/ ATmega328 (TESTED - WORKS)
 *
 * This will make sure suitable FUSE settings are used (8MHz RC-oscillator...)
 *
 *
 * DO NOT USE A BOOTLOADER WITH THE WATCHDOG TIMER
 *
 * ISP CODE UPLOAD ONLY
 * 
 * Change options in the .h file
 *
 */

#define FW_MAJOR_V 1
#define FW_MINOR_V_A 3
#define FW_MINOR_V_B 2

/*
 * PC5: FAN-speed (A5 in Arduino lingo) (OK)
 * PC3: TIP122.base --> FAN (OK)
 * PC2: fan-current-sense mod (OPTIONAL) - see Docs folder
 * PC0: ADC <-- amplif. thermo couple voltage (A0 in Arduino lingo) (OK)
 * #21: AREF <--- about 2.5V as analog reference for ADC
 * PB1: opto-triac driver !! THIS IS DANGEROUS TO USE !! (OK)
 *
 * PB0: 7-seg digit 0 [common Anode] (OK) 
 * PB7: 7-seg digit 1 [common Anode] (OK)
 * PB6: 7-seg digit 2 [common Anode] (OK)
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
 * PB4: reed switch (wand cradle sensor) (OK)
 *
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <EEPROM.h>
#include "youyue858d.h"

uint8_t framebuffer[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };	// dig0, dig1, dig2, dot0, dot1, dot2 - couting starts from right side

CPARAM p_gain = { 0, 999, P_GAIN_DEFAULT, P_GAIN_DEFAULT, 2, 3 };	// min, max, default, value, eep_addr_high, eep_addr_low
CPARAM i_gain = { 0, 999, I_GAIN_DEFAULT, I_GAIN_DEFAULT, 4, 5 };
CPARAM d_gain = { 0, 999, D_GAIN_DEFAULT, D_GAIN_DEFAULT, 6, 7 };
CPARAM i_thresh = { 0, 100, I_THRESH_DEFAULT, I_THRESH_DEFAULT, 8, 9 };
CPARAM temp_offset_corr = { -100, 100, TEMP_OFFSET_CORR_DEFAULT, TEMP_OFFSET_CORR_DEFAULT, 10, 11 };
CPARAM temp_setpoint = { 50, 500, TEMP_SETPOINT_DEFAULT, TEMP_SETPOINT_DEFAULT, 12, 13 };
CPARAM temp_averages = { 100, 999, TEMP_AVERAGES_DEFAULT, TEMP_AVERAGES_DEFAULT, 14, 15 };
CPARAM slp_timeout = { 0, 30, SLP_TIMEOUT_DEFAULT, SLP_TIMEOUT_DEFAULT, 16, 17 };

#ifdef CURRENT_SENSE_MOD
CPARAM fan_current_min = { 0, 999, FAN_CURRENT_MIN_DEFAULT, FAN_CURRENT_MIN_DEFAULT, 22, 23 };
CPARAM fan_current_max = { 0, 999, FAN_CURRENT_MAX_DEFAULT, FAN_CURRENT_MAX_DEFAULT, 24, 25 };
#else
CPARAM fan_speed_min = { 120, 180, FAN_SPEED_MIN_DEFAULT, FAN_SPEED_MIN_DEFAULT, 18, 19 };
CPARAM fan_speed_max = { 300, 400, FAN_SPEED_MAX_DEFAULT, FAN_SPEED_MAX_DEFAULT, 20, 21 };
#endif

int main(void)
{
	init();			// make sure the Arduino-specific stuff is up and running (timers... see 'wiring.c')

	setup_858D();

#ifdef USE_WATCHDOG
	if (_mcusr & _BV(WDRF)) {
		// there was a watchdog reset - should never ever happen
		HEATER_OFF;
		FAN_ON;
		while (1) {
			display_string("RST");
			delay(1000);
			clear_display();
			delay(1000);
		}
	}
#endif

	show_firmware_version();
	fan_test();
#ifdef USE_WATCHDOG
	watchdog_on();
#endif

#ifdef DEBUG
	Serial.begin(2400);
	Serial.println("\nRESET");
#endif

	while (1) {
#ifdef DEBUG
		int32_t start_time = micros();
#endif
		static int16_t temp_inst = 0;
		static int32_t temp_accu = 0;
		static int16_t temp_average = 0;
		static int16_t temp_average_previous = 0;

		static int32_t button_input_time = 0;

		static int16_t heater_ctr = 0;
		static int16_t heater_duty_cycle = 0;
		static int16_t error = 0;
		static int32_t error_accu = 0;
		static int16_t velocity = 0;
		static float PID_drive = 0;

		static int16_t button_counter = 0;

		static uint8_t temp_setpoint_saved = 1;
		static int32_t temp_setpoint_saved_time = 0;

		static uint32_t heater_start_time = 0;

		temp_inst = analogRead(A0) + temp_offset_corr.value;	// approx. temp in °C

		if (temp_inst < 0) {
			temp_inst = 0;
		}

		if (REEDSW_OPEN && (temp_setpoint.value >= temp_setpoint.value_min)
		    && (temp_average < MAX_TEMP_ERR) && ((millis() - heater_start_time) < ((uint32_t) (slp_timeout.value) * 60 * 1000))) {

			FAN_ON;

			error = temp_setpoint.value - temp_average;
			velocity = temp_average_previous - temp_average;

			if (abs(error) < i_thresh.value) {
				// if close enough to target temperature use PID control
				error_accu += error;
			} else {
				// otherwise only use PD control (avoids issues with error_accu growing too large
				error_accu = 0;
			}

			PID_drive =
			    error * (p_gain.value / P_GAIN_SCALING) + error_accu * (i_gain.value / I_GAIN_SCALING) +
			    velocity * (d_gain.value / D_GAIN_SCALING);

			heater_duty_cycle = (int16_t) (PID_drive);

			if (heater_duty_cycle > HEATER_DUTY_CYCLE_MAX) {
				heater_duty_cycle = HEATER_DUTY_CYCLE_MAX;
			}

			if (heater_duty_cycle < 0) {
				heater_duty_cycle = 0;
			}

			if (heater_ctr < heater_duty_cycle) {
				set_dot();
				HEATER_ON;
			} else {
				HEATER_OFF;
				clear_dot();
			}

			heater_ctr++;
			if (heater_ctr == PWM_CYCLES) {
				heater_ctr = 0;
			}

		} else if (REEDSW_CLOSED) {
			HEATER_OFF;
			heater_start_time = millis();
			clear_dot();
		} else {
			HEATER_OFF;
			clear_dot();
		}

		static uint16_t temp_avg_ctr = 0;

		temp_accu += temp_inst;
		temp_avg_ctr++;

		if (temp_avg_ctr == (uint16_t) (temp_averages.value)) {
			temp_average_previous = temp_average;
			temp_average = temp_accu / temp_averages.value;
			temp_accu = 0;
			temp_avg_ctr = 0;
		}

		if (temp_average >= FAN_ON_TEMP) {
			FAN_ON;
		} else if (REEDSW_CLOSED && (temp_average <= FAN_OFF_TEMP)) {
			FAN_OFF;
		} else if (REEDSW_OPEN) {
			FAN_ON;
		}

		if (SW0_PRESSED && SW1_PRESSED) {
			HEATER_OFF;
#ifdef USE_WATCHDOG
			watchdog_off();
#endif
			change_config_parameter(&p_gain, "P");
			change_config_parameter(&i_gain, "I");
			change_config_parameter(&d_gain, "D");
			change_config_parameter(&i_thresh, "ITH");
			change_config_parameter(&temp_offset_corr, "TOF");
			change_config_parameter(&temp_averages, "AVG");
			change_config_parameter(&slp_timeout, "SLP");
#ifdef CURRENT_SENSE_MOD
			change_config_parameter(&fan_current_min, "FCL");
			change_config_parameter(&fan_current_max, "FCH");
#else
			change_config_parameter(&fan_speed_min, "FSL");
			change_config_parameter(&fan_speed_max, "FSH");
#endif
#ifdef USE_WATCHDOG
			watchdog_on();
#endif
		} else if (SW0_PRESSED) {
			button_input_time = millis();
			button_counter++;

			if (button_counter == 200) {

				if (temp_setpoint.value < temp_setpoint.value_max) {
					temp_setpoint.value++;
					temp_setpoint_saved = 0;
				}

			}

			if (button_counter == 600) {

				if (temp_setpoint.value < (temp_setpoint.value_max - 10)) {
					temp_setpoint.value += 10;
					temp_setpoint_saved = 0;
				}

				button_counter = 201;

			}

		} else if (SW1_PRESSED) {
			button_input_time = millis();
			button_counter++;

			if (button_counter == 200) {

				if (temp_setpoint.value > temp_setpoint.value_min) {
					temp_setpoint.value--;
					temp_setpoint_saved = 0;
				}

			}

			if (button_counter == 600) {

				if (temp_setpoint.value > (temp_setpoint.value_min + 10)) {
					temp_setpoint.value -= 10;
					temp_setpoint_saved = 0;
				}

				button_counter = 201;

			}

		} else {
			button_counter = 0;
		}

		if ((millis() - button_input_time) < SHOW_SETPOINT_TIMEOUT) {
			display_number(temp_setpoint.value);	// show temperature setpoint
		} else {
			if (temp_setpoint_saved == 0) {
				set_eeprom_saved_dot();
				eep_save(&temp_setpoint);
				temp_setpoint_saved_time = millis();
				temp_setpoint_saved = 1;
			} else if (temp_average <= SAFE_TO_TOUCH_TEMP) {
				display_string("---");
			} else if (temp_average >= MAX_TEMP_ERR) {
				// something might have gone terribly wrong
				HEATER_OFF;
				FAN_ON;
#ifdef USE_WATCHDOG
				watchdog_off();
#endif
				while (1) {
					// stay here until the power is cycled
					// make sure the user notices the error by blinking "FAN"
					// and don't resume operation if the error goes away on its own
					//
					// possible reasons to be here:
					//
					// * wand is not connected (false temperature reading)
					// * thermo couple has failed
					// * true over-temperature condition
					//
					display_string("FAN");
					delay(1000);
					clear_display();
					delay(1000);
				}
			} else if (abs((int16_t) (temp_average) - (int16_t) (temp_setpoint.value)) < TEMP_REACHED_MARGIN) {
				display_number(temp_setpoint.value);	// avoid showing insignificant fluctuations on the display (annoying)
			} else {
				display_number(temp_average);
				//display_number(temp_inst);
			}
		}

		if ((millis() - temp_setpoint_saved_time) > 500) {
			clear_eeprom_saved_dot();
		}
#if defined(WATCHDOG_TEST) && defined(USE_WATCHDOG)
		// watchdog test
		if (temp_average > 100) {
			delay(150);
		}
#endif

#ifdef USE_WATCHDOG
		wdt_reset();
#endif

#ifdef DEBUG
		int32_t stop_time = micros();
		Serial.println(stop_time - start_time);
#endif
	}

}

void setup_858D(void)
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

#ifdef CURRENT_SENSE_MOD
	DDRC &= ~_BV(PC2);	// set as input
#endif

	setup_timer1_ctc();	// needed for background display refresh

	analogReference(EXTERNAL);	// use external 2.5V as ADC reference voltage (VCC / 2)

	if (EEPROM.read(0) != 0x22) {
		// check if the firmware was just flashed and the EEPROM is therefore empty
		// assumption: full chip erase with ISP programmer (preserve eeprom fuse NOT set!)
		// if so, restore default parameter values & write a 'hint' to address 0
		restore_default_conf();
		EEPROM.write(0, 0x22);
	}

	if (SW0_PRESSED && SW1_PRESSED) {
		restore_default_conf();
	}

	eep_load(&p_gain);
	eep_load(&i_gain);
	eep_load(&d_gain);
	eep_load(&i_thresh);
	eep_load(&temp_offset_corr);
	eep_load(&temp_setpoint);
	eep_load(&temp_averages);
	eep_load(&slp_timeout);
#ifdef CURRENT_SENSE_MOD
	eep_load(&fan_current_min);
	eep_load(&fan_current_max);
#else
	eep_load(&fan_speed_min);
	eep_load(&fan_speed_max);
#endif
}

void clear_display(void)
{
	framebuffer[0] = 255;	// 255 --> 7-seg character off
	framebuffer[1] = 255;
	framebuffer[2] = 255;
	framebuffer[3] = 255;
	framebuffer[4] = 255;
	framebuffer[5] = 255;
}

void display_string(const char *string)
{
	clear_display();

	uint8_t ctr;

	for (ctr = 0; ctr <= 2; ctr++) {
		// read the first 3 characters of the string
		if (string[ctr] == '\0') {
			break;
		} else {
			framebuffer[2 - ctr] = string[ctr];
		}
	}
}

void change_config_parameter(CPARAM * param, const char *string)
{
	display_string(string);
	delay(750);		// let the user read what is shown

	uint8_t loop = 1;
	uint16_t button_counter = 0;

	while (loop == 1) {

		if (SW0_PRESSED && SW1_PRESSED) {
			loop = 0;
		} else if (SW0_PRESSED) {
			button_counter++;

			if (button_counter == 500) {

				if (param->value < param->value_max) {
					param->value++;
				}
			}

			if (button_counter == 2000) {

				if (param->value < param->value_max - 10) {
					param->value += 10;
				}
				button_counter = 501;

			}

		} else if (SW1_PRESSED) {
			button_counter++;

			if (button_counter == 500) {

				if (param->value > param->value_min) {
					param->value--;
				}
			}

			if (button_counter == 2000) {

				if (param->value > param->value_min + 10) {
					param->value -= 10;
				}

				button_counter = 501;

			}

		} else {
			button_counter = 0;
		}
		display_number(param->value);
	}
	set_eeprom_saved_dot();
	eep_save(param);
	delay(1000);
	clear_eeprom_saved_dot();
}

void eep_save(CPARAM * param)
{
	// make sure NOT to save invalid parameter values
	if ((param->value >= param->value_min) && (param->value <= param->value_max)) {
		// nothing to do
	} else {
		// reset to sensible minimum
		param->value = param->value_default;
	}
	EEPROM.write(param->eep_addr_high, highByte(param->value));
	EEPROM.write(param->eep_addr_low, lowByte(param->value));
}

void eep_load(CPARAM * param)
{
	int16_t tmp = (EEPROM.read(param->eep_addr_high) << 8) | EEPROM.read(param->eep_addr_low);

	// make sure NOT to restore invalid parameter values
	if ((tmp >= param->value_min) && (tmp <= param->value_max)) {
		// the value was good, so we use it
		param->value = tmp;
	} else {
		// reset to sensible value
		param->value = param->value_default;
	}
}

void restore_default_conf(void)
{
	p_gain.value = p_gain.value_default;
	i_gain.value = i_gain.value_default;
	d_gain.value = d_gain.value_default;
	i_thresh.value = i_thresh.value_default;
	temp_offset_corr.value = temp_offset_corr.value_default;
	temp_setpoint.value = temp_setpoint.value_default;
	temp_averages.value = temp_averages.value_default;
	slp_timeout.value = slp_timeout.value_default;
#ifdef CURRENT_SENSE_MOD
	fan_current_min.value = fan_current_min.value_default;
	fan_current_max.value = fan_current_max.value_default;
#else
	fan_speed_min.value = fan_speed_min.value_default;
	fan_speed_max.value = fan_speed_max.value_default;
#endif

	eep_save(&p_gain);
	eep_save(&i_gain);
	eep_save(&d_gain);
	eep_save(&i_thresh);
	eep_save(&temp_offset_corr);
	eep_save(&temp_setpoint);
	eep_save(&temp_averages);
	eep_save(&slp_timeout);
#ifdef CURRENT_SENSE_MOD
	eep_save(&fan_current_min);
	eep_save(&fan_current_max);
#else
	eep_save(&fan_speed_min);
	eep_save(&fan_speed_max);
#endif
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

void display_number(int16_t number)
{
	if (number < 0) {
		framebuffer[3] = '.';
		framebuffer[4] = '.';
		framebuffer[5] = '.';
		number = -number;
	} else {
		// don't clear framebuffer[3], as this is the heater-indicator
		framebuffer[4] = 255;
		framebuffer[5] = 255;
	}

	framebuffer[0] = (uint8_t) (number % 10);
	number /= 10;
	framebuffer[1] = (uint8_t) (number % 10);
	number /= 10;
	framebuffer[2] = (uint8_t) (number % 10);
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
		PORTD = (uint8_t) (~0xAF);	// activate segments for displaying a '0'
		break;
	case 1:
		PORTD = (uint8_t) (~0xA0);	// '1'
		break;
	case 2:
		PORTD = (uint8_t) (~0xC7);	// '2'
		break;
	case 3:
		PORTD = (uint8_t) (~0xE5);	// '3'
		break;
	case 4:
		PORTD = (uint8_t) (~0xE8);	// '4'
		break;
	case 5:
		PORTD = (uint8_t) (~0x6D);	// '5'
		break;
	case 6:
		PORTD = (uint8_t) (~0x6F);	// '6'
		break;
	case 7:
		PORTD = (uint8_t) (~0xA1);	// '7'
		break;
	case 8:
		PORTD = (uint8_t) (~0xEF);	// '8'
		break;
	case 9:
		PORTD = (uint8_t) (~0xE9);	// '9'
		break;
	case '-':
		PORTD = (uint8_t) (~0x40);	// '-'
		break;
	case 'O':
		PORTD = (uint8_t) (~0x66);	// 'o'
		break;
	case '.':
		PORTD = (uint8_t) (~0x10);	// '.'
		break;
	case 'C':
		PORTD = (uint8_t) (~0x0F);	// 'C'
		break;
	case 'F':
		PORTD = (uint8_t) (~0x4B);	// 'F'
		break;
	case 'A':
		PORTD = (uint8_t) (~0xEB);	// 'A'
		break;
	case 'N':
		PORTD = (uint8_t) (~0xAB);	// 'N'
		break;
	case 'P':
		PORTD = (uint8_t) (~0xCB);	// 'P'
		break;
	case 'I':
		PORTD = (uint8_t) (~0x20);	// 'i'
		break;
	case 'D':
		PORTD = (uint8_t) (~0xE6);	// 'd'
		break;
	case 'T':
		PORTD = (uint8_t) (~0x4E);	// 't'
		break;
	case 'H':
		PORTD = (uint8_t) (~0x6A);	// 'h'
		break;
	case 'R':
		PORTD = (uint8_t) (~0x42);	// 'r'
		break;
	case 'S':
		PORTD = (uint8_t) (~0x6D);	// 'S'
		break;
	case 'L':
		PORTD = (uint8_t) (~0x0E);	// 'L'
		break;
	case 'V':
		PORTD = (uint8_t) (~0x26);	// 'v'
		break;
	case 'G':
		PORTD = (uint8_t) (~0x6F);	// 'G'
		break;
	case 255:
		PORTD = (uint8_t) (0xFF);	// segments OFF
		break;
	default:
		PORTD = (uint8_t) (~0x10);	// '.'
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
		display_char(0, ctr);
		delay(10);
	}

	for (ctr = 0; ctr <= 255; ctr++) {
		display_char(1, ctr);
		delay(10);
	}

	for (ctr = 0; ctr <= 255; ctr++) {
		display_char(2, ctr);
		delay(10);
	}
}

#ifdef CURRENT_SENSE_MOD
void fan_test(void)
{
	HEATER_OFF;

	uint16_t fan_current;

	if (REEDSW_CLOSED) {

		FAN_ON;
		delay(3000);
		fan_current = analogRead(A2);

		/*
		   while(1) {
		   fan_current = analogRead(A2);
		   display_number(fan_current);  
		   }
		 */

		if ((fan_current < (uint16_t) (fan_current_min.value)) || (fan_current > (uint16_t) (fan_current_max.value))) {
			// the fan is not working as it should
			FAN_OFF;
			while (1) {
				display_string("FAN");
				delay(1000);
				clear_display();
				delay(1000);
			}
		}

		FAN_OFF;

	} else {
		// if the wand is not in the cradle when powered up, go into a safe mode
		// and display an error
		while (1) {
			display_string("FAN");
			delay(1000);
			clear_display();
			delay(1000);
		}
	}
}
#else
void fan_test(void)
{
	HEATER_OFF;

	uint16_t fan_speed;

	if (REEDSW_CLOSED) {

		FAN_ON;
		delay(3000);
		fan_speed = analogRead(A5);

		/*
		   while(1) {
		   fan_speed = analogRead(A5);
		   display_number(fan_speed);  
		   }
		 */

		if ((fan_speed < (uint16_t) (fan_speed_min.value)) || (fan_speed > (uint16_t) (fan_speed_max.value))) {
			// the fan is not working as it should
			FAN_OFF;
			while (1) {
				display_string("FAN");
				delay(1000);
				clear_display();
				delay(1000);
			}
		}

		FAN_OFF;

	} else {
		// if the wand is not in the cradle when powered up, go into a safe mode
		// and display an error
		while (1) {
			display_string("FAN");
			delay(1000);
			clear_display();
			delay(1000);
		}
	}
}
#endif

void show_firmware_version(void)
{
	framebuffer[0] = FW_MINOR_V_B;	// dig0
	framebuffer[1] = FW_MINOR_V_A;	// dig1
	framebuffer[2] = FW_MAJOR_V;	// dig2
	framebuffer[3] = 255;	// dig0.dot
	framebuffer[4] = 255;	// dig1.dot
	framebuffer[5] = '.';	// dig2.dot
	delay(2000);
	clear_display();
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

	/* set prescaler to 64 */
	TCCR1B |= (_BV(CS11) | _BV(CS10));
	TCCR1B &= ~(_BV(CS12));

	/* set WGM mode 4: CTC using OCR1A */
	TCCR1A &= ~(_BV(WGM10) | _BV(WGM11));
	TCCR1B |= _BV(WGM12);
	TCCR1B &= ~_BV(WGM13);

	/* normal operation - disconnect PWM pins */
	TCCR1A &= ~(_BV(COM1A1) | _BV(COM1A0) | _BV(COM1B1) | _BV(COM1B0));

	/* set top value for TCNT1 */
	OCR1A = 256;		// display refresh about 80Hz

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

#ifdef USE_WATCHDOG
void watchdog_off(void)
{
	wdt_reset();
	MCUSR &= ~_BV(WDRF);	// clear WDRF, as it overrides WDE-bit in WDTCSR-reg and leads to endless reset-loop (15ms timeout after wd-reset)
	wdt_disable();
}

void watchdog_on(void)
{
	wdt_reset();
	MCUSR &= ~_BV(WDRF);	// clear WDRF, as it overrides WDE-bit in WDTCSR-reg and leads to endless reset-loop (15ms timeout after wd-reset)
	wdt_enable(WDTO_120MS);
}

void watchdog_off_early(void)
{
	wdt_reset();
	_mcusr = MCUSR;
	MCUSR &= ~_BV(WDRF);	// clear WDRF, as it overrides WDE-bit in WDTCSR-reg and leads to endless reset-loop (15ms timeout after wd-reset)
	wdt_disable();
}
#endif
