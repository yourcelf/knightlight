//
// This code sends out a byte of data every 10 seconds.
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void sendstartbit (void)
{
   sei();
   _delay_ms(0.2);
   cli();
   PORTA = ((0 << 0)|(0 << 1)); // pull LED low
   _delay_ms(2.8);
   sei();
   _delay_ms(0.2);
   cli();
}

void sendbit_0 (void)
{
   PORTA = ((0 << 0)|(0 << 1)); // pull LED low
   _delay_ms(1);
   sei();
   _delay_ms(0.2);
   cli();
}

void sendbit_1 (void)
{
   PORTA = ((0 << 0)|(0 << 1)); // pull LED low
   _delay_ms(1.6);
   sei();
   _delay_ms(0.2);
   cli();
}

int main (void)
{
   //DDRA   |= (1 << 0); // Set LED as output
   DDRA   |= ((1 << 0)|(1 << 1)); // Set LED as output
   
   TCCR1B |= (1 << WGM12); // Configure timer 1 for CTC mode

   TIMSK1 |= (1 << OCIE1A); // Enable CTC interrupt

   OCR1A   = 263; // Set CTC compare value to 38kHz at 20Mhz clock w/o prescaler

   TCCR1B |= (1 << CS10); // Start timer at Fcpu/1
   
   sei(); //  Enable global interrupts

   for (;;)
   {
      
	  // K = 01001011 75 4B
	  // Z = 01011010 90 5A
	  
	  sendstartbit(); //start bit
	  
	  // this is the correct order, send bit 7 first!
	  sendbit_0();    //bit 7
	  sendbit_1();    //bit 6
	  sendbit_0();    //bit 5
	  sendbit_1();    //bit 4
	  sendbit_1();    //bit 3
	  sendbit_0();    //bit 2
	  sendbit_1();    //bit 1
	  sendbit_0();    //bit 0

	  
	  //wait between sending bytes
	  PORTA = ((0 << 0)|(0 << 1)); // pull LED low
	  _delay_ms(10);
	  
   }
}

ISR(TIM1_COMPA_vect)
{
   //PORTA ^= (1 << 0); // Toggle the LED
   PORTA ^= ((1 << 0)|(1 << 1)); // Toggle the LED
}
