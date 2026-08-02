#include "all52.h"

static int spriteid_next=1;

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
}

/* New.
 */
 
struct sprite *sprite_new(const struct sprite_type *type,double x,double y,const void *args,int argslen) {
  if (!type) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  
  sprite->type=type;
  if (spriteid_next<1) spriteid_next=1; // Expect this to happen every few million years of play.
  sprite->id=spriteid_next++;
  sprite->x=x;
  sprite->y=y;
  sprite->layer=100;
  
  if (type->init&&((type->init(sprite,args,argslen)<0)||sprite->defunct)) {
    sprite_del(sprite);
    return 0;
  }
  return sprite;
}
