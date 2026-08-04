/* sprite.h
 * Generic inhabitant of the world.
 * I'm hoping that everything interactive will be expressible thru sprites.
 * We don't own the global set of sprites -- that's "world".
 */
 
#ifndef SPRITE_H
#define SPRITE_H

/* Generic sprite.
 ************************************************************************/

struct sprite {
  const struct sprite_type *type;
  int defunct;
  int id; // Positive and unique across all time, unless 2 billion get spawned.
  double x,y; // In world meters ie 0..64 ish.
  int layer; // Hero at 100.
  int render_always; // Nonzero, we'll call (render) every frame. Zero, we call it when you're close to the camera.
  int family; // Zero if you don't participate, otherwise which region's completion do you contribute to.
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  
  /* Return <0 or set (defunct) to abort construction.
   */
  int (*init)(struct sprite *sprite,const void *args,int argslen);
  
  void (*update)(struct sprite *sprite,double elapsed);
  
  /* Renderer must arm image:sprites before.
   * If you disarm it, you must restore before returning.
   * (x,y) is the center of this sprite in framebuffer pixels.
   * There is no default render. If you don't implement this hook, you're invisible.
   */
  void (*render)(struct sprite *sprite,int x,int y);
  
  /* React to the hero walking into me.
   * Return nonzero to behave solid; player's walking is rejected.
   */
  int (*bump)(struct sprite *sprite,struct sprite *hero);
};

void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(const struct sprite_type *type,double x,double y,const void *args,int argslen);

/* Species.
 ***********************************************************************/

extern const struct sprite_type sprite_type_hero;
extern const struct sprite_type sprite_type_monster;
extern const struct sprite_type sprite_type_card;
extern const struct sprite_type sprite_type_guard;

struct sprite_args_hero {
  int TODO;
};

void sprite_hero_input(struct sprite *sprite,int input);

/* I want to do a "peek" feature, where you press A in the outerworld and the camera pans a bit in whatever direction you're facing.
 * If peek is in play, this returns nonzero and populates (px,py) with -1..1.
 */
int sprite_hero_get_peeking(double *px,double *py,const struct sprite *sprite);

uint64_t sprite_hero_get_hand(struct sprite *sprite);
int sprite_hero_set_hand(struct sprite *sprite,uint64_t hand);
void sprite_hero_set_blackout(struct sprite *sprite,double s); // No movement for so long. For when monsters flee.

struct sprite_args_monster {
  int TODO;
};

uint64_t sprite_monster_get_hand(struct sprite *sprite);
int sprite_monster_set_hand(struct sprite *sprite,uint64_t hand);
int sprite_monster_set_tileid(struct sprite *sprite,uint8_t tileid);
int sprite_monster_get_resting_position(int *x,int *y,const struct sprite *sprite); // My quantized position, or if I'm fleeing, where I will eventually end up.

uint64_t sprite_card_get_hand(const struct sprite *sprite); // Hand should be just one bit, but we tolerate anything.
int sprite_card_set_hand(struct sprite *sprite,uint64_t hand);

int sprite_guard_set_limit(struct sprite *sprite,int limit);

#endif
