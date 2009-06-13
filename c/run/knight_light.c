#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

// Higher is slower
#define MAX_DUTY 180
// Lower is brighter
#define DUTY_DIVISOR 2 

#define BLINKS_BEFORE_TIMEOUT 10
#define BLINKS_BEFORE_COLOR_CHANGE 3
// Higher is longer
#define POST_BLINK_DELAY 1000
// higher is sloppier, lower is more collisions
#define PRE_BLINK_DELAY 50000

// in milliseconds
#define RED_DURATION .25
#define GREEN_DURATION .5
#define BLUE_DURATION .75

#define NUM_COLORS 3

#define POST_SEND_DELAY .25

#define DUR_FUDGE_FACTOR 0.5
#define DUR_IN_RANGE(dur, constant) (dur < (((constant + DUR_FUDGE_FACTOR) / 1000) * F_CPU))

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

#define RED   0
#define GREEN 1
#define BLUE  2

uint8_t i, total_duty; // loop vars
uint8_t pulse_count;
uint8_t timeout_count;
uint8_t color;
uint16_t green_duty, red_duty, blue_duty, duty_duty;

// IR i/o

uint8_t receptive;
uint8_t lose_immunity;
uint8_t immune;
uint16_t new_timestamp, old_timestamp, time_interval;
uint16_t delay_1;
uint16_t delay_2;

// Function prototypes
void init(void);
void receptive_sleep_delay(void);
void randomize_color(void);
void cycle(void);
void transmit_id(void);
void send_38k_pulse(void);
void byte_received(void);


int main(void) {
    init();
    timeout_count = BLINKS_BEFORE_TIMEOUT;
    for (;;) {
        for (; timeout_count > 0; timeout_count--) {
            if (timeout_count == (BLINKS_BEFORE_TIMEOUT - BLINKS_BEFORE_COLOR_CHANGE)) {
                randomize_color();
            }
            cycle();
        }
        receptive_sleep_delay();
        transmit_id();
    }
    return 0;
}
void receptive_sleep_delay(void) {
    receptive = 1;
    for (delay_1 = 255; delay_1 > 0; delay_1--) {
        _delay_ms(1000);
        GREEN_OFF; // argh compiler.  Makes the following work.
        // Required to avoid compiler optimization that breaks cross-thread
        // variable update
        asm volatile("lds __tmp_reg__, %0" "\n\t"
                     "sbrs __tmp_reg__, 0" "\n\t"
                     "ret" "\n\t"
                     :: "i" (& receptive));
    }
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

    receptive = 0;

    sei();
}

void shine(void) {
    duty_duty = total_duty / (DUTY_DIVISOR);
    if (color == RED) {
        red_duty = duty_duty;
        green_duty = 0;
        blue_duty = 0;
    } else if (color == GREEN) {
        red_duty = 0;
        green_duty = duty_duty;
        blue_duty = 0;
    } else if (color == BLUE) {
        red_duty = 0;
        green_duty = 0;
        blue_duty = duty_duty;
    }
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
void cycle(void) {
    receptive = 0;
    delay_1 = rand() % PRE_BLINK_DELAY;
    delay_2 = PRE_BLINK_DELAY - delay_1;
    //srand(TCNT1);
    // delay to avoid collisions in id transmission
    for (; delay_1 > 0; delay_1--) {
        asm volatile("nop\n\t");
    }
    transmit_id();
    // delay for complement, so timing stays roughly consistent
    for (; delay_2 > 0; delay_2--) {
        asm volatile("nop\n\t");
    }

    for (total_duty = 0; total_duty < MAX_DUTY; total_duty++) {
        shine();
    }
    for (total_duty = MAX_DUTY; total_duty > 0; total_duty--) {
        shine();
    }
    receptive = 1;
    for (delay_1 = POST_BLINK_DELAY; delay_1 > 0; delay_1--) {
        _delay_ms(1);
        // If we don't do this with ASM, the compiler will abstract away the
        // SRAM to a register, and won't see when it is updated in the
        // interrupt.
        // equivalent:
        // if (receptive == 0) {
        //    return;
        // }
        asm volatile("lds __tmp_reg__, %0" "\n\t"
                     "sbrs __tmp_reg__, 0" "\n\t"
                     "ret" "\n\t"
                     :: "i" (& receptive));


    }
    if (lose_immunity) {
        immune = 0;
    }
}
void randomize_color(void) {
    uint8_t old_color = color;
    color = rand() % NUM_COLORS;
    if (old_color == color) {
        color = (color + 1) % NUM_COLORS;
    }
    immune = 1;
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
    if (color == RED) {
        _delay_ms(RED_DURATION);
    } else if (color == GREEN) {
        _delay_ms(GREEN_DURATION);
    } else if (color == BLUE) {
        _delay_ms(BLUE_DURATION);
    }
    send_38k_pulse();
    _delay_ms(POST_SEND_DELAY);
    sei();
}
// Receive data from IR decoder
ISR(TIMER1_CAPT_vect) {
    new_timestamp = ICR1;
    time_interval = new_timestamp - old_timestamp;
    old_timestamp = new_timestamp;

    if (DUR_IN_RANGE(time_interval, RED_DURATION)) {
        if (immune == 0) {
            color = RED;
        }
    } else if (DUR_IN_RANGE(time_interval, GREEN_DURATION)) {
        if (immune == 0) {
            color = GREEN;
        }
    } else if (DUR_IN_RANGE(time_interval, BLUE_DURATION)) {
        if (immune == 0) {
            color = BLUE;
        }
    } else {
        return;
    }

    lose_immunity = 1;
    timeout_count = BLINKS_BEFORE_TIMEOUT;
    receptive = 0;
}
