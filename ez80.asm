;
; Native eZ80 assembly routines
;
; Copyright (c) 2019 Dr. D'nar
; SPDX-License-Identifier: BSD-2-clause

	.assume adl=1
	.def _decompress_string
	.def _lcd_dim
	.def _lcd_bright
	.def _get_rtc_seconds
	.def _get_rtc_seconds_plus
	.def _get_csc
	.def _on_key_pressed
	.def _clear_on_key
	.ref _huffman_tree
	.ref _exit_clean

_GetCSC                    = 002014Ch
_RestoreLCDBrightness      = 0021AB8h
_DimLCDSlow                = 0021AC0h
flags			= 0D00080h		; location of OS Flags (+-80h)
rtc_Seconds = 0F30000h
rtc_Minutes = 0F30004h

.text
;-------------------------------------------------------------------------------
_decompress_string:
; Inputs:
;  - arg0: Pointer to input Huffman bit stream
;  - arg1: Pointer to target buffer
; Output:
;  - HL: Pointer to byte after last written byte, can be used to check output
;        string length
	ld	iy, 0
	add	iy, sp
	push	ix
	ld	ix, (iy + 6)
	ld	iy, (iy + 3)
; Internal registers:
;  - BC: Current bit/byte of input Huffman stream
;  - DE: Temporary
;  - HL: Pointer to Huffman table entry
	ld	de, 0
	ld	b, 1
.loop:	
	ld	hl, (_huffman_tree)
.innerLoop:
	; Is this node a leaf?
	ld	e, (hl)
	bit	7, e
	jr	nz, .deHuffmanDone
	; Is it time to fetch another byte of input?
	djnz	.getNextBit
	; Yes, get next byte
	ld	b, 8
	ld	c, (iy)
	inc	iy
.getNextBit:
	; Get next bit of input
	rrc	c
	jr	nc, .resetBit
	inc	hl
	ld	e, (hl)
.resetBit: ; Move to next node
	add	hl, de
	jr	.innerLoop
.deHuffmanDone:
	; Write decompressed code to output buffer
	ld	a, e
	and	7Fh
	jr	z, .notNbsp
	; TODO: This is a terrible hack to covert code 127 into a non-breaking space
	cp	7Fh
	jr	nz, .notNbsp
	add	a, 160 - 7Fh	; needs to become 160 & reset Z
.notNbsp:
	ld	(ix), a
	inc	ix
	jr	nz, .loop
	lea	hl, ix + 0
	pop	ix
	ret


;-------------------------------------------------------------------------------
_lcd_dim:
; Calls the OS's LCD idle dimming routines.
	ld	iy, flags
	jp	_DimLCDSlow


_lcd_bright:
; Restores the LCD brightness to the user's preference.
	ld	iy, flags
	jp	_RestoreLCDBrightness


;-------------------------------------------------------------------------------
_get_rtc_seconds:
; Returns the number of seconds elapsed since the start of the hour.
; Used by APD routines.
	ld	hl, rtc_Seconds
	ld	a, (hl)
	ld	de, (rtc_Minutes)
	ld	hl, (hl)
	cp	l
	jr	nz, _get_rtc_seconds
	ld	d, 60
	mlt	de
	add	hl, de
	ret


_get_rtc_seconds_plus:
; Returns the number of seconds elapsed since the start of the hour, plus an
; offset.  If the resulting time passes the end of the hour, wraps the time
; around.
; Used by APD routines.
	call	_get_rtc_seconds
	pop	bc
	pop	de
	push	de
	push	bc
	add	hl, de
	ld	de, 3600
	or	a
	sbc	hl, de
	ret	nc
	add	hl, de
	ret


;-------------------------------------------------------------------------------
_get_csc:
; Wraps GetCSC with two differences:
;  - Assumes you're using this in an input loop, so it does EI \ HALT to save a
;    tiny bit of power.
;  - If the ON key has been pressed and the user then presses CLEAR, aborts the
;    program immediately.
	ei
	halt
	call	_GetCSC
	cp	15		; skClear
	ret	nz
	ld	hl, flags + 9	; onFlags
	bit	4, (hl)		; onInterrupt
	ret	z
	jp	_exit_clean


_on_key_pressed:
; Returns 1 if the ON key has been pressed since the last time the
; ON-key-pressed flag was checked.
; Returns 0 if the ON key has not been pressed.
	ld	hl, flags + 9
	and	10h
	ret	z
	inc	a
	ret


_clear_on_key:
; Resets the ON-key-pressed flag.
	ld	hl, flags + 9	; iy + onFlags
	res	4, (hl)		; onInterrupt
	ret


;-------------------------------------------------------------------------------
;_get_compressed_string:
;	pop	de
;	pop	hl
;	push	hl
;	push	de
;	ld	de, (_compressed_strings)
;	jr	indexShortArray
	
;indexShortArray:
;	ld	a, l
;	or	h
;	ret	z
;	add	hl, hl
;	ex	de, hl
;	add	hl, de
;	ld	e, (hl)
;	inc	hl
;	ld	d, (hl)
;	ld	hl, (_dungeon)
;	add	hl, de
;	ret

