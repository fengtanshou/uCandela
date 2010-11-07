; useful utilities and macros


;;
;; === BASIC 16 bit register manipulation
;;
; loads a 16 bit value into register pair (@0H:@0L)
; @0 reg alias
; @1 immediate
.macro ldiw regH, regL, value
	ldi \regH,   lo8(\value)
	ldi \regL, hi8(\value)
.endm

; loads a 16 bit address into register pair (@0H:@0L)
; @0 reg alias
; @1 address
.macro ladr regH, regL, addr
	ldiw \regH, \regL, (\addr << 1)
.endm

;; ; performs 16 bit clear
;; .macro clrw reg
;; 	clr "(\reg)"
;; 	clr "(\reg+1)"
;; .endm

;; ; performs 16 bit set
;; .macro setw reg
;; 	ser "(\reg)"
;; 	ser "(\reg+1)"
;; .endm

;; ; performs 16 bit move 
;; ; @0 Destination
;; ; @1 Source
;; .macro movew dstreg
;; 	mov @0L, @1L
;; 	mov @0H, @1H
;; .endm

;; ; performs 16 bit port input
;; ; @0 register
;; ; @1 port
;; .macro inw
;; 	in @0L, @1L
;; 	in @0H, @1H
;; .endm

;; ; performs 16 bit port out
;; ; @0 port
;; ; @1 register
;; .macro outw
;; 	out @0L, @1L
;; 	out @0H, @1H
;; .endm



;; ;;
;; ;; === 16 bit arithmetics ===
;; ;;

;; ; perform 16 bit addition
;; ; @0 destination
;; ; @1 source
;; .macro addw
;; 	add @0L, @1L
;; 	adc @0H, @1H
;; .endm

;; ; perform 16 bit subtraction
;; ; @0 dst
;; ; @1 src
;; .macro subw
;; 	sub @0L, @1L
;; 	sbc @0H, @1H
;; .endm

;; .macro comw
;; 	com @0L
;; 	com @0H
;; .endm

;; .macro negw
;; 	neg @0L
;; 	neg @0H
;; .endm

;; .macro asrw
;; 	asr @0H
;; 	ror @0L
;; .endm 

;; .macro lsrw
;; 	clc
;; 	ror @0H
;; 	ror @0L
;; .endm

;; .macro lslw
;; 	clc
;; 	rol @0L
;; 	rol @0H
;; .endm


;; ;;
;; ;; === special arithmetic operations ===
;; ;;

;; ; saturate result of Unsigned + Signed addition
;; ; 16 bits wide
;; ;
;; ; the math behind the code is
;; ; let R = X + Y = @0 + @1
;; ; we expand both values to signed 17 bit, and then
;; ; perform the addition.
;; ; X16 is always 0, Y16 equal to Y15 (sign expansion)
;; ; so, R16 = carry + X16 + Y16 = carry + Y15
;; ; saturation occurs only if R16 is 1
;; .macro satausw
;; 	sbrc @1H, 7	; if Y15 == 1
;; 	brcs skip	;	 & carry == 1 goto skip
;; 	sbrs @1H, 7	; if Y == 0
;; 	brcc skip	; 	 & carry == 0 goto skip
;; 	; compute saturation value
;; 	clr @0H
;; 	sbrs @1H, 7
;; 	com @0H
;; 	mov @0L,@0H
;; skip:
;; .endm

;; ; synopsis: movee Rd,Rs
;; ;  reads byte from EEPROM address defined in Rs
;; ;  and stores it in Rd
;; .macro movee
;; 	out EEAR,@1
;; 	sbi EECR,EERE
;; 	in  @0,EEDR
;; .endm


;; loops
.macro loopz label
	inc ZL
	brne \label
	inc ZH
	rjmp \label
.endm

;; ; waiting
;; ; delay = 6*N+5
;; .macro wait32
;; wait32_loop:
;; 	subi ZL,1
;; 	sbci ZH,0
;; 	sbci YL,0
;; 	sbci YH,0
;; 	brcc wait32_loop
;; .endm

;; ; delay = 6*N+5
;; .macro wait16
;; wait16_loop:
;; 	subi @0L,1
;; 	sbci @0H,0
;; 	nop
;; 	nop
;; 	brcc wait16_loop
;; .endm

;; ; addition with saturation
;; .macro adds
;; 	add @0,@1
;; 	brcc nosat
;; 	ser @0
;; nosat:
;; .endm

;; ; subtraction with saturation
;; .macro subs
;; 	sub @0,@1
;; 	brcc nosat
;; 	clr @0
;; nosat:
;; .endm

;; ; signed addition with saturation
;; ; the idea is:
;; ; 127+127 = 127 (+V+N)
;; ; -128-128 = -128 (+V+S)
;; .macro addss
;; 	add @0,@1
;; 	brvc nosat
;; 	ldi @0,$7f
;; 	brmi nosat
;; 	inc @0
;; nosat:
;; .endm

;; .macro saturate16_s
;; 	brbc SREG_V,nosat
;; 	ser @1
;; 	ldi @0,$7f
;; 	brmi nosat
;; 	inc @1
;; 	inc @0
;; nosat:
;; .endm

;; .macro ror16_4
;; 	ror @0
;; 	ror @1
;; 	ror @0
;; 	ror @1
;; 	ror @0
;; 	ror @1
;; 	ror @0
;; 	ror @1
;; .endm

;; ; set or clear arg1 according to its MSB
;; .macro signexp
;; 	sbrs @0,7
;; 	clr @0
;; 	sbrc @0,7
;; 	ser @0
;; .endm
