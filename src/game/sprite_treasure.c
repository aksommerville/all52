/* sprite_treasure.c
 * Bump to get the hat.
 */
 
#include "all52.h"

struct sprite_treasure {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_treasure*)sprite)

static int _treasure_bump(struct sprite *sprite,struct sprite *hero) {
  g.hat=1;
  sprite->defunct=1;
  struct modal_args_dialogue args={
    .rid=1,
    .strix=29,
    .suppress_sound=1,
  };
  modal_spawn(&modal_type_dialogue,&args,sizeof(args));
  SFX(get_card)
  return 1;
}

static void _treasure_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,0x53,0);
}

const struct sprite_type sprite_type_treasure={
  .name="treasure",
  .objlen=sizeof(struct sprite_treasure),
  .bump=_treasure_bump,
  .render=_treasure_render,
};
