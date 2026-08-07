#include "all52.h"

/* Globals.
 */
 
#define SPRITE_LIMIT 64
#define REGION_LIMIT 16
 
static struct {
  int init;
  
  uint64_t deck; // Only relevant during init.
  int family; // ''
  
  struct sprite *spritev[SPRITE_LIMIT];
  int spritec;
  
  /* Region index is "family".
   * Slot zero is unused.
   */
  struct region {
    int xlo,ylo,xhi,yhi; // In world meters, inclusive.
    int done; // Goes nonzero when we first detect the cards and monsters are gone, so we don't need to check again after.
  } regionv[REGION_LIMIT];
  int herofamily; // Which region the hero was in last, so we don't have to check all of them except when she leaves.
  
} world={0};

/* Quit.
 */
 
void world_quit() {
  while (world.spritec-->0) sprite_del(world.spritev[world.spritec]);
  memset(&world,0,sizeof(world));
}

/* Search a sorted list of int.
 */
 
static int cell_present(const int *v,int c,int q) {
  int lo=0,hi=c;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int ckv=v[ck];
         if (q<ckv) hi=ck;
    else if (q>ckv) lo=ck+1;
    else return 1;
  }
  return 0;
}

/* Generate free cards within a given rectangle of the world.
 * (xlo,ylo,xhi,yhi) are inclusive, in meters ie 0..63.
 */
 
static int world_freergn(int xlo,int ylo,int xhi,int yhi,int spritec) {
  //fprintf(stderr,"freergn size %d\n",(xhi-xlo+1)*(yhi-ylo+1));
  if (world.family<REGION_LIMIT) {
    world.regionv[world.family]=(struct region){xlo,ylo,xhi,yhi};
  }

  /* Write a list of candidate cells.
   */
  int cellv[1024];
  int cellc=0;
  int y=ylo; for (;y<=yhi;y++) {
    const uint8_t *physics=g.physics+y*8+(xlo>>3);
    uint8_t phmask=0x80>>(xlo&7);
    int x=xlo; for (;x<=xhi;x++) {
      if (cellc>=1024) break;
      if ((x==32)&&(y==32)) break; // Skip this one, where the hero goes. Easier than scanning sprites generically.
      if ((x==41)&&(y==41)) break; // '' ...hopefully we won't have many sprites like this...
      if (!((*physics)&phmask)) {
        cellv[cellc++]=y*MAPW+x;
      }
      if (phmask==0x01) {
        phmask=0x80;
        physics++;
      } else {
        phmask>>=1;
      }
    }
    if (cellc>=1024) break;
  }
  if (cellc<spritec) return -1;
  
  /* Generate cards.
   */
  while (spritec-->0) {
    int cellp=rand()%cellc;
    int x=cellv[cellp]%MAPW;
    y=cellv[cellp]/MAPW;
    cellc--;
    memmove(cellv+cellp,cellv+cellp+1,sizeof(int)*(cellc-cellp));
    
    struct sprite *sprite=sprite_spawn(&sprite_type_card,x+0.5,y+0.5,0,0);
    if (!sprite) return -1;
    uint64_t hand=hand_deal_n(world.deck,1);
    if (!hand) {
      fprintf(stderr,"Out of cards!\n");
      return -1;
    }
    world.deck&=~hand;
    if (sprite_card_set_hand(sprite,hand)<0) return -1;
    sprite->family=world.family;
  }
  
  world.family++;
  return 0;
}

/* Generate monsters within a given rectangle of the world.
 * (xlo,ylo,xhi,yhi) are inclusive, in meters ie 0..63.
 */
 
static int world_poprgn(int xlo,int ylo,int xhi,int yhi,int spritec,int cardc_per,uint8_t tileid) {
  //fprintf(stderr,"poprgn size %d\n",(xhi-xlo+1)*(yhi-ylo+1));
  if (world.family<REGION_LIMIT) {
    world.regionv[world.family]=(struct region){xlo,ylo,xhi,yhi};
  }

  /* Write a list of candidate cells.
   */
  int cellv[1024];
  int cellc=0;
  int y=ylo; for (;y<=yhi;y++) {
    const uint8_t *physics=g.physics+y*8+(xlo>>3);
    uint8_t phmask=0x80>>(xlo&7);
    int x=xlo; for (;x<=xhi;x++) {
      if (cellc>=1024) break;
      if ((x==32)&&(y==32)) break; // Skip this one, where the hero goes. Easier than scanning sprites generically.
      if (!((*physics)&phmask)) {
        cellv[cellc++]=y*MAPW+x;
      }
      if (phmask==0x01) {
        phmask=0x80;
        physics++;
      } else {
        phmask>>=1;
      }
    }
    if (cellc>=1024) break;
  }
  if (cellc<spritec) return -1;
  
  /* Generate monsters.
   */
  while (spritec-->0) {
    int cellp=rand()%cellc;
    int x=cellv[cellp]%MAPW;
    y=cellv[cellp]/MAPW;
    cellc--;
    memmove(cellv+cellp,cellv+cellp+1,sizeof(int)*(cellc-cellp));
    
    struct sprite *sprite=sprite_spawn(&sprite_type_monster,x+0.5,y+0.5,0,0);
    if (!sprite) return -1;
    uint64_t hand=hand_deal_n(world.deck,cardc_per);
    if (!hand) {
      fprintf(stderr,"Out of cards!\n");
      return -1;
    }
    world.deck&=~hand;
    if (sprite_monster_set_hand(sprite,hand)<0) return -1;
    if (sprite_monster_set_tileid(sprite,tileid++)<0) return -1;
    sprite->family=world.family;
  }
  
  world.family++;
  return 0;
}

/* Generate the initial set of sprites, including dealing out the deck.
 */
 
static int world_populate() {
  world.deck=0x000fffffffffffffll;
  world.family=1;
  
  int seed=get_rand_seed();
  //fprintf(stderr,"Random seed 0x%08x\n",seed);
  
  /* Hero at a fixed point (the very middle).
   */
  struct sprite *hero=sprite_spawn(&sprite_type_hero,32.5,32.5,0,0);
  if (!hero) return -1;
  
  /* Monsters spawn within specific ranges.
   * poprgn( xlo,ylo, xhi,yhi, spritec, cardc_per, tileid )
   * 32 cards.
   */
  if (world_poprgn(47,26,55,33, 1,10,0x33)<0) return -1; // Witch's castle.
  if (world_poprgn( 1,52,12,62, 2, 3,0x13)<0) return -1; // SW island. Birds.
  if (world_poprgn(15,49,29,61, 2, 1,0x03)<0) return -1; // Donut island. Rodents.
  if (world_poprgn(32,52,62,62, 2, 6,0x23)<0) return -1; // SE island. Mammals.
  
  /* Spawning free cards is the same idea as monsters.
   */
  if (world_freergn( 3, 2,19,32, 5)<0) return -1; // NW island.
  if (world_freergn(22, 3,47,18, 4)<0) return -1; // North end of big island.
  if (world_freergn(51, 1,62,10, 4)<0) return -1; // NE island.
  if (world_freergn(46,14,62,22, 3)<0) return -1; // ENE peninsula.
  if (world_freergn(25,27,41,45, 6)<0) return -1; // Start zone.

  /* Three Card Guards, blocking specific regions.
   */
  struct sprite *sprite;
  if (!(sprite=sprite_spawn(&sprite_type_guard,34.5,25.5,0,0))) return -1;
  sprite_guard_set_limit(sprite,10);
  if (!(sprite=sprite_spawn(&sprite_type_guard,43.5,37.5,0,0))) return -1;
  sprite_guard_set_limit(sprite,30);
  if (!(sprite=sprite_spawn(&sprite_type_guard,53.5,35.5,0,0))) return -1;
  sprite_guard_set_limit(sprite,40);
  
  /* Other sprites.
   */
  if (!(sprite=sprite_spawn(&sprite_type_bonusguard,15.5,33.5,0,0))) return -1;
  if (!(sprite=sprite_spawn(&sprite_type_dialogue,41.5,41.5,0,0))) return -1;
  sprite_dialogue_set_strix(sprite,28);
  if (!(sprite=sprite_spawn(&sprite_type_treasure,9.5,41.5,0,0))) return -1;
  
  if (world.deck) {
    fprintf(stderr,"%s:%d:OOPS: Failed to deal out the whole deck.",__FILE__,__LINE__);
    hand_log("remaining",world.deck);
    return -1;
  }
  
  return 0;
}

/* Reset.
 */
 
int world_reset() {
  world_quit();
  world.init=1;
  g.hat=0;
  if (world_populate()<0) return -1;
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
  int defuncted=0;
  for (i=world.spritec,p=world.spritev+world.spritec-1;i-->0;p--) {
    struct sprite *sprite=*p;
    if (!sprite->defunct) continue;
    if (sprite==hero) hero=0;
    world.spritec--;
    memmove(p,p+1,sizeof(void*)*(world.spritec-i));
    sprite_del(sprite);
    defuncted=1;
  }
  
  /* If anything got removed, recheck all regions for completion.
   */
  if (defuncted) {
    struct region *region=world.regionv;
    for (i=REGION_LIMIT;i-->0;region++) {
      region->done=1;
    }
    for (i=world.spritec,p=world.spritev;i-->0;p++) {
      struct sprite *sprite=*p;
      world.regionv[sprite->family].done=0;
    }
  }
  
  /* One pass of the layer sort.
   */
  //TODO Punt this a bit. We're not very sprite-heavy, and I think we might not even need sorting.
  
  /* If there's a hero, check her region.
   */
  if (hero) {
    int qx=(int)hero->x;
    int qy=(int)hero->y;
    struct region *region=world.regionv+world.herofamily;
    if ((qx<region->xlo)||(qx>region->xhi)||(qy<region->ylo)||(qy>region->yhi)) {
      world.herofamily=0;
      for (i=0,region=world.regionv;i<REGION_LIMIT;i++,region++) {
        if (qx<region->xlo) continue;
        if (qx>region->xhi) continue;
        if (qy<region->ylo) continue;
        if (qy>region->yhi) continue;
        world.herofamily=i;
        break;
      }
    }
  }
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

struct sprite *sprite_by_id(int spriteid) {
  struct sprite **p=world.spritev;
  int i=world.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct) continue; // Even if the ID matches, a defunct sprite pretends not to exist anymore.
    if (sprite->id==spriteid) return sprite;
  }
  return 0;
}

/* Local completion.
 */
 
int world_describe_local_completion() {
  if (!world.herofamily) return 0;
  return world.regionv[world.herofamily].done?1:-1;
}
