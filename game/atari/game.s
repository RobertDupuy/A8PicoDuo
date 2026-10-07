; A8Duo game v0.2.1: physical input, double-buffered ANTIC 5, PM player, POKEY.
; Uses the ORIGINAL cartridge's ATR commands. No timing-engine changes.
.setcpu "6502"
.include "../generated/display.inc"
FRAME=$0800
PM=$7400
HUD_BACK=$0900
HUD=$7A00
DL=$7B00
FONT=$7C00
.segment "ZEROPAGE"
fps: .res 1
state: .res 1
level: .res 1
px: .res 1
py: .res 1
clock: .res 1
lastclock: .res 1
elapsed: .res 1
pending: .res 1
activefont: .res 1
backfont: .res 1
newpx: .res 1
newpy: .res 1
newdying: .res 1
oldy: .res 1
sound: .res 1
sfx: .res 1
muted: .res 1
enemy_color: .res 1
new_enemy_color: .res 1
src: .res 2
dst: .res 2
sector: .res 2
pages: .res 1
temp: .res 1
timeout: .res 2
.export start,main_loop,cart_read,cart_write,cart_wait,load_level,fps,state,px,py,level,clock,pending
.segment "CODE"
 .byte 0,0
 .word $2000,start
start:
 sei
 cld
 ldx #$ff
 txs
 lda #0
 sta $D40E
 sta $D20E
 sta $D400
 sta $D01D
 ldx #$7f
clear_zp:
 sta $80,x
 dex
 bpl clear_zp
 sta dst
 lda #$70
 sta dst+1
 ldx #8
 ldy #0
 lda #0
clear_pm:
 sta (dst),y
 iny
 bne clear_pm
 inc dst+1
 dex
 bne clear_pm
 lda #3
 sta $D20F
 lda #0
 sta $D208
 sta $D201
 sta $D203
 sta $D205
 sta $D207
 sta $D008
 sta $D009
 sta $D00A
 sta $D00B
 sta $D00C
 sta $D01C
 sta $D001
 sta $D002
 sta $D003
 lda #1
 sta $D01B
 lda #2
 sta $D01D
 sta $D401
 lda #$70
 sta $D407
 lda #$48
 sta $D012               ; red player (PAL and NTSC palettes verified separately)
 lda #$8C
 sta $D016               ; purple walls
 lda #$2C
 sta $D019               ; yellow coins
 lda #$0E
 sta $D01A               ; light floor
 lda #<font_data
 sta src
 lda #>font_data
 sta src+1
 lda #0
 sta dst
 lda #>FONT
 sta dst+1
 ldx #4
 ldy #0
copy_font:
 lda (src),y
 sta (dst),y
 iny
 bne copy_font
 inc src+1
 inc dst+1
 dex
 bne copy_font
 ldx #display_end-display_list-1
copy_dl:
 lda display_list,x
 sta DL,x
 dex
 bpl copy_dl
 lda #<DL
 sta $D402
 lda #>DL
 sta $D403
 lda #0
 sta temp
measure_frame:
 lda $D40B
 cmp temp
 bcc measured
 sta temp
 jmp measure_frame
measured:
 lda #60
 ldx temp
 cpx #140
 bcc measured_fps
 lda #50
measured_fps:
 sta fps
 lda #$40
 sta activefont
 sta backfont
 lda #<dli
 sta $0200
 lda #>dli
 sta $0201
 lda #<vbi
 sta $0222
 lda #>vbi
 sta $0223
 lda #$C0
 sta $D40E
 lda #0
 sta elapsed
 jsr load_level
first_commit:
 lda pending
 bne first_commit
 lda #$3A
 sta $D400
 lda clock
 sta lastclock
main_loop:
 lda pending
 bne main_loop
 lda clock
 sec
 sbc lastclock
 cmp #2
 bcc main_loop
 cmp #13
 bcc delta_ok
 lda #12
 delta_ok:
 sta elapsed
 lda clock
 sta lastclock
 jsr load_level
 jmp main_loop

load_level:
 ; The producer must not reuse the pending buffer before VBI acknowledges it.
wait_commit:
 lda pending
 bne wait_commit
 lda activefont
 eor #8
 sta backfont
 ; Keep a complete back buffer until the next VBI commits it.
 lda #'H'
 sta FRAME
 lda #'G'
 sta FRAME+1
 lda #HG_PROTOCOL_VERSION
 sta FRAME+2
 lda fps
 sta FRAME+3
 lda elapsed
 sta FRAME+4
 lda $D300
 eor #15
 and #15
 sta FRAME+5
 lda $D01F
 eor #7
 and #7
 asl a
 sta temp
 lda $D010
 eor #1
 and #1
 ora temp
 sta FRAME+6
 lda #0
 sta FRAME+7
 sta FRAME+8
 sta FRAME+9
 jsr cart_write
 bcc write_ok
 jmp failed
write_ok:
 lda #<FRAME
 sta dst
 lda #>FRAME
 sta dst+1
 lda #<$03E9
 sta sector
 lda #>$03E9
 sta sector+1
 jsr cart_read
 bcc status_ok
 jmp failed
status_ok:
 lda FRAME
 cmp #'H'
 bne bad_status
 lda FRAME+1
 cmp #'G'
 bne bad_status
 lda FRAME+2
 cmp #HG_PROTOCOL_VERSION
 bne bad_status
 lda FRAME+3
 bne bad_status
 lda FRAME+5
 cmp #6
 bcs bad_status
 lda FRAME+6
 cmp #HG_SCREEN_WIDTH-HG_PLAYER_WIDTH+1
 bcs bad_status
 lda FRAME+7
 cmp #HG_SCREEN_HEIGHT-HG_PLAYER_HEIGHT+1
 bcs bad_status
 jmp valid_status
bad_status:
 jmp failed
valid_status:
 lda backfont
 sta dst+1
 lda #0
 sta dst
 lda #HG_VIDEO_SECTORS
 sta pages
read_video:
 inc sector
 bne no_sector_carry
 inc sector+1
no_sector_carry:
 jsr cart_read
 bcs failed
 clc
 lda dst
 adc #128
 sta dst
 bcc no_page_carry
 inc dst+1
no_page_carry:
 dec pages
 bne read_video
 lda FRAME+4
 sta level
 lda FRAME+5
 sta state
 lda FRAME+6
 sta newpx
 lda FRAME+7
 sta newpy
 lda FRAME+15
 sta newdying
 lda FRAME+16
 sta muted
 lda FRAME+22
 sta new_enemy_color
 jsr update_hud
 jsr audio_update
 lda #1
 sta pending
 rts
failed:
 lda #4
 sta state
 ldx #39
 lda #0
error_blank:
 sta HUD_BACK,x
 dex
 bpl error_blank
 ldx #20
error_text:
 lda error,x
 sta HUD_BACK+9,x
 dex
 bpl error_text
 lda #2                 ; HUD-only commit: keep last complete maze and player.
 sta pending
 rts

; ROM NMI entry saves A/X/Y before invoking the immediate VBI vector.
; Exit through XITVBV without the OS display-list/shadow-register work.
vbi:
 inc clock
 lda pending
 beq no_commit
 cmp #2
 beq commit_hud
 lda backfont
 sta activefont
 clc
 adc #4
 sta DL+(display_world-display_list)+2
 lda newpx
 sta px
 lda newpy
 sta py
 lda new_enemy_color
 sta enemy_color
 ldx oldy
 lda #0
 ldy #HG_PLAYER_SCANLINES
clear_player:
 sta PM,x
 inx
 dey
 bne clear_player
 lda py
 asl a
 clc
 adc #HG_WORLD_FIRST_SCANLINE
 sta oldy
 tax
 lda #HG_PLAYER_PATTERN
 ldy newdying
 beq solid_player
 lda clock
 and #2
 beq solid_player
 lda #0
 jmp player_pattern
solid_player:
 lda #HG_PLAYER_PATTERN
player_pattern:
 ldy #HG_PLAYER_SCANLINES
plot_player:
 sta PM,x
 inx
 dey
 bne plot_player
 lda px
 clc
 adc #48
 sta $D000
commit_hud:
 ldx #39
commit_hud_byte:
 lda HUD_BACK,x
 sta HUD,x
 dex
 bpl commit_hud_byte
 lda #0
 sta pending
no_commit:
 lda #>FONT
 sta $D409
 lda #$0E
 sta $D017
 lda #0
 sta $D018
 lda sound
 beq no_sound
 dec sound
 bne no_sound
 lda #0
 sta $D201
no_sound:
 jmp $E462               ; XITVBV: restores Y, X, A; RTI

dli:
 pha
 lda activefont
 sta $D40A               ; apply after last HUD scanline
 sta $D409
 lda enemy_color
 sta $D017
 lda #$CC
 sta $D018
 pla
 rti

cart_read:
 lda sector
 sta $D501
 lda sector+1
 sta $D502
 lda #0
 sta $D500
 sta $D503
 lda #$21
 jsr cart_wait
 bcs bad_cart
 lda $D501
 bne bad_cart
 ; Eight-byte unroll keeps the 6502 transfer within two television frames.
 ldx #7
patch_copy:
 lda dst
 clc
 adc copy_offsets,x
 ldy copy_sites,x
 sta read_packet+1,y
 lda dst+1
 sta read_packet+2,y
 dex
 bpl patch_copy
 ldx #0
read_packet:
 .repeat 8,i
 lda $D502+i,x
 sta $4000+i,x
 .endrepeat
 txa
 clc
 adc #8
 tax
 bpl read_packet
 clc
 rts
bad_cart:
 sec
 rts
cart_write:
 lda #0
 sta $D500
 sta $D503
 lda #<$03E8
 sta $D501
 lda #>$03E8
 sta $D502
 ldx #0
write_packet:
 lda FRAME,x
 sta $D504,x
 inx
 cpx #128
 bne write_packet
 lda #$22
 jsr cart_wait
 bcs bad_cart
 lda $D501
 bne bad_cart
 clc
 rts
cart_wait:
 sta $D5DF
 lda #0
 sta timeout
 sta timeout+1
wait_cart_loop:
 lda $D500
 cmp #$11
 beq wait_cart_done
 dec timeout
 bne wait_cart_loop
 dec timeout+1
 bne wait_cart_loop
 sec
 rts
wait_cart_done:
 clc
 rts

update_hud:
 ldx #39
copy_hud:
 lda hud_text,x
 sta HUD_BACK,x
 dex
 bpl copy_hud
 lda level
 clc
 adc #1
 ldx #0
 jsr two_digits
 lda FRAME+11
 ora #16
 sta HUD_BACK+7
 lda FRAME+10
 ora #16
 sta HUD_BACK+8
 lda FRAME+9
 ora #16
 sta HUD_BACK+9
 lda FRAME+8
 ora #16
 sta HUD_BACK+10
 lda FRAME+12
 ldx #13
 jsr two_digits
 lda FRAME+13
 ldx #16
 jsr two_digits
 lda state
 asl a
 tax
 lda messages,x
 sta src
 lda messages+1,x
 sta src+1
 ldy #0
copy_message:
 lda (src),y
 sta HUD_BACK+19,y
 iny
 cpy #21
 bne copy_message
 rts
two_digits:
 ldy #16
tens:
 cmp #10
 bcc units
 sec
 sbc #10
 iny
 bne tens
units:
 ora #16
 sta HUD_BACK+1,x
 tya
 sta HUD_BACK,x
 rts

audio_update:
 lda muted
 beq audio_on
 lda #0
 sta $D201
 sta $D203
 sta $D205
 sta $D207
 rts
audio_on:
 ; Optional three-voice POKEY transcription arrives in the RAM snapshot.
 ldx #0
music_registers:
 lda FRAME+24,x
 sta $D202,x
 inx
 cpx #6
 bne music_registers
 lda FRAME+14
 and #1
 beq not_death
 lda #145
 sta $D200
 lda #$86
 ldx #20
 bne effect
not_death:
 lda FRAME+14
 and #4
 beq not_clear
 lda #32
 sta $D200
 lda #$A8
 ldx #24
 bne effect
not_clear:
 lda FRAME+14
 and #10
 beq no_effect
 lda #18
 sta $D200
 lda #$A6
 ldx #5
effect:
 sta $D201
 stx sound
no_effect:
 rts

.segment "RODATA"
display_list:
 .repeat HG_TOP_BLANK/8
 .byte $70
 .endrepeat
 .byte $C2
 .word HUD
display_world:
 .byte $45
 .word $4400
 .repeat HG_CHAR_ROWS-1
 .byte $05
 .endrepeat
 .byte $41
 .word DL
display_end:
; Atari screen codes are ASCII minus 32.
.macro msg name,text
name:
 .repeat .strlen(text),i
 .byte .strat(text,i)-32
 .endrepeat
 .if .strlen(text)<21
 .res 21-.strlen(text),0
 .endif
.endmacro
msg hud_text,"01/30 D0000 C00/00  FIRE:START            "
msg title,"FIRE:START SEL:LEVEL  "
msg play,"FIRE:PAUSE OPT:SOUND "
msg cleared,"CLEAR! FIRE:NEXT     "
msg won,"ALL 30! FIRE:RESTART "
msg error,"U2 LINK ERROR - RESET"
msg pause,"PAUSED FIRE:CONTINUE "
messages: .word title,play,cleared,won,error,pause
copy_offsets: .byte 0,1,2,3,4,5,6,7
copy_sites: .byte 3,9,15,21,27,33,39,45
font_data:
 .incbin "../generated/font.bin"
