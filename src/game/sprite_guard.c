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
    fprintf(stderr,"NOPE!\n");
    return 1;
  } else {
    fprintf(stderr,"ok %d > %d\n",cardc,SPRITE->limit);
    sprite->defunct=1;
    return 0;
  }
  //TODO dialogue
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
