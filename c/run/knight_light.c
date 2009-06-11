#ifndef UNIT_ID
#define UNIT_ID 0
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000
#include <util/delay.h>
#include <stdint.h>
#define RAND_MAX 25000
#include <stdlib.h>

// Higher is slower
#define MAX_DUTY 180
// Lower is brighter
#define DUTY_DIVISOR 2 

#define BLINKS_BEFORE_TIMEOUT 10

// in milliseconds
#define ZERO_DURATION .25
#define ONE_DURATION .5
#define TWO_DURATION .75
#define POST_SEND_DELAY .25
#define BIT_WIDTH_FUDGE_FACTOR .265
// delays in clock cycles
#define CLOCK_ZERO_DURATION ((ZERO_DURATION + BIT_WIDTH_FUDGE_FACTOR) / 1000. * F_CPU)
#define CLOCK_ONE_DURATION ((ONE_DURATION + BIT_WIDTH_FUDGE_FACTOR) / 1000. * F_CPU)
#define CLOCK_TWO_DURATION ((TWO_DURATION + BIT_WIDTH_FUDGE_FACTOR) / 1000. * F_CPU)
// percentage
#define TOLERANCE .3
#define DUR_IN_RANGE(dur, constant) ((dur > (int)(constant * (1 - TOLERANCE))) && (dur < (int)(constant * (1 + TOLERANCE))))

#define RED_ON      PORTA &= ~(1 << PA0);
#define RED_OFF     PORTA |= (1 << PA0);
#define GREEN_ON    PORTA &= ~(1 << PA1);
#define GREEN_OFF   PORTA |= (1 << PA1);
#define BLUE_ON     PORTA &= ~(1 << PA2);
#define BLUE_OFF    PORTA |= (1 << PA2);
#define ALL_OFF     RED_OFF; GREEN_OFF; BLUE_OFF;

// IR routines; works for all 3 board types
#define IR_ON       PORTA &= ~((1 << PA3) | (1 << PA4)); PORTA |= (1 << PA5);
#define IR_OFF      PORTA |= ((1 << PA3) | (1 << PA4)); PORTA &= ~(1 << PA5);

uint8_t i, total_duty; // loop vars
uint8_t pulse_count;
uint8_t timeout_count;
uint8_t green_count, red_count, blue_count, total_count;
uint16_t green_duty, red_duty, blue_duty;

// IR i/o
uint8_t ir_byte_out;
uint8_t ir_byte_in;

uint8_t receptive;

uint16_t new_timestamp, old_timestamp, time_interval;
uint16_t random_delay_1;
uint16_t random_delay_2;

// Function prototypes
void init(void);
void reset_stack_pointer(void);
void cycle(void);
void degrade(void);
void transmit_id(void);
void send_38k_pulse(void);
void byte_received(void);


int main(void) {
    init();
    reset_stack_pointer();
    return 0;
}
void reset_stack_pointer() {
    asm volatile("mov __tmp_reg__, r16\n"
                 "ldi r16, 0x1\n"
                 "out __SP_H__, r16\n"
                 "ldi r16, 0x00\n"
                 "out __SP_L__, r16\n" 
                 "mov r16, __tmp_reg__\n");
    for (timeout_count=BLINKS_BEFORE_TIMEOUT; timeout_count > 0; timeout_count--) {
        cycle();
    }
    for (;;);
}
void init(void) {
    // Init LED
    PORTA = 0xFF & ~(1 << PA5);
    DDRA = 0xFF;
    ALL_OFF;
    
    // Init input capture pin timer for IR receive
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1B |= (1 << CS10); // Start timer at Fcpu/1
    TCCR1B |= (1 << ICES1); // Capture timer on rising edge
    TCCR1B |= (1 << ICNC1); // Enable Input noise canceller
    TIMSK1 |= (1 << ICIE1); // Enable Input capture Interrupt
    new_timestamp = 0;
    old_timestamp = 0;
    time_interval = 0;

    red_count = 0;
    green_count = 0;
    blue_count = 0;
    total_count = 0;
    receptive = 0;
    degrade();

    sei();
}

void degrade(void) {
    if (red_count > 0) {
        red_count--;
        total_count--;
    }
    if (green_count > 0) {
        green_count--;
        total_count--;
    }
    if (blue_count > 0) {
        blue_count--;
        total_count--;
    }
    if (total_count == 0) {
#if UNIT_ID == 0
        red_count = 1;
#elif UNIT_ID == 1
        green_count = 1;
#elif UNIT_ID == 2
        blue_count = 1;
#endif
        total_count = red_count + blue_count + green_count;
    }

}
void shine(void) {
    if (total_count > 0) {
        red_duty = (total_duty * red_count) / (total_count * DUTY_DIVISOR);
        green_duty = (total_duty * green_count) / (total_count * DUTY_DIVISOR);
        blue_duty = (total_duty * blue_count) / (total_count * DUTY_DIVISOR);
        for (i = 0; i < MAX_DUTY; i++) {
            if (i < red_duty) {
                RED_ON;
            } else {
                RED_OFF;
            }
            if (i < green_duty) {
                GREEN_ON;
            } else {
                GREEN_OFF;
            }
            if (i < blue_duty) {
                BLUE_ON;
            } else {
                BLUE_OFF;
            }
        }
    }
}
void cycle(void) {
    receptive = 0;
    
    random_delay_1 = rand();
    random_delay_2 = RAND_MAX - random_delay_1;
    // delay to avoid collisions in id transmission
    for (; random_delay_1 > 0; random_delay_1--) {
        asm volatile("nop\n\t");
    }
    transmit_id();
    // delay for complement, so timing stays roughly consistent
    for (; random_delay_2 > 0; random_delay_2--) {
        asm volatile("nop\n\t");
    }

    for (total_duty = 0; total_duty < MAX_DUTY; total_duty++) {
        shine();
    }
    for (total_duty = MAX_DUTY; total_duty > 0; total_duty--) {
        shine();
    }
    degrade();
    receptive = 1;
    _delay_ms(1000);
}

/***
 * IR routines
 */
void send_38k_pulse(void) {
    // .2ms long pulse of 38kHz.
    for (pulse_count = 8; pulse_count > 0; pulse_count--) {
        IR_ON;
        _delay_ms(0.00246);
        IR_OFF;
        _delay_ms(0.02214);
    }
}
void transmit_id(void) {
    cli();
    send_38k_pulse();
#if UNIT_ID == 0
    _delay_ms(ZERO_DURATION);
#elif UNIT_ID == 1
    _delay_ms(ONE_DURATION);
#elif UNIT_ID == 2
    _delay_ms(TWO_DURATION);
#endif
    send_38k_pulse();
    _delay_ms(POST_SEND_DELAY);
    sei();
}
// Receive data from IR decoder
ISR(TIMER1_CAPT_vect) {
    new_timestamp = ICR1;
    time_interval = new_timestamp - old_timestamp;
    old_timestamp = new_timestamp;

    if (DUR_IN_RANGE(time_interval, CLOCK_ZERO_DURATION)) {
        red_count++;
        total_count++;
    } else if (DUR_IN_RANGE(time_interval, CLOCK_ONE_DURATION)) {
        green_count++;
        total_count++;
    } else if (DUR_IN_RANGE(time_interval, CLOCK_TWO_DURATION)) {
        blue_count++;
        total_count++;
    } else {
        return;
    }

    if (receptive) {
        reset_stack_pointer();
    }
}
