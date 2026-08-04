/* world.h
 * Model of the game's overall state.
 * I guess that means the sprites and the deck.
 */
 
#ifndef WORLD_H
#define WORLD_H

void world_quit();

/* Begin a new campaign.
 * If we persist campaigns -- an open question -- there will be an alternative function for it.
 */
int world_reset();

void world_update(double elapsed,int input);

struct sprite *sprite_spawn(const struct sprite_type *type,double x,double y,const void *args,int argslen);

/* It's generally safe to operate on individual sprites from anywhere.
 * The lists don't move except when the update cycle is complete.
 * With (world_get_sprites) use some caution, that's the real underlying list.
 */
struct sprite *world_get_hero();
int world_get_sprites(struct sprite ***vppp);
struct sprite *sprite_by_id(int spriteid);

/* Returns one of:
 *  +1: We're in a completable region and it's been completed.
 *   0: We're not in a completable region. Crossing a bridge or something.
 *  -1: We're in a completable region and it's still pending.
 */
int world_describe_local_completion();

#endif
