#include <assert.h>
#include <stdio.h>
#include "game.h"

static void quiet(Game *g) {
    unsigned i;
    for (i=0;i<BUG_COUNT;i++) {g->bugs[i].x=10<<4;g->bugs[i].y=28<<4;g->bugs[i].wait=255;}
    for (i=0;i<SPARK_COUNT;i++) {g->sparks[i].x=13;g->sparks[i].y=32;g->sparks[i].wait=255;}
}
static void collect(Game *g) {
    quiet(g);
    g->sparks[0].x=(g->x>>4)+8;g->sparks[0].y=(g->y>>4)+8;g->sparks[0].wait=0;
    game_tick(g,0);
}
static void collision(Game *g) {
    quiet(g);
    g->bugs[0].x=g->x+128;g->bugs[0].y=g->y+128;
    g->bugs[0].vx=g->bugs[0].vy=g->bugs[0].wait=0;
}
int main(void) {
    Game g,a,b;
    unsigned i,round;
    uint16_t score;
    int16_t x;
    game_init(&g,1);quiet(&g);
    assert(g.hearts==3 && game_seconds(&g)==60);
    x=g.x;game_tick(&g,INPUT_RIGHT);assert(g.x-x==23);
    x=g.x;game_tick(&g,INPUT_RIGHT|INPUT_FOCUS);assert(g.x-x==11);
    x=g.x;game_tick(&g,INPUT_RIGHT|INPUT_DASH);assert(g.x-x==80 && g.dash==11);
    for(i=0;i<60;i++){quiet(&g);game_tick(&g,INPUT_DASH);}
    assert(!g.dash && !g.cooldown); /* Holding A must not auto-dash. */
    game_tick(&g,0);game_tick(&g,INPUT_DASH);assert(g.dash==11);

    game_init(&g,2);
    for(i=0;i<5;i++) collect(&g);
    assert(g.collected==5 && g.bursts==1 && g.charge==0);
    assert(g.score==270 && (g.event&EVENT_BURST) && !g.cooldown);
    g.hearts=2;
    for(i=0;i<5;i++) collect(&g);
    assert(g.bursts==2 && g.hearts==3);
    for(i=0;i<18;i++) collect(&g);
    assert(g.multiplier==8);
    g.combo_timer=1;quiet(&g);game_tick(&g,0);
    assert(g.multiplier==1 && g.chain==0);
    g.score=65530;collect(&g);assert(g.score==65535);

    game_init(&g,3);collision(&g);game_tick(&g,0);
    assert(g.hearts==2 && (g.event&EVENT_HIT));
    collision(&g);game_tick(&g,0);assert(g.hearts==2);
    g.invincible=0;collision(&g);game_tick(&g,INPUT_DASH);
    assert(g.hearts==2 && g.popped==1 && (g.event&EVENT_POP));
    g.dash=g.invincible=0;g.hearts=1;collision(&g);game_tick(&g,0);
    assert(g.ended && !g.hearts && (g.event&EVENT_END));
    score=g.frame;game_tick(&g,INPUT_RIGHT);assert(g.frame==score);

    game_init(&g,4);g.frame=3599;quiet(&g);game_tick(&g,0);
    assert(g.ended && game_seconds(&g)==0 && g.hearts==3);
    /* Deterministic random-input rounds exercise boundaries and all timers. */
    for(round=1;round<=32;round++) {
        uint16_t rng=round;
        game_init(&a,round);game_init(&b,round);
        for(i=0;i<ROUND_FRAMES;i++) {
            uint8_t input;
            rng=(uint16_t)(rng*25173u+13849u);input=(rng>>8)&63;
            game_tick(&a,input);game_tick(&b,input);
            assert(a.x==b.x && a.y==b.y && a.score==b.score && a.rng==b.rng);
            assert(a.x>=5<<4 && a.x<=139<<4 && a.y>=24<<4 && a.y<=112<<4);
            assert(a.hearts<=3 && a.multiplier>=1 && a.multiplier<=8 && a.charge<5);
            if(a.ended) break;
        }
        assert(a.ended);
    }
    puts("PASS: movement, dash, combos, bursts, healing, damage, timeout, determinism, bounds");
}
