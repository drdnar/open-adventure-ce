include 'ez80.inc'
	.assume adl=1
	.def _decompress_string
	.def _huffman_tree

_decompress_string:
; Inputs:
;  - arg0: Pointer to input Huffman bit stream
;  - arg1: Pointer to target buffer
;;  - arg2: Pointer to serialized Huffman tree
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
.loop:	
;	ld	hl, 9
;	add	hl, sp
;	ld	hl, (hl)
	ld	hl, 0
_huffman_tree := $ - 2
.innerLoop:
	; Is this node a leaf?
	ld	a, (hl)
	bit	7, a
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
	; Branch right or left?
	jr	z, .branchLeft
	inc	hl
.branchLeft:
	; Move to next node
	ld	e, (hl)
	add	hl, de
	jr	.innerLoop
.deHuffmanDone:
	; Write decompressed code to output buffer
	and	7Fh
	ld	(ix), a
	inc	ix
	or	a
	jr	nz, .loop
	lea	hl, ix + 0
	pop	ix
	ret
