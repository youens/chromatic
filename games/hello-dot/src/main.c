#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include "game.h"
#include "assets.h"

#define INK RGB(2,2,5)
#define CREAM RGB(30,29,25)
#define GOLD RGB(31,26,6)
#define MINT RGB(13,29,22)
#define LILAC RGB(22,18,31)
#define PINK RGB(31,12,21)

Game game;
uint16_t best_score = 0;
uint16_t menu_frame = 0;
uint8_t scene = 0, muted = 0, last_keys = 0, banner_time = 0;
uint8_t number_tiles[6];
const uint16_t bg_palettes[] = {
    INK, RGB(5,5,10), RGB(11,11,18), CREAM,
    INK, RGB(7,6,12), GOLD, CREAM,
    INK, RGB(6,8,11), RGB(6,17,15), MINT,
    INK, RGB(9,4,10), RGB(17,7,15), PINK,
    INK, RGB(7,5,13), GOLD, LILAC
};
const uint16_t spr_palettes[] = {
    INK, RGB(23,15,3), GOLD, RGB(3,3,5),
    INK, RGB(18,7,14), RGB(30,22,29), PINK,
    INK, RGB(5,18,15), MINT, CREAM,
    INK, RGB(14,9,21), LILAC, CREAM,
    INK, MINT, CREAM, RGB(4,11,10)
};
const uint16_t melody[] = {
    1751,0,1821,0,1860,1821,1751,0,
    1675,0,1751,0,1821,0,1860,0,
    1860,0,1891,0,1860,1821,1751,0,
    1675,1751,1821,0,1751,0,0,0
};

static void attributes(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t pal) {
    VBK_REG=1;
    fill_bkg_rect(x,y,w,h,pal);
    VBK_REG=0;
}

static void label(uint8_t x,uint8_t y,const char *s,uint8_t pal) {
    uint8_t n=0;
    while (*s && x+n<20) { set_bkg_tile_xy(x+n,y,(uint8_t)*s-31u);++s;++n; }
    attributes(x,y,n,1,pal);
}

static void number(uint8_t x,uint8_t y,uint16_t n,uint8_t width,uint8_t pal) {
    uint8_t i=width;
    while(i) {number_tiles[--i]=17u+n%10u;n/=10u;}
    set_bkg_tiles(x,y,width,1,number_tiles);
    attributes(x,y,width,1,pal);
}

static void hide_sprites(void) {
    uint8_t i;
    for(i=0;i<40;++i) hide_sprite(i);
}

static void sprite_at(uint8_t id,uint8_t tile,uint8_t x,uint8_t y,uint8_t pal) {
    set_sprite_tile(id,tile);
    set_sprite_prop(id,pal);
    move_sprite(id,x+8u,y+16u);
}

static void dot_at(uint8_t x,uint8_t y,uint8_t blink,uint8_t pal) {
    uint8_t tile=blink?SPR_BLINK:SPR_DOT;
    sprite_at(0,tile,x,y,pal);
    sprite_at(1,tile+2u,x+8u,y,pal);
}

static void clear_screen(void) {
    DISPLAY_OFF;
    hide_sprites();
    fill_bkg_rect(0,0,20,18,0);
    attributes(0,0,20,18,0);
}

static void sound_init(void) {
    NR52_REG=0x80;
    NR50_REG=0x66;
    NR51_REG=0xFF;
}

static void ping(uint16_t frequency,uint8_t envelope) {
    if(muted) return;
    NR10_REG=0;
    NR11_REG=0x80;
    NR12_REG=envelope;
    NR13_REG=(uint8_t)frequency;
    NR14_REG=0x80u|(uint8_t)(frequency>>8);
}

static void noise(uint8_t pitch,uint8_t envelope) {
    if(muted) return;
    NR41_REG=0x10;
    NR42_REG=envelope;
    NR43_REG=pitch;
    NR44_REG=0x80;
}

static void music_tick(void) {
    uint16_t f;
    uint8_t step;
    if(muted || game.frame%12u) return;
    step=(game.frame/12u)&31u;
    f=melody[step];
    if(f) {
        NR21_REG=0x80;
        NR22_REG=0x42;
        NR23_REG=(uint8_t)f;
        NR24_REG=0x80u|(uint8_t)(f>>8);
    }
    if(!(step&3u)) noise(0x35,0x31);
}

static void show_title(void) {
    scene=0;
    clear_screen();
    label(2,0,"A TINY GOOD TIME",4);
    set_bkg_data(128,LOGO_TILE_COUNT,logo_tiles);
    set_bkg_tiles(0,2,20,6,logo_map);
    attributes(0,2,20,6,1);
    label(3,11,"BIG HELLOS.",4);
    label(3,12,"LITTLE HERO.",4);
    label(4,14,"A / START PLAY",2);
    label(5,16,"B HOW TO PLAY",0);
    label(4,17,"BEST",4);number(10,17,best_score,5,1);
    dot_at(72,70,0,0);
    DISPLAY_ON;
}

static void show_help(void) {
    scene=1;
    clear_screen();
    label(4,1,"SAY HELLO!",1);
    label(1,3,"D-PAD  MOVE",0);
    label(1,5,"A      DASH",2);
    label(1,6,"DASH THROUGH BUGS",0);
    label(1,8,"B      SLOW DOWN",0);
    label(1,10,"5 SPARKS = BURST!",1);
    label(1,11,"2 BURSTS = HEART",3);
    label(1,13,"CHAIN FOR UP TO X8",4);
    label(1,14,"START PAUSE",0);
    label(1,15,"SELECT SOUND",0);
    label(4,17,"A / START GO!",2);
    DISPLAY_ON;
}

static void arena(void) {
    uint8_t x,y;
    clear_screen();
    for(y=3;y<16;++y) for(x=0;x<20;++x)
        if(((x+y)&3u)==0u) set_bkg_tile_xy(x,y,TILE_DUST);
    fill_bkg_rect(0,16,20,1,TILE_LINE);
    label(0,0,"SCORE",4);label(16,0,"TIME",4);
    label(0,2,"5 SPARKS = HELLO!",1);
    label(0,17,"A",2); label(8,17,"HELLO",4);
    DISPLAY_ON;
}

static void hud(void) {
    uint8_t i;
    number(0,1,game.score,5,1);
    number(17,1,game_seconds(&game),2,game_seconds(&game)<=10u?3:0);
    for(i=0;i<3;++i) set_bkg_tile_xy(8u+i,0,i<game.hearts?TILE_HEART:0);
    attributes(8,0,3,1,3);
    label(8,1,"X",4);number(9,1,game.multiplier,1,1);
    for(i=0;i<5;++i) {
        set_bkg_tile_xy(14u+i,17,i<game.charge?TILE_PIP:TILE_EMPTY_PIP);
        set_bkg_tile_xy(2u+i,17,game.cooldown < (5u-i)*10u ? TILE_BAR:TILE_EMPTY_PIP);
    }
    attributes(2,17,5,1,2); attributes(14,17,5,1,1);
}

static void draw_game(void) {
    uint8_t i,pal;
    if(game.invincible && !game.dash && (game.invincible&4u)) {hide_sprite(0);hide_sprite(1);}
    else dot_at(game.x>>4,game.y>>4,(game.frame%150u)>143u,game.dash?4:0);
    for(i=0;i<SPARK_COUNT;++i) {
        Spark *s=&game.sparks[i];
        if(s->wait) hide_sprite(i+2u);
        else sprite_at(i+2u,((game.frame/12u+i)&1u)?SPR_SPARK:SPR_SPARK_ALT,s->x-4u,s->y-4u,2);
    }
    for(i=0;i<BUG_COUNT;++i) {
        Bug *b=&game.bugs[i];
        if(i>=game.active_bugs) hide_sprite(i+6u);
        else {
            pal=b->wait?3:1;
            sprite_at(i+6u,b->wait?SPR_WARNING:((game.frame&8u)?SPR_BUG:SPR_BUG_ALT),
                (b->x>>4)-4u,(b->y>>4)-4u,pal);
        }
    }
    if(game.event&EVENT_BURST) {
        label(0,2,"HELLO! NICE BURST!   ",2);
        banner_time=55;
        ping(1950,0xA4);
    } else if(game.event&EVENT_HIT) {
        label(0,2,"OUCH! KEEP GOING.    ",3);banner_time=50;noise(0x65,0xA3);
    } else if(game.event&EVENT_POP) {
        label(0,2,"BUG POP! +BONUS      ",2);banner_time=35;ping(1910,0x92);
    } else if(game.event&EVENT_PICK) ping(1751u+game.multiplier*24u,0x82);
    else if(game.event&EVENT_DASH) noise(0x24,0x72);
    if(banner_time && !--banner_time) label(0,2,"5 SPARKS = HELLO!    ",1);
    if(game.flash) {
        uint16_t color=game.flash&2u?RGB(7,10,13):INK;
        set_bkg_palette_entry(0,0,color);
    } else set_bkg_palette_entry(0,0,INK);
    hud();
}

static void start_game(void) {
    game_init(&game,menu_frame^DIV_REG^0xD071u);
    game.last_input=(last_keys&J_A)?INPUT_DASH:0;
    banner_time=0;
    arena();
    scene=2;
    hud();
    ping(1860,0x82);
}

static void show_results(void) {
    uint8_t record=game.score>best_score;
    if(record) best_score=game.score;
    scene=4;
    clear_screen();
    set_bkg_palette_entry(0,0,INK);
    label(3,1,game.hearts?"TIME WELL SPENT!":"ONE MORE HELLO?",1);
    label(5,4,"YOUR SCORE",4);
    number(6,6,game.score,5,1);
    if(record) label(5,8,"NEW BEST!",2);
    else {label(3,8,"BEST",4);number(9,8,best_score,5,0);}
    label(3,10,"SPARKS",0);number(13,10,game.collected,3,2);
    label(3,11,"BUG POPS",0);number(13,11,game.popped,3,3);
    label(3,12,"HELLOS",0);number(13,12,game.bursts,3,1);
    label(2,15,"A / START AGAIN",2);
    label(4,17,"B TITLE SCREEN",4);
    DISPLAY_ON;
    ping(game.hearts?1860:1500,0xA4);
}

void main(void) {
    uint8_t keys,pressed,input;
    DISPLAY_OFF;
    cpu_fast();
    set_bkg_data(0,BG_TILE_COUNT,bg_tiles);
    set_sprite_data(0,SPRITE_TILE_COUNT,sprite_tiles);
    set_bkg_palette(0,5,bg_palettes);
    set_sprite_palette(0,5,spr_palettes);
    SPRITES_8x16;
    SHOW_BKG;SHOW_SPRITES;HIDE_WIN;
    sound_init();
    show_title();
    for(;;) {
        vsync();
        ++menu_frame;
        keys=joypad();pressed=keys&~last_keys;last_keys=keys;
        if(pressed&J_SELECT) {muted=!muted;NR51_REG=muted?0:0xFF;}
        if(scene==0) {
            dot_at(72,70u+((menu_frame>>4)&1u),menu_frame%160u>153u,0);
            if(pressed&(J_A|J_START)) start_game();
            else if(pressed&J_B) show_help();
        } else if(scene==1) {
            if(pressed&(J_A|J_START)) start_game();
            else if(pressed&J_B) show_title();
        } else if(scene==2) {
            if(pressed&J_START) {scene=3;label(0,2,"PAUSED  START RESUME ",4);NR51_REG=0;continue;}
            input=0;
            if(keys&J_UP) input|=INPUT_UP;
            if(keys&J_DOWN) input|=INPUT_DOWN;
            if(keys&J_LEFT) input|=INPUT_LEFT;
            if(keys&J_RIGHT) input|=INPUT_RIGHT;
            if(keys&J_A) input|=INPUT_DASH;
            if(keys&J_B) input|=INPUT_FOCUS;
            game_tick(&game,input);
            if(game.ended) show_results();
            else {music_tick();draw_game();}
        } else if(scene==3) {
            if(pressed&J_START) {scene=2;label(0,2,"5 SPARKS = HELLO!    ",1);NR51_REG=muted?0:0xFF;}
        } else if(scene==4) {
            if(pressed&(J_A|J_START)) start_game();
            else if(pressed&J_B) show_title();
        }
    }
}
