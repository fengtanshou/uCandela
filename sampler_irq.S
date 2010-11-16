/*
 * Fast capture IRQ routines
 */

#include <avr/io.h>

#define TCCR1_CS_MASK ( _BV(CS13)|_BV(CS12)|_BV(CS11)|_BV(CS10) )

	.section .noinit, "a"
	.global sampler_value
	.type sampler_value, @object
	.size sampler_value, 1
sampler_value:	.byte 0
	
	.text
	.global ANA_COMP_vect
	.type   ANA_COMP_vect, @function
	.global TIM1_OVF_vect
	.type	TIM1_OVF_vect, @function

.macro save_context FREE_REG	; total = 5 ck
	push r0			; 2 ck
	in r0, SREG		; 1 ck
	push \FREE_REG		; 2 ck
.endm

.macro restore_context FREE_REG	; total = 5 ck
	pop \FREE_REG		; 2 ck
	out SREG, r0		; 1 ck
	pop r0			; 2 ck
.endm

;;;
;;; Timer1 overflow routine
;;; - used for handling overflow condition
;;; 
TIM1_OVF_vect:			; 4 ck (rjmp to ISR)
	save_context r24	; 5 ck
	in r24, TCCR1
	andi r24, TCCR1_CS_MASK
	brne 2f
	sei
	rjmp capture_irq_exit
2:
	andi r24, 0
	out TCCR1, r24
	sei
	ser r24
	sts sampler_value, r24
	rjmp capture_irq_exit

	;;;
;;; Analog comparator routine
;;; - used for handling capture, tracks TCNT1
;;; 
ANA_COMP_vect:			; 4 ck (rjmp to ISR)
	save_context r24	; 5 ck
	in r24, TCCR1		; 1 ck
	andi r24, TCCR1_CS_MASK	; 1 ck
	brne 1f			; 1/2ck
	sei			; 1 ck
	;;  14 ck used at this point
	rjmp capture_irq_exit
1:
	andi r24, 0		; 1 ck
	out TCCR1, r24		; 1 ck
	sei			; 1 ck
	;; 17 ck used at this point
	in r24, TCNT1
	sts sampler_value, r24
	;; fallthrough

capture_irq_exit:
	restore_context r24
	reti
