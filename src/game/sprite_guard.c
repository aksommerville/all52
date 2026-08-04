/* sprite_guard.c
 * Blocks the way until you have so many cards.
 */
 
#include "all52.h"

struct sprite_guard {
  struct sprite hdr;
  int limit;
};

#define SPRITE ((struct sprite_guard*)sprite)

static int _guard_init(struct sprite *sprite,const void *args,int argslen) {
  return 0;
}

static int _guard_bump(struct sprite *sprite,struct sprite *hero) {
  int cardc=hand_count_cards(sprite_hero_get_hand(hero));
  if (cardc<SPRITE->limit) {
    struct text_insertion ins={.mode='i',.i=SPRITE->limit};
    struct modal_args_dialogue args={
      .rid=1,
      .strix=24,
      .insv=&ins,
      .insc=1,
    };
    struct modal *modal=modal_spawn(&modal_type_dialogue,&args,sizeof(args));
    return 1;
  } else {
    struct modal_args_dialogue args={
      .rid=1,
      .strix=25,
    };
    struct modal *modal=modal_spawn(&modal_type_dialogue,&args,sizeof(args));
    sprite->defunct=1;
    return 1;
  }
}

static void _guard_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,0x34,0);
}

const struct sprite_type sprite_type_guard={
  .name="guard",
  .objlen=sizeof(struct sprite_guard),
  .init=_guard_init,
  .bump=_guard_bump,
  .render=_guard_render,
};

int sprite_guard_set_limit(struct sprite *sprite,int limit) {
  if (!sprite||(sprite->type!=&sprite_type_guard)) return -1;
  SPRITE->limit=limit;
  return 0;
}
