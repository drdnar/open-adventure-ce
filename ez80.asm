	.assume adl=1
	.def _decompress_string
	.ref _huffman_tree

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
	xor	a
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
	; Carry controls left or right branch, move to next node
	adc	hl, de
	jr	.innerLoop
.deHuffmanDone:
	; Write decompressed code to output buffer
	res	7, e
	ld	(ix), e
	inc	ix
	cp	e
	jr	nz, .loop
	lea	hl, ix + 0
	pop	ix
	ret



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

