/* sprite_ghost.c
 * Spawned by monster after it's dispatched.
 */
 
#include "all52.h"

struct sprite_ghost {
  struct sprite hdr;
  double ttl;
  double dx,dy;
  double turnclock;
};

#define SPRITE ((struct sprite_ghost*)sprite)

static int _ghost_init(struct sprite *sprite) {
  SPRITE->ttl=2.000;
  SPRITE->dy=-5.000;
  SPRITE->dx=1.000+((rand()&0xffff)*3.000)/65535.0;
  if (rand()&1) SPRITE->dx*=-1.0;
  SPRITE->turnclock=0.250+((rand()&0xffff)*0.500)/65535.0;
  return 0;
}

static void _ghost_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    sprite->defunct=1;
    return;
  }
  if ((SPRITE->turnclock-=elapsed)<=0.0) {
    SPRITE->turnclock+=0.250+((rand()&0xffff)*0.500)/65535.0;
    SPRITE->dx*=-1.0;
  }
  sprite->x+=SPRITE->dx*elapsed;
  sprite->y+=SPRITE->dy*elapsed;
}

static void _ghost_render(struct sprite *sprite,int x,int y) {
  int alpha=0xff;
  const double FADE_TIME=0.500;
  if (SPRITE->ttl<FADE_TIME) {
    alpha=(int)((SPRITE->ttl*255.0)/FADE_TIME);
    if (alpha<1) return;
    if (alpha>0xff) alpha=0xff;
  }
  if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
  graf_tile(&g.graf,x,y,0x43,(SPRITE->dx<0.0)?EGG_XFORM_XREV:0);
  if (alpha<0xff) graf_set_alpha(&g.graf,0xff);
}

const struct sprite_type sprite_type_ghost={
  .name="ghost",
  .objlen=sizeof(struct sprite_ghost),
  .init=_ghost_init,
  .update=_ghost_update,
  .render=_ghost_render,
};
