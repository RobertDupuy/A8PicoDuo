.setcpu "6502"
.export boot, init, run, finished, low_payload
.segment "CODE"
boot:
.byte 0,4
.word $0700,init
    lda #$a1
    sta $0600
    lda #<run
    sta $0a
    lda #>run
    sta $0b
    clc
    rts
init:
    lda #$a2
    sta $0601
    rts
run:
    lda #$a3
    sta $0602
    ; Open C: through CIO and read a second file after the boot EOF.
    lda #<name
    sta $0354
    lda #>name
    sta $0355
    lda #3
    sta $0352
    lda #4
    sta $035a
    lda #128
    sta $035b
    ldx #16
    jsr $e456
    sty $0603
    lda #7
    sta $0352
    lda #0
    sta $0354
    lda #$20
    sta $0355
    lda #5
    sta $0358
    lda #0
    sta $0359
    ldx #16
    jsr $e456
    sty $0604
    ; Ask for one more byte, testing the EOF record.
    lda #1
    sta $0358
    ldx #16
    jsr $e456
    sty $0605
    ; Same cassette through raw SIO (next file) and RBLOKV.
    lda #$60
    sta $0300
    lda #$52
    sta $0302
    lda #0
    sta $0304
    lda #$21
    sta $0305
    lda #131
    sta $0308
    lda #0
    sta $0309
    jsr $e459
    sty $0606
    jsr $e47a
    sty $0607
    lda $0400
    sta $0608
    ; Reject writes through C: without changing the source file.
    lda #12
    sta $0352
    ldx #16
    jsr $e456
    lda #<name
    sta $0354
    lda #>name
    sta $0355
    lda #3
    sta $0352
    lda #8
    sta $035a
    ldx #16
    jsr $e456
    sty $060a
    ; Raw SIO consumes the final EOF, then times out beyond the source file.
    lda #$60
    sta $0300
    lda #$52
    sta $0302
    lda #0
    sta $0304
    sta $0309
    lda #$22
    sta $0305
    lda #131
    sta $0308
    jsr $e459
    sty $060b
    jsr $e459
    sty $060c
    ldx #0
check:
    lda $0600,x
    cmp expected,x
    bne failed
    inx
    cpx #9
    bne check
    lda $060a
    cmp #146
    bne failed
    lda $060b
    cmp #1
    bne failed
    lda $060c
    cmp #138
    bne failed
    ldx #4
check_hello:
    lda $2000,x
    cmp hello,x
    bne failed
    dex
    bpl check_hello
    ldx #13
check_raw:
    lda $2103,x
    cmp raw,x
    bne failed
    dex
    bpl check_raw
    lda #<pass_text
    ldy #>pass_text
    jmp display
failed:
    lda #$44
    sta $02c8
    lda #<fail_text
    ldy #>fail_text
 display:
    sta $0344
    sty $0345
    lda #11
    sta $0342
    lda #text_end-pass_text
    sta $0348
    lda #0
    sta $0349
    ldx #0
    jsr $e456
    lda #$a4
    sta $0609
finished:
    jmp finished
name: .byte "C:",$9b
expected: .byte $a1,$a2,$a3,1,1,136,1,1,'Z'
hello: .byte "HELLO"
raw: .byte "RAW SIO RECORD"
pass_text: .byte $7d,"A8DUO CAS TEST: PASS",$9b,"RESET CART TO RETURN TO MENU",$9b
text_end:
fail_text: .byte $7d,"A8DUO CAS TEST: FAIL",$9b,"RESET CART TO RETURN TO MENU",$9b
.res 512-(*-boot),0
low_payload:
