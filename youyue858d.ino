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
 * PC3: TIP122.base --> FAN
 * PC0: ADC <-- amplif. thermo couple voltage
 * PB1: opto-triac driver
 *
 * PB0: 7-seg digit 1 (OK)
 * PB7: 7-seg digit 2 (OK)
 * PB6: 7-seg digit 3 (OK)
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
 * PB5: SW1 (button1)
 * PB2: SW2 (button2)
 * PB4: reed switch (wand craddle sensor)
 *
 */

#define FAN_OFF ( PORTC |= _BV(PC3) )
#define FAN_ON  ( PORTC &= ~_BV(PC3) )

#define TRIAC_ON ( PORTB &= ~_BV(PB1) )
#define TRIAC_OFF ( PORTB |= _BV(PB1) )

#define SW1_PRESSED ( !(PINB & _BV(PB5)) )
#define SW2_PRESSED ( !(PINB & _BV(PB2)) )
#define REEDSW_CLOSED ( !(PINB & _BV(PB4)) )

/*
x0 = 555;
x1 = x0 - (x0/100)*100; // 55;
x2 = x1 - (x1/10)*10; // 5;

d3 = (x0 - x1)/100;
d2 = (x1 - x2)/10;
d1 = x2;
*/

void setup(void)
{
	DDRB &= ~(_BV(PB5) | _BV(PB4) | _BV(PB2));	// set as inputs (switches)
	// DDRC |= _BV(PC3);	// set as output (FAN control)
	DDRD |= 0xFF;		// all as outputs (7-seg segments)
	DDRB |= (_BV(PB0) | _BV(PB6) | _BV(PB7));	// 7-seg digits 1,2,3
}

void loop(void)
{
  segm_test();
  //char_test();
}

void display_char(uint8_t digit, uint8_t character)
{
	// clear digit-bits in PORTB - all digits OFF (set LOW, as current source)
	PORTB &= ~(_BV(PB0) | _BV(PB6) | _BV(PB7));

	// all segments OFF (set HIGH, as current sinks)
	PORTD = 0xFF;

	switch (digit) {
	case 1:
		PORTB |= _BV(PB0);	// turn on digit #1
		break;

	case 2:
		PORTB |= _BV(PB7);	// #2
		break;

	case 3:
		PORTB |= _BV(PB6);	// #3
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
                PORTD = ~0x10; // '.'
                break;
                
        case 'F':
                PORTD = ~0x4B  ; // 'F'
                break;

	default:
		PORTD = ~0x10;	// '.'              
		break;
	}
}

void segm_test(void) {
  uint8_t ctr;
  
  PORTB |= (_BV(PB0) | _BV(PB6) | _BV(PB7));
  
  for(ctr=0; ctr<=7; ctr++) {
    PORTD = 0xFF;
    PORTD &= ~_BV(ctr);
    delay(200); 
  }  
}

void char_test(void) {
  uint16_t ctr;
  
  for(ctr=0; ctr <= 255; ctr++)
  {
    display_char(1,ctr);
    delay(100);
  }
  
  for(ctr=0; ctr <= 255; ctr++)
  {
    display_char(2,ctr);
    delay(10);
  }
  
  for(ctr=0; ctr <= 255; ctr++)
  {
    display_char(3,ctr);
    delay(10);
  }
}
