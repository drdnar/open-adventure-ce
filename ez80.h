#ifndef EZ80_H
#define EZ80_H

extern uint8_t* huffman_tree;
char* decompress_string(void* input, char* output/*, void* tree*/);
void lcd_dim(void);
void lcd_bright(void);
unsigned int get_rtc_seconds(void);
unsigned int get_rtc_seconds_plus(unsigned int offset);
unsigned char get_csc(void);
bool on_key_pressed(void);
void clear_on_key(void);

#endif
