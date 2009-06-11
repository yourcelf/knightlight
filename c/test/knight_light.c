#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000
#include <util/delay.h>
#include <stdint.h>

#define UNIT_ID 3

#define IR_STARTBIT_DURATION (.003 * F_CPU)
#define IR_ZERO_DURATION (.0012 * F_CPU)
#define IR_ONE_DURATION (.0018 * F_CPU)
#define DUR_IN_RANGE(dur, constant) ((dur > (int)(constant * 0.8)) && (dur < (int)(constant * 1.2)))

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

uint8_t i, j; // loop vars
uint8_t pulse_count;
uint8_t green_level, red_level, blue_level;
// IR i/o
uint8_t ir_byte_out;
uint8_t ir_bitmask_out;
uint8_t ir_byte_in;
uint8_t ir_bitmask_in;

uint16_t new_timestamp, old_timestamp, time_interval;

// Function prototypes
void init(void);

void shine(void);
void send_ir_start_bit(void);
void send_38k_pulse(void);
void send_ir_0(void);
void send_ir_1(void);
void byte_received(void);
void transmit_id(void);

void test_lights(void);
void test_broadcast_ir(void);

void main(void) {
    init();
    test_lights();
    ALL_OFF;
    while (1) {
        //test_lights();
        transmit_id();
        _delay_ms(500);
    }
}
void init(void) {
    if (F_CPU == 8000000) {
        // Init clock divider to 1 (8MHz)
        CLKPR = (1 << CLKPCE);
        CLKPR = (0 << CLKPS3)|(0 << CLKPS2)|(0 << CLKPS1)|(0 << CLKPS0);
    }
    // Init LEDs (all on PORTA)
    PORTA = 0xFF & ~(1 << PA5);
    DDRA = 0xFF;
    
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

    sei();
}

/***
 * IR routines
 */
void send_ir_start_bit(void) {
    send_38k_pulse();
    _delay_ms(2.8);
    send_38k_pulse();
}
void send_38k_pulse(void) {
    // .2ms long pulse of 38kHz.
    for (pulse_count = 8; pulse_count > 0; pulse_count--) {
        IR_ON;
        _delay_ms(0.00246);
        IR_OFF;
        _delay_ms(0.02214);
    }
}
void send_ir_0(void) {
    _delay_ms(1);
    send_38k_pulse();
}
void send_ir_1(void) {
    _delay_ms(1.6);
    send_38k_pulse();
}
void send_ir_byte_delay(void) {
    _delay_ms(10);
}
void send_ir_byte(void) {
    send_ir_start_bit();
    ir_bitmask_out = 0b10000000; //MSB first
    while (ir_bitmask_out > 0) {
        if (ir_byte_out & ir_bitmask_out) {
            send_ir_1();
        } else {
            send_ir_0();
        }
        ir_bitmask_out >>= 1;
    }
    send_ir_byte_delay();
}
// Receive data from IR decoder
ISR(TIMER1_CAPT_vect) {
    new_timestamp = ICR1;
    time_interval = new_timestamp - old_timestamp;
    old_timestamp = new_timestamp;

    if (DUR_IN_RANGE(time_interval, IR_STARTBIT_DURATION)) {
        ir_bitmask_in = (1 << 7); // MSB first
        ir_byte_in = 0;
    } else if (DUR_IN_RANGE(time_interval, IR_ZERO_DURATION)) {
        ir_bitmask_in >>= 1;
    } else if (DUR_IN_RANGE(time_interval, IR_ONE_DURATION)) {
        ir_byte_in |= ir_bitmask_in;
        ir_bitmask_in >>= 1;
    }
    if (ir_bitmask_in == 0) {
        ir_bitmask_in = 0xFF; // Prevent double-calling byte_received()
        byte_received();
    }
}
void byte_received(void) {
    ir_bitmask_in = (1 << 7);
    while (ir_bitmask_in > 0) {
        if (ir_byte_in & ir_bitmask_in) {
            _delay_ms(1);
        } else {
            _delay_ms(2);
        }
        ir_bitmask_in >>= 1;
        _delay_ms(1);
    }
    ir_bitmask_in = 0xFF;

    ALL_OFF;
    if (ir_byte_in == 1) {
        RED_ON;
    } else if (ir_byte_in == 2) {
        GREEN_ON;
    } else if (ir_byte_in == 3) {
        BLUE_ON;
    }
    _delay_ms(100);
    ALL_OFF;
}
void transmit_id(void) {
    cli();
    ir_byte_out = UNIT_ID;
    send_ir_byte();
    sei();
}

// higher is slower
#define LIGHT_SPEED 255
void shine(void) {
    for (i = LIGHT_SPEED; i > 0; i--) {
        if (i < green_level) {
            GREEN_ON;
        } else {
            GREEN_OFF;
        }
        if (i < red_level) {
            RED_ON;
        } else {
            RED_OFF;
        }
        if (i < blue_level) {
            BLUE_ON;
        } else {
            BLUE_OFF;
        }
    }
}

/**
 * Testing routines
 */
void test_broadcast_ir(void) {
    ir_byte_out--;
    send_ir_byte();
}

void test_lights(void) {
    for (j = LIGHT_SPEED; j > 0; j--) {
        red_level = j;
        green_level = LIGHT_SPEED - j;
        shine();
    }
    for (j = LIGHT_SPEED; j > 0; j--) {
        green_level = j;
        blue_level = LIGHT_SPEED - j;
        shine();
    }
    for (j = LIGHT_SPEED; j > 0; j--) {
        blue_level = j;
        red_level = LIGHT_SPEED - j;
        shine();
        shine();
    }
}
