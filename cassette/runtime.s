; A8Duo standard CAS reader for the bundled RAM OS, assembled with ca65.
; The boot sequence follows AltirraOS boot.s, Copyright (C) 2008-2016 Avery Lee.
; Copying and distribution of that work, with or without modification, are
; permitted in any medium without royalty provided the copyright notice and
; this notice are preserved. Offered as-is, without any warranty.
;
; Original transport/adapter code for this project. No SIO cable is used.
; Executes in unused OS RAM, not in Atari program RAM. D5xx commands run with
; IRQ disabled and never require Atari cartridge ROM while the MCU is busy.
; The OS critical flag defers normal VBI work; display NMIs may run from RAM.
.setcpu "6502"
.export cas_entry, cas_sio, cas_open, cas_open_read, record_sector, cas_end
.segment "CODE"
cas_entry:
    lda #2
    sta record_sector
    lda #0
    sta record_sector+1
    sta $09
    lda #<cas_sio
    sta $e45a
    lda #>cas_sio
    sta $e45b
    lda #<(cas_open-1)
    sta $e440
    lda #>(cas_open-1)
    sta $e441
    lda #<cas_open_read
    sta $e47e
    lda #>cas_open_read
    sta $e47f
    lda #$80
    sta $3e
    jsr cas_open_read
    jsr $e47a
    bpl :+
    jmp boot_error
:
    ldx #3
@header:
    lda $0400,x
    sta $0240,x
    dex
    bpl @header
    lda $0242
    sta $15
    lda $0243
    sta $16
    lda $0404
    sta $02
    lda $0405
    sta $03
@block:
    ldy #127
@copy:
    lda $0400,y
    sta ($15),y
    dey
    bpl @copy
    clc
    lda $15
    adc #128
    sta $15
    bcc :+
    inc $16
:
    ; Consume the EOF record after the boot body, like the Atari OS. The
    ; last read's status is ignored; the real OS permits a missing EOF.
    jsr $e47a
    dec $0241
    beq @loaded
    tya
    bpl @block
    jmp boot_error
@loaded:
    lda $0242
    clc
    adc #6
    sta $04
    lda $0243
    adc #0
    sta $05
    jsr run_loader
    bcs boot_error
    jsr run_init
    lda #0
    sta $03e9
    lda #2
    sta $09
    jmp ($0a)
run_loader:
    jmp ($04)
run_init:
    jmp ($02)
boot_error:
    lda #$44
    sta $02c8
    ; Report a transport/boot-continuation failure through the stock E: IOCB.
    lda #11
    sta $0342
    lda #<error_text
    sta $0344
    lda #>error_text
    sta $0345
    lda #error_end-error_text
    sta $0348
    lda #0
    sta $0349
    ldx #0
    jsr $e456
@stop:
    jmp @stop
error_text: .byte "CAS LOAD ERROR - RESET CART",$9b
error_end:

cas_open:
    lda $2a
    and #$0c
    cmp #4
    beq :+
    ldy #146
    rts
:
    lda $2b
    sta $3e
cas_open_read:
    lda #0
    sta $0289
    sta $3f
    lda #128
    sta $3d
    sta $028a
    lda #$34
    sta $d302
    ldy #1
    rts

cas_sio:
    lda $0300
    cmp #$60
    beq :+
    jmp $e7f2                 ; generator asserts the bundled SIOV target
:
    lda $0302
    cmp #$52
    beq :+
    jmp unsupported
:
    lda $0309
    beq :+
    jmp unsupported
:
    lda $0308
    cmp #133
    bcc :+
    jmp unsupported
:
    ; Save the application's scratch pointer and all interrupt enable state.
    php
    sei
    lda $42
    pha
    lda #1
    sta $42
    lda $cb
    pha
    lda $cc
    pha
    lda #138
    sta io_status
    lda record_sector
    sta $d501
    lda record_sector+1
    sta $d502
    jsr read_sector
    bne @restore
    ldx #127
@first:
    lda $d502,x
    sta frame_buffer,x
    dex
    bpl @first
    clc
    lda record_sector
    adc #1
    sta $d501
    lda record_sector+1
    adc #0
    sta $d502
    jsr read_sector
    bne @restore
    ldx #3
@second:
    lda $d502,x
    sta frame_buffer+128,x
    dex
    bpl @second
    lda frame_buffer
    cmp #$55
    bne @restore
    lda frame_buffer+1
    cmp #$55
    bne @restore
    clc
    lda record_sector
    adc #2
    sta record_sector
    bcc :+
    inc record_sector+1
:
    lda $0304
    sta $cb
    lda $0305
    sta $cc
    ldy #0
@copy:
    cpy $0308
    beq @success
    lda frame_buffer,y
    sta ($cb),y
    iny
    bne @copy
@success:
    lda #1
    sta io_status
@restore:
    pla
    sta $cc
    pla
    sta $cb
    pla
    sta $42
    plp
    ldy io_status
    bne sio_exit
unsupported:
    ldy #146
sio_exit:
    sty $0303
    sty $30
    tya
    rts
read_sector:
    lda #0
    sta $d500
    sta $d503
    lda #$21
    sta $d5df
@wait:
    lda $d500
    cmp #$11
    bne @wait
    lda $d501
    rts
record_sector: .word 2
io_status: .byte 0
frame_buffer: .res 132,0
cas_end:
.assert cas_end <= $cc00, error, "Cassette runtime exceeds reserved OS filler"
