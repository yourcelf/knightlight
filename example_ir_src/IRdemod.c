//
//
// IRdemod.c
// 
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h> //NG


#define tx_pin          PA5 // transmit pin
#define bit_delay_time  104 // bit delay, 1/9600 in usec
#define char_delay_time 1   // char delay time, in ms

#define bit_delay()         _delay_us(bit_delay_time)   // RS232 bit delay
#define char_delay()        _delay_ms(char_delay_time)  // RS232 char delay
#define output(pin)         (DDRA |= byte(pin))         // set pin for output
#define set(pin)            (PORTA |= byte(pin))        // set pin in PORTA
#define clear(pin)          (PORTA &= ~(byte(pin)))     // clear pin in PORTA
#define byte(bit)           (1 << bit)                  // byte with bit set
#define bit_test(byte,bit)  (byte & (1 << bit))         // test for bit set


// Input: 38kHz pulses on ICP PIN
// This program determines the time between 38kHz pulses

unsigned int new_TS, old_TS, timeInt, timeInt_ms, startbit, stopbit;
unsigned int freq = 20000000; //20MHz
unsigned int counter = 0;

unsigned char IRbyte = 0; //IR information

char message[]  PROGMEM = "Hello World!\n"; // hello-world message
char startmsg[] PROGMEM = "S"; // accuracy test
char endmsg[]   PROGMEM = "E"; // accuracy test
char errormsg[] PROGMEM = "B"; // accuracy test

void put_char(char txchar) {
   //
   // put the character in txchar
   // assumes no line driver (doesn't invert bits)
   //
   // start bit
   //
   set(tx_pin);
   bit_delay();
   //
   // data bits
   //
   if bit_test(txchar,0)
      clear(tx_pin);
   else
      set(tx_pin);
   bit_delay();
   if bit_test(txchar,1)
      clear(tx_pin);
   else
      set(tx_pin);
   bit_delay();
   if bit_test(txchar,2)
      clear(tx_pin);
   else
      set(tx_pin);
   bit_delay();
   if bit_test(txchar,3)
      clear(tx_pin);
   else
      set(tx_pin);
   bit_delay();
   if bit_test(txchar,4)
      clear(tx_pin);
   else
      set(tx_pin);
   bit_delay();
   if bit_test(txchar,5)
      clear(tx_pin);
   else
      set(tx_pin);
   bit_delay();
   if bit_test(txchar,6)
      clear(tx_pin);
   else
      set(tx_pin);
   bit_delay();
   if bit_test(txchar,7)
      clear(tx_pin);
   else
      set(tx_pin);
   bit_delay();
   //
   // stop bit
   //
   clear(tx_pin);
   bit_delay();
   }


void print_string(char *str) {
   //
   // print the null-terminated program memory string str
   //
   while (pgm_read_byte(str) != 0x00)
      put_char(pgm_read_byte(str++));
      char_delay();
   }
   

ISR(TIMER1_CAPT_vect)
{
   
   new_TS  = ICR1;
   timeInt = new_TS-old_TS;
   old_TS  = new_TS;
   
   //
   // decode IR signal (assume signal is a sony byte
   //
   
   if ((timeInt >= 56000)&&(timeInt <= 64000)) // START BIT, 3 ms = 60000
   {
      counter = 9;
	  PORTA = ((1 << PA0)|(1 << PA1)); // Turn on the LED
	  print_string(startmsg);
   }
   
   else if ((timeInt >= 20000)&&(timeInt <= 28000)) // 1.2 ms = 24000
   {
      IRbyte |= (0 << (counter-2)); // shift 0 into reg at bit (counter-2)
	  counter = counter-1;
	  put_char(0);
	  PORTA = ((0 << PA0)|(0 << PA1)); // Turn off the LED
   }
   
   else if ((timeInt >= 32000)&&(timeInt <= 40000)) // 1.8 ms = 36000
   {
      IRbyte |= (1 << (counter-2)); // shift 1 into reg at bit (counter-2)
	  counter = counter-1;
	  put_char(1);
	  PORTA = ((1 << PA0)|(1 << PA1)); // Turn on the LED
   }
   
   else
   {
      counter = 0;
	  PORTA = ((0 << 0)|(0 << 1)); // Turn off the LED
	  print_string(errormsg);
   }
   
   if (counter == 1)
   {
      print_string(endmsg); //accuracy test
      put_char(IRbyte);     // send to computer via serial communication (later via bluetooth)
	  print_string(endmsg); //accuracy test
	  
	  counter = 0;
	  PORTA = ((0 << PA0)|(0 << PA1)); // Turn off the LED
   }
	
}



int main(void)
{   
   
   DDRA |= ((1 << DDA0)|(1 << DDA1)); // Set LED as output
   
   DDRA |= (0 << DDA7); // Set ICP pin (PA7) as input
                         // "If DDxn is written logic zero, Pxn is configured as an input pin." 
   
   TCCR1B |= (1 << CS10); // Start timer at Fcpu/1 (timer prescaler 1)
   
   TCCR1B |= (1<<ICES1); // Capture time on rising edge
   TCCR1B |= (1<<ICNC1); // Enable Input noise canceller
   
   TIMSK1 |= (1<<ICIE1); //Enable Input Capture Interrupt
   
   output(tx_pin); // init serial output
   clear(tx_pin);  // init serial output
   
   sei();//enable global interrupt
   
   for (;;)
   {

   }
}


