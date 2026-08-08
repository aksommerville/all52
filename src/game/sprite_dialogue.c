/* sprite_dialogue.c
 * A sprite that just spouts canned dialogue when bumped.
 */
 
#include "all52.h"

struct sprite_dialogue {
  struct sprite hdr;
  int strix;
  uint8_t tileid;
};

#define SPRITE ((struct sprite_dialogue*)sprite)

static int _dialogue_init(struct sprite *sprite) {
  SPRITE->tileid=0x44;
  return 0;
}

static int _dialogue_bump(struct sprite *sprite,struct sprite *hero) {
  struct modal_args_dialogue args={
    .rid=1,
    .strix=SPRITE->strix,
  };
  struct modal *modal=modal_spawn(&modal_type_dialogue,&args,sizeof(args));
  return 1;
}

static void _dialogue_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,SPRITE->tileid,0);
}

const struct sprite_type sprite_type_dialogue={
  .name="dialogue",
  .objlen=sizeof(struct sprite_dialogue),
  .init=_dialogue_init,
  .bump=_dialogue_bump,
  .render=_dialogue_render,
};

int sprite_dialogue_set_strix(struct sprite *sprite,int strix) {
  if (!sprite||(sprite->type!=&sprite_type_dialogue)) return -1;
  SPRITE->strix=strix;
  return 0;
}
