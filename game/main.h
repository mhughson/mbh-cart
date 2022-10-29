/** (C) Matt Hughson 2020 */

// Note: all variables need extern and should be defined in main.c

#ifndef ONCE_MAIN_H
#define ONCE_MAIN_H

#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
#define PROFILE_POKE(val) POKE((0x2001),(val));
#else
#define PROFILE_POKE(val)
#endif

#define PROF_CLEAR 0x1e // none
#define PROF_R_TINT 0x3e // red
#define PROF_G_TINT 0x5e // green
#define PROF_B_TINT 0x9e // blue
#define PROF_W_TINT 0x1e // white
#define PROF_R 0x3f // red + grey
#define PROF_G 0x5f // green + grey
#define PROF_B 0x9f // blue + grey
#define PROF_W 0x1f // white + grey

enum { BANK_0, BANK_1, BANK_2 };

#define ROOM_WIDTH_PAGES (1)
#define ROOM_WIDTH_PIXELS (256*ROOM_WIDTH_PAGES)
#define ROOM_WIDTH_TILES (16*ROOM_WIDTH_PAGES)
#define GRID_XY_TO_ROOM_INDEX(x,y) (((y) * ROOM_WIDTH_TILES) + (x))
#define CELL_SIZE (16)

#define META_TILE_FLAGS_OFFSET (5)
#define META_TILE_NUM_BYTES (8)
#define END_OF_META (128)
#define META_TILE_SET_NUM_BYTES (128*META_TILE_NUM_BYTES)

#define FLAG_WALL 			(1 << 0)
#define FLAG_FLOOR			(1 << 1)
#define FLAG_COLLECTIBLE	(1 << 2)
#define FLAG_KILL			(1 << 3)
#define FLAG_DOWN_LEFT		(1 << 4)
#define FLAG_DOWN_RIGHT		(1 << 5)
#define FLAG_ENTRY			(1 << 7)

#define P1_MOVE_SPEED (FP_WHOLE(1))
#define BOULDER_MOVE_SPEED (FP_WHOLE(1) + FP_0_15)
#define BOULDER_FALL_SPEED (FP_0_025)

// Fixed Point Math Helpers

#define FP_0_025 (6)
#define FP_0_05 (13)
#define FP_0_10 (26)
#define FP_0_15 (38)
#define FP_0_20 (51)
#define FP_0_25 (64)
#define FP_0_50 (128)
#define FP_0_75 (192)
#define HALF_POS_BIT_COUNT (8)
#define FP_WHOLE(x) ((x)<<HALF_POS_BIT_COUNT)

// Tunables
#define JUMP_VEL (FP_WHOLE(2) + 0) // 2
#define JUMP_HOLD_MAX (10) // 10
#define GRAVITY (FP_0_25) // .25
#define GRAVITY_LOW (FP_0_05)
#define WALK_SPEED (FP_WHOLE(1) + FP_0_50)
#define JUMP_COYOTE_DELAY (8)
#define ATTACK_LEN (5)

#define NUM_BG_BANKS 2 // MUST BE POWER OF 2

#define NUM_SCORE_DIGITS (5)

#define NUM_ACTORS (6)
// subracting this from the tile id from the name table, will give you the OBJ_TYPE
#define TYPE_ID_START_INDEX_TILED (112)

enum STATES 
{
	STATE_BOOT, STATE_TITLE, STATE_GAMEPLAY, STATE_GAMEOVER, STATE_LEVEL_COMPLETE
};

enum OBJ_TYPES
{
	TYPE_NONE, TYPE_PLAYER, TYPE_GOBLIN, TYPE_BOULDER,
};

typedef struct anim_info
{
	// index into sprite_anims array.
	unsigned char anim_current;
	unsigned char anim_queued;

	// how many ticks have passed since the last frame change.
	unsigned char anim_ticks;

	// the currently displaying frame of the current anim.
	unsigned char anim_frame;
} anim_info;

// data speciic to player game objects.
typedef struct animated_sprite
{
	// Was the last horizontal move *attempted* in the left direction?
	unsigned char facing_left;

	// Stores all of the active animation info.
	anim_info anim;

} animated_sprite;

typedef struct game_actor
{
	animated_sprite sprite;

	unsigned int pos_x;
	unsigned int pos_y;

    signed int vel_x;
	signed int vel_y;

	unsigned char facing_left;

	unsigned char is_dead;

	unsigned char type;
} game_actor;

#pragma bss-name(push, "ZEROPAGE")

extern unsigned char i;
extern unsigned char index;
extern unsigned int index16;
extern unsigned char x;
extern unsigned char y;
extern unsigned char tick_count;
extern unsigned char pads;
extern unsigned char pads_new;
extern unsigned int px_old;
extern unsigned int py_old;
extern unsigned char in_oam_x;
extern unsigned char in_oam_y;
extern const unsigned char *in_oam_data;
extern game_actor player1;
extern unsigned int temp16;
extern unsigned char tempFlags;
extern unsigned char tempFlagsDown;
extern unsigned char tempFlagsUp;
extern unsigned char ticks_since_attack;
extern unsigned char temp_on_ramp;
extern unsigned char is_jumping;
extern unsigned char is_on_ramp;
extern unsigned char current_room[240];
// Used by the anim functions to avoid passing in a parameter.
extern anim_info* global_working_anim;
extern unsigned char score_bcd[NUM_SCORE_DIGITS];
extern unsigned char score_best_bcd[NUM_SCORE_DIGITS];
extern char in_x_tile; 
extern char in_y_tile;
extern unsigned char char_state;
extern unsigned char cur_state;
extern unsigned char gems_remaining;
extern unsigned char grounded;
extern unsigned char cur_room_index;
extern signed char cur_time_digits[6];
extern unsigned char timer_expired;
extern unsigned char loaded_obj_index;
extern unsigned char loaded_obj_id;

extern game_actor* in_obj_a;
extern game_actor* in_obj_b;
extern game_actor objs[NUM_ACTORS];
// load_current_map
extern unsigned int in_nametable;
// add_bcd_score
extern const unsigned char * in_points;
// Counter used to ensure each sound effect ends up on a different
// channel. (no regard to priority)
extern unsigned char cur_sfx_chan;


#pragma bss-name(pop)

void c_oam_meta_spr_flipped(void);
unsigned char update_anim();
void queue_next_anim(unsigned char next_anim_index);
void commit_next_anim();
void vram_buffer_load_2x2_metatile();
void go_to_state(unsigned char next_state);
void add_bcd_score();
// Params:	in_obj_a
//			in_obj_b
unsigned char intersects_box_box();
void kill_player();
void fade_to_black();
void fade_from_black();
void display_score();
void display_score_ppu_off();
void display_score_best_ppu_off();
void draw_cur_time();
void draw_cur_time_ppu_off();
void draw_statusbar_ppu_off();


// TODO: Where is non-zero page? Is this just starting at zero page?

#endif // ONCE_MAIN_H



/*
/// TODO

// Core Loop
[done]


// Must Have
* [later] Actual level design.

// Should Have
* Additional enemies
* Score multiplier to reward skill play
* Level complete screen++
* Countdown Timer (with score associated)
* Pause
* Settings (Music/Sound disable)
* Auto-advance credits.
* [Bug] Tile lookup on edges is broken.
* [Bug] Can fall into ramp if bounching off wall after launching up from ramp.
* [Bug] Sometimes, when going slowly down ramp, then speeding up, player pops back up to upper rail.
* [Bug] Title sprites are seen on just after fade in to first level.


// Nice to Have
* Palette changes for level
* Big countdown text for final seconds.
* Title screen nose/mouth overlay.
* Timed collectibles (cherries, etc)
* Score tally on Level complete screen.
* Score tally on Game Over screen.
* Point kickers
* Stunned animation for Goblin
* Brake sparks.
* Death animation.
* Difficulty.
* Custom header tiles for HUD.
* Time sound effect on low time.
* Delay on starting gameplay (flashing player, etc)


///
*/