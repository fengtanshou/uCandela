;;;;;;;;;;;
;; UART routines
;;;;;;;;;;;

#include <avr/io.h>
#include "uart_config.h"
#include "regs.h"

#if defined(UART_RX_ENABLE)
	.set ENABLE_RX, 1
#endif

#if defined(UART_TX_ENABLE)
	.set ENABLE_TX, 1
#endif
	
	.include "util.i"

	.section .noinit, "a"
	uart_speed:
	.byte 0
	.type   uart_speed, @object
	.size	uart_speed, 1
	
	.text
	.global uart_init

; delay = 3*N+2 ticks
.macro uart_delay8 N
42:	dec \N
	brne 42b
.endm


uart_init:
	ldi xl, lo8(uart_speed)
	ldi xh, hi8(uart_speed)
	st  X, r24
.ifdef ENABLE_TX
	sbi UART_PORT, UART_TXB ; TX high
	sbi DDRB,UART_TXDB	; allow output
.endif
.ifdef ENABLE_RX
	cbi DDRB,UART_RXDB	; input at
.endif
	ret

/*
 * Transmit single byte
 */
.ifdef ENABLE_TX
	.global uart_putchar
	.type uart_putchar, @function
	;; r25:r24 - character
uart_putchar:
	ldi r18, 10 		; 1start+8data+1stop
	ldi xl, lo8(uart_speed)
	ldi xh, hi8(uart_speed)

	clc			; zero start bit
	rol r24			; push start bit into chain
	
1:	sbrs r24, 0		; 2c/3c
	cbi UART_PORT, UART_TXB	; 2c
	sbrc r24, 0		; 3c/2c
	sbi UART_PORT, UART_TXB	; 2c -- total 9c

	ror r24			; get away with bit we just sent (1c)
	sec			; shift in only 1 bytes (1c)

	ld  r19, X		; wait (1c)
	uart_delay8 r19

	dec r18			; 1c
	brne 1b			; 2c/1c
	;; total loop time 14 cycles
	ret
.endif

/*
 * Receive single byte
 */
.ifdef ENABLE_RX
	.global uart_getchar
	.type uart_getchar, @function
	;; returns r25:r24
uart_getchar:
	ldi r18, 9 		; 1start+8data+1stop
	ldi xl, lo8(uart_speed)
	ldi xh, hi8(uart_speed)

	ld r19, X
	lsr r19 		; wait delay/2 to hit the middle of start pulse 
	uart_delay8 r19

1:	sbis UART_PIN, UART_RXB ; 2c/3c |
	clc			; 1c	|
	sbic UART_PIN, UART_RXB	; 3/2c	| 7c
	sec			; 1c	|

	ror r24			; 1c
	ld r19, X		; 2c
	uart_delay8 r19

	nop			; maintain cycle length

	dec r18			; 1c
	brne 1b			; 2c/1c
	;; total loop time 14 cycles
	clr r25
	ret
.endif
