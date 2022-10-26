/** (C) Matt Hughson 2020 */

#include "LIB/neslib.h"
#include "LIB/nesdoug.h"
#include "A53/bank_helpers.h"
#include "../include/stdlib.h"

#include "main.h"
#include "PRG0.h"

#pragma rodata-name ("CODE")
#pragma code-name ("CODE")

const unsigned char palette[16]={ 0x0f,0x0f,0x13,0x37,0x0f,0x17,0x29,0x20,0x0f,0x13,0x23,0x33,0x0f,0x14,0x24,0x34 };
const unsigned char palette_bg[16]={ 0x0f,0x07,0x17,0x10,0x0f,0x03,0x11,0x35,0x0f,0x07,0x16,0x38,0x0f,0x09,0x19,0x29 };

const unsigned char palette_title_bg[16]={ 0x0f,0x17,0x00,0x10,0x0f,0x16,0x00,0x37,0x0f,0x19,0x17,0x38,0x0f,0x17,0x13,0x37 };

// face
const unsigned char title_screen_Metasprite0_data[]={

	- 8,-12,0x01,2,
	  0,-12,0x02,2,
	- 8,- 4,0x11,2,
	  0,- 4,0x12,2,

	- 8,  4,0x21,2,
	  0,  4,0x22,2,
	0x80

};

// diamond
const unsigned char title_screen_Metasprite1_data[]={

	-12,- 8,0x03,1,
	- 4,- 8,0x04,1,
	  4,- 8,0x05,1,
	-12,  0,0x13,1,

	- 4,  0,0x14,1,
	  4,  0,0x15,1,
	0x80

};

const unsigned char* const title_screen_list[]={

	title_screen_Metasprite0_data,
	title_screen_Metasprite1_data

};

const char ease_pulse[4] = {-1, 0, 1, 0};


// Initalized RAM variables
//

unsigned char i;
unsigned char index;
unsigned int index16;
unsigned char x;
unsigned char y;
unsigned char tick_count;
unsigned char pads;
unsigned char pads_new;
unsigned int px;
unsigned int py;
unsigned int px_old;
unsigned int py_old;
signed int dx;
signed int dy;
unsigned char in_oam_x;
unsigned char in_oam_y;
const unsigned char *in_oam_data;
game_actor player1;
game_actor enemy1;
unsigned int temp16;
unsigned char tempFlags;
unsigned char tempFlagsUp;
unsigned char tempFlagsDown;
unsigned char ticks_since_attack;
unsigned char temp_on_ramp;
unsigned char is_jumping;
unsigned char is_on_ramp;
unsigned char current_room[240];
// Used by the anim functions to avoid passing in a parameter.
anim_info* global_working_anim;
unsigned char score_bcd[NUM_SCORE_DIGITS];
char in_x_tile; 
char in_y_tile;
unsigned char cur_sfx_chan;
unsigned char char_state;
unsigned char cur_state;
unsigned char gems_remaining;
unsigned char cur_room_index;

game_actor* in_obj_a;
game_actor* in_obj_b;
unsigned int in_nametable;
const unsigned char * in_points;


// batch add
// unsigned char anim_index;
unsigned char grounded;
// unsigned char jump_held_count;
// unsigned char can_jump;
// unsigned char airtime;
// unsigned char ticks_down;
// unsigned char jump_count;
// unsigned char on_ground;
// unsigned char new_jump_btn;
// unsigned int scroll_y;

#pragma rodata-name ("CODE")
#pragma code-name ("CODE")

// function prototypes
//void load_current_map(unsigned int nt);
//void update_player();
//void update_gameplay();

// It seems like main() MUST be in fixed back!
void main (void) 
{
	ppu_off(); // screen off

	set_vram_buffer(); // do at least once, sets a pointer to a buffer
	clear_vram_buffer();

	set_chr_bank_0(0);
	bank_bg(0);
	bank_spr(1);

	pal_bg(palette_bg);
	pal_spr(palette);

	cur_state = 0xff;
	go_to_state(STATE_BOOT);

	// infinite loop
	while (1)
	{
		++tick_count;

		ppu_wait_nmi(); // wait till beginning of the frame

		oam_clear();

		pads = pad_poll(0) | pad_poll(1); // read the first controller
		pads_new = get_pad_new(0) | get_pad_new(1); // newly pressed button. do pad_poll first

		clear_vram_buffer(); // do at the beginning of each frame

		switch (cur_state)
		{
			case STATE_BOOT:
			{
				if (pads_new & PAD_ANY_CONFIRM_BUTTON)
				{
					go_to_state(STATE_TITLE);
				}
				break;
			}

			case STATE_TITLE:
			{
				if (pads_new & PAD_ANY_CONFIRM_BUTTON)
				{
					go_to_state(STATE_GAMEPLAY);
				}

				oam_meta_spr(140, 128 + (ease_pulse[(tick_count / 16) % 4]), title_screen_Metasprite1_data);
				break;
			}

			case STATE_GAMEPLAY:
			{
				banked_call(BANK_0, update_gameplay);
				break;
			}

			case STATE_GAMEOVER:
			{
				if (pads_new & PAD_ANY_CONFIRM_BUTTON)
				{
					go_to_state(STATE_TITLE);
				}				
				break;
			}

			case STATE_LEVEL_COMPLETE:
			{
				if (pads_new & PAD_ANY_CONFIRM_BUTTON)
				{
					go_to_state(STATE_GAMEPLAY);
				}				
				break;
			}
		}
	}
}



void c_oam_meta_spr_flipped(void)
{
	static const unsigned char* _elem;
	static unsigned char _x;
	static unsigned char _y;
	static unsigned char _sprite;
	static unsigned char _att;

	if (in_oam_data == NULL)
	{
		return;
	}

	_elem = in_oam_data;

	while (_elem[0] != END_OF_META)
	{
		_x = 8 - _elem[0];
		_y = _elem[1];
		_sprite = _elem[2];
		_att = _elem[3] | OAM_FLIP_H;
		_elem += 4;
		oam_spr(in_oam_x + _x, in_oam_y + _y, _sprite, _att);
	}
}

void queue_next_anim(unsigned char next_anim_index)
{
	global_working_anim->anim_queued = next_anim_index;
}

void commit_next_anim()
{
	if (global_working_anim->anim_queued != 0xff && global_working_anim->anim_queued != global_working_anim->anim_current)
	{
		global_working_anim->anim_current = global_working_anim->anim_queued;
		global_working_anim->anim_frame = 0;
		global_working_anim->anim_ticks = 0;
	}

	global_working_anim->anim_queued = 0xff;
}

void go_to_state(unsigned char next_state)
{
	if (next_state == cur_state)
	{
		return;
	}

	switch (next_state)
	{	
		case STATE_TITLE:
		{
			// reset the score as we leave the title screen, rather than entering
			// gameplay, since that would reset the score when reaching next level
			// as well.
			memfill(score_bcd, 0, NUM_SCORE_DIGITS);

			cur_room_index = 0;
		}
		default:
		{
			break;
		}
	}

	cur_state = next_state;

	switch (cur_state)
	{
		case STATE_BOOT:
		{
			banked_call(BANK_0, load_screen_boot);
			break;
		}

		case STATE_TITLE:
		{
			set_chr_bank_0(2);
			bank_bg(0);
			bank_spr(1);

			pal_bg(palette_title_bg);
			pal_spr(palette_bg);			

			banked_call(BANK_0, load_screen_title);
			music_play(1);
			break;
		}

		case STATE_GAMEPLAY:
		{
			fade_to_black();

			ppu_off();

			set_chr_bank_0(0);
			bank_bg(0);
			bank_spr(1);

			pal_bg(palette_bg);
			pal_spr(palette);			

			in_nametable = NAMETABLE_A;
			banked_call(BANK_0, load_current_map);

			// vram_adr(NTADR_A(7, 2));
			// vram_write("NESDEV COMPO 2022", 17);

			vram_adr(NTADR_A(2, 1));
			vram_write("SCORE", 5);

			ppu_on_all(); // turn on screen

			// todo: non-vram version!
			display_score();

			px = 128 << 8;
			py = (6 * 16) << 8;
			dx = P1_MOVE_SPEED;
			dy = 0;
			is_jumping = 0;

			player1.pos_x = FP_WHOLE(4 * 16);
			player1.pos_y = FP_WHOLE(9 * 16);
			player1.vel_x = P1_MOVE_SPEED;
			player1.vel_y = 0;
			player1.is_dead = 0;

			enemy1.pos_x = FP_WHOLE(2 * 16);
			enemy1.pos_y = FP_WHOLE(9 * 16);
			enemy1.vel_x = P1_MOVE_SPEED;
			enemy1.vel_y = 0;
			enemy1.is_dead = 0;

			fade_from_black();
			music_play(0);

			break;
		}

		case STATE_LEVEL_COMPLETE:
		{
			++cur_room_index;
			music_play(2);
			delay(120);
			banked_call(BANK_0, load_screen_levelcomplete);
			break;
		}

		case STATE_GAMEOVER:
		{
			banked_call(BANK_0, load_screen_gameover);
			music_stop();
			break;
		}

		default:
		{
			break;
		}
	}
}

// box to box intersection

unsigned char intersects_box_box()
{
	static signed int _pos_delta;
	static signed int _dist_sum;
	static signed int _width_half = 8;
	static signed int _height_half = 8;

	_pos_delta = (high_byte(in_obj_a->pos_x) + _width_half) - (high_byte(in_obj_b->pos_x) + _width_half);
	_dist_sum = _width_half + _width_half;
	if( abs(_pos_delta) >= _dist_sum ) return 0;

	_pos_delta = (high_byte(in_obj_a->pos_y) + _height_half) - (high_byte(in_obj_b->pos_y) + _height_half);
	_dist_sum = _height_half + _height_half;
	if( abs(_pos_delta) >= _dist_sum ) return 0;
	
	return 1;
}

void kill_player()
{
	music_stop();
	sfx_play(21, 0);

	player1.vel_x = 0;
	player1.vel_y = -FP_WHOLE(3);
	player1.is_dead = 1;
	//go_to_state(STATE_GAMEOVER);
}

#define FADE_DELAY 2
void fade_to_black()
{
	pal_bright(3);
	delay(FADE_DELAY);
	pal_bright(2);
	delay(FADE_DELAY);
	pal_bright(1);
	delay(FADE_DELAY);
	pal_bright(0);
//	delay(FADE_DELAY);
}

void fade_from_black()
{
	pal_bright(1);
	delay(FADE_DELAY);
	pal_bright(2);
	delay(FADE_DELAY);
	pal_bright(3);
	delay(FADE_DELAY);
	pal_bright(4);
//	delay(FADE_DELAY);
}

void add_bcd_score()
{
	static unsigned char _carry;
	static signed char _i;

	for (_i = (NUM_SCORE_DIGITS - 1); _i >= 0; --_i)
	{
		unsigned char _sum = in_points[_i] + score_bcd[_i] + _carry;
		_carry = 0;
		if (_sum >= 10)
		{
			_sum -= 10;
			_carry = 1;
		}
		
		score_bcd[_i] = _sum;
	}
}

void display_score()
{
	static unsigned char _display_score[NUM_SCORE_DIGITS + 2];


	for (i = 0; i < NUM_SCORE_DIGITS; ++i)
	{
		_display_score[i] = score_bcd[i] + '0';
		//one_vram_buffer(score_bcd[i] + '0', get_ppu_addr(0, 2<<3, 2<<3));
	}
	_display_score[NUM_SCORE_DIGITS] = '0';
	_display_score[NUM_SCORE_DIGITS + 1] = '0';
	multi_vram_buffer_horz(_display_score, NUM_SCORE_DIGITS + 2, get_ppu_addr(0, 2<<3, 2<<3));
}

// const unsigned int _digit_shift_table[NUM_SCORE_DIGITS] =
// {
// 		  1,
// 		 10,
// 	    100,
// 	   1000,
// 	  10000,
//     //  100000,
// 	// 1000000,
// };

// void __display_score()
// {

// 	static unsigned int _temp_score;
// 	static signed char _i;
// 	static unsigned _digit;
// 	static unsigned char _score_display[NUM_SCORE_DIGITS];
// 	static unsigned int _digit_shift;

// 	PROFILE_POKE(PROF_R);
	
// 	_temp_score = score;

// 	i = 0;
// 	for (_i = (NUM_SCORE_DIGITS - 1); _i >= 0; --_i)
// 	{
// 		_digit = 0;
// 		_digit_shift = _digit_shift_table[_i];

// 		if (_temp_score >= (8 * _digit_shift)) 
// 		{
// 			_digit |= 8;
// 			_temp_score -= (8 * _digit_shift);
// 		}
// 		if (_temp_score >= (4 * _digit_shift)) 
// 		{
// 			_digit |= 4;
// 			_temp_score -= (4 * _digit_shift);
// 		}
// 		if (_temp_score >= (2 * _digit_shift)) 
// 		{
// 			_digit |= 2;
// 			_temp_score -= (2 * _digit_shift);
// 		}
// 		if (_temp_score >= (1 * _digit_shift)) 
// 		{
// 			_digit |= 1;
// 			_temp_score -= (1 * _digit_shift);
// 		}
// 		_score_display[i++] = '0' + _digit;		
// 	}

// 	multi_vram_buffer_horz(_score_display, NUM_SCORE_DIGITS, get_ppu_addr(0, 2<<3, 2<<3));
// 	multi_vram_buffer_horz("00", 2, get_ppu_addr(0, 7<<3, 2<<3));
// 	PROFILE_POKE(PROF_CLEAR);
// }

// void ___display_score()
// {
// 	static unsigned long temp_score;
// 	static unsigned char i;
// 	static unsigned char digit;

// 	PROFILE_POKE(PROF_R);

// 	temp_score = score;

// 	// clear out any old score.
// 	multi_vram_buffer_horz("0000000", 7, get_ppu_addr(0, 2<<3, 2<<3));

// 	i = 0;
// 	while(temp_score != 0)
// 	{
// 		digit = temp_score % 10;
// 		one_vram_buffer('0' + digit, get_ppu_addr(0, (8<<3) - (i << 3), 2<<3 ));

// 		temp_score = temp_score / 10;
// 		++i;
// 	}

// 	PROFILE_POKE(PROF_CLEAR);
// }