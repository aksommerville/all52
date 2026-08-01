#include "all52.h"

/* Globals.
 */
 
#define SPRITE_LIMIT 128
 
static struct {
  int init;
  
  struct sprite *spritev[SPRITE_LIMIT];
  int spritec;
  
} world={0};

/* Quit.
 */
 
void world_quit() {
  while (world.spritec-->0) sprite_del(world.spritev[world.spritec]);
  memset(&world,0,sizeof(world));
}

/* Reset.
 */
 
int world_reset() {
  world_quit();
  world.init=1;
  
  /* Hero starts dead center.
   */
  {
    struct sprite_args_hero args={0};
    struct sprite *hero=sprite_spawn(&sprite_type_hero,(MAPW>>1)+0.5,(MAPH>>1)+0.5,&args,sizeof(args));
    if (!hero) return -1;
  }
  
  //TODO Spawn monsters randomly.
  //TODO Deal out the 52 cards.
  
  return 0;
}

/* Update.
 */
 
void world_update(double elapsed,int input) {

  /* Update all sprites, and if we find a hero, record her and give her the input first.
   */
  struct sprite *hero=0;
  struct sprite **p=world.spritev;
  int i=world.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct) continue;
    if (!hero&&(sprite->type==&sprite_type_hero)) {
      hero=sprite;
      sprite_hero_input(sprite,input);
    }
    if (sprite->type->update) sprite->type->update(sprite,elapsed);
  }
  
  /* Drop any defunct sprites.
   */
  for (i=world.spritec,p=world.spritev+world.spritec-1;i-->0;p--) {
    struct sprite *sprite=*p;
    if (!sprite->defunct) continue;
    world.spritec--;
    memmove(p,p+1,sizeof(void*)*(world.spritec-i));
    sprite_del(sprite);
  }
  
  /* One pass of the layer sort.
   */
  //TODO
}

/* Spawn sprite.
 */
 
struct sprite *sprite_spawn(const struct sprite_type *type,double x,double y,const void *args,int argslen) {
  struct sprite *sprite=sprite_new(type,x,y,args,argslen);
  if (!sprite) return 0;
  if (world.spritec>=SPRITE_LIMIT) {
    sprite_del(sprite);
    return 0;
  }
  world.spritev[world.spritec++]=sprite;
  return sprite;
}

/* Get sprites.
 */
 
struct sprite *world_get_hero() {
  struct sprite **p=world.spritev;
  int i=world.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->type!=&sprite_type_hero) continue;
    if (sprite->defunct) continue;
    return sprite;
  }
  return 0;
}

int world_get_sprites(struct sprite ***vppp) {
  *vppp=world.spritev;
  return world.spritec;
}
