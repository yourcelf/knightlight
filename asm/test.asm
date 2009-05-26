.device attiny44

.macro init_leds
    ldi temp, 0xFF
    out DDRA, temp
    out PORTA, temp
.endmacro

.macro red_on
    cbi PORTA, PA0
.endmacro
.macro red_off
    sbi PORTA, PA0
.endmacro

.macro green_on
    cbi PORTA, PA1
.endmacro
.macro green_off
    sbi PORTA, PA1
.endmacro

.macro blue_on
    cbi PORTA, PA2
.endmacro
.macro blue_off
    sbi PORTA, PA2
.endmacro

.macro ir_on
    cbi PORTA, PA3
.endmacro
.macro ir_off
    sbi PORTA, PA3
.endmacro

.def temp = R16
.def pwm_0 = R17
.def pwm_1 = R18
.def pwm_2 = R19

.cseg
.org 0
rjmp reset

;
; main program
;
reset:
    ldi temp, high(RAMEND)
    out SPH, temp
    ldi temp, low(RAMEND)
    out SPL, temp

    init_leds

    main_loop:
        ldi pwm_0, 0xFF
        pwm_0_loop:
            ldi pwm_1, 0xFF
            pwm_1_loop:
                cp pwm_0, pwm_1
                    brlo pwm_1_loop_light_on

                green_on
                blue_on
                red_off
                rjmp pwm_1_loop_finish

                pwm_1_loop_light_on:
                    green_off
                    red_on
                    blue_off

                pwm_1_loop_finish:
                    dec pwm_1
                    brne pwm_1_loop


            red_on

            dec pwm_0
            brne pwm_0_loop

        rjmp main_loop

