/* sprite_flag.c
 * Indicates whether a set of monsters has all been vanquished.
 */
 
#include "all52.h"

struct sprite_flag {
  struct sprite hdr;
  int family;
  int finished;
  uint8_t xform;
};

#define SPRITE ((struct sprite_flag*)sprite)

static int _flag_init(struct sprite *sprite,const void *args,int argslen) {
  return 0;
}

static void _flag_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,SPRITE->finished?0x71:0x70,SPRITE->xform);
}

const struct sprite_type sprite_type_flag={
  .name="flag",
  .objlen=sizeof(struct sprite_flag),
  .init=_flag_init,
  .render=_flag_render,
};

int sprite_flag_set_family(struct sprite *sprite,int family) {
  if (!sprite||(sprite->type!=&sprite_type_flag)) return -1;
  SPRITE->family=family;
  return 0;
}

int sprite_flag_get_family(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_flag)) return 0;
  return SPRITE->family;
}

void sprite_flag_set_xform(struct sprite *sprite,uint8_t xform) {
  if (!sprite||(sprite->type!=&sprite_type_flag)) return;
  SPRITE->xform=xform;
}

/* Refresh all.
 */

void sprite_flag_refresh() {
  uint32_t incomplete=0; // 1<<family
  struct sprite **spritev;
  int spritec=world_get_sprites(&spritev);
  struct sprite **p=spritev;
  int i=spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct) continue;
    if (sprite->type==&sprite_type_monster) { // Living monster. Mark its family incomplete.
      int family=sprite_monster_get_family(sprite);
      if ((family>0)&&(family<32)) incomplete|=1<<family;
    }
  }
  // And another pass for the flags.
  for (p=spritev,i=spritec;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct) continue;
    if (sprite->type==&sprite_type_flag) {
      if ((SPRITE->family>0)&&(SPRITE->family<32)) {
        if (incomplete&(1<<SPRITE->family)) {
          SPRITE->finished=0;
        } else {
          SPRITE->finished=1;
        }
      }
    }
  }
}
