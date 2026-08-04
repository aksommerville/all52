/* sprite_card.c
 * A stationary card that you can pick up.
 */
 
#include "all52.h"

struct sprite_card {
  struct sprite hdr;
  uint64_t hand; // Should be just one bit.
};

#define SPRITE ((struct sprite_card*)sprite)

/* Init.
 */
 
static int _card_init(struct sprite *sprite,const void *args,int argslen) {
  return 0;
}

/* Bump.
 */
 
static int _card_bump(struct sprite *sprite,struct sprite *hero) {
  uint64_t herohand=sprite_hero_get_hand(hero);
  hand_log("hero before card",herohand);
  hand_log("card hand",SPRITE->hand);
  herohand|=SPRITE->hand;
  hand_log("hero after picking up",herohand);
  sprite_hero_set_hand(hero,herohand);
  sprite->defunct=1;
  //TODO fanfare
  return 0;
}

/* Render.
 */
 
static void _card_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,0x72,0);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_card={
  .name="card",
  .objlen=sizeof(struct sprite_card),
  .init=_card_init,
  .bump=_card_bump,
  .render=_card_render,
};

/* Public accessors.
 */

uint64_t sprite_card_get_hand(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_card)) return 0;
  return SPRITE->hand;
}

int sprite_card_set_hand(struct sprite *sprite,uint64_t hand) {
  if (!sprite||(sprite->type!=&sprite_type_card)) return -1;
  SPRITE->hand=hand;
  return 0;
}
