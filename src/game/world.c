#include "all52.h"

/* Globals.
 */
 
#define SPRITE_LIMIT 1024
 
static struct {
  int init;
  
  uint64_t deck; // Only relevant during init.
  int family; // ''
  
  struct sprite *spritev[SPRITE_LIMIT];
  int spritec;
  
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

/* Generate monsters within a given rectangle of the world.
 * (xlo,ylo,xhi,yhi) are inclusive, in meters ie 0..63.
 */
 
static int world_poprgn(int xlo,int ylo,int xhi,int yhi,int spritec,int cardc_per,uint8_t tileid) {

  /* Write a list of candidate cells.
   */
  int cellv[1024]; // Current scratch map, the largest region is 756 cells. (the monster zone, in the southwest)
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
    if (sprite_monster_set_family(sprite,world.family)<0) return -1;
  }
  
  /* Every third cell around the border, if its outer neighbor is solid, it is vacant, and no monster is there,
   * put a flag sprite there.
   * Idea is to show the bounds of each monster region, and indicate whether the region is complete.
   */
  int xp=xlo,yp=ylo,phase=0;
  int ndx=0,ndy=-1;
  uint8_t xform=0;
  for (;;) {
    if (!phase--) {
      if (!CKPH(xp,yp)) { // Cell is vacant, so far so good.
        if (CKPH(xp+ndx,yp+ndy)) { // Outer neighbor is solid, good.
          if (cell_present(cellv,cellc,yp*MAPW+xp)) { // And we didn't make a monster here.
            fprintf(stderr,"make a flag at %d,%d. spritec=%d\n",xp,yp,world.spritec);//TODO make a flag here
            struct sprite *sprite=sprite_spawn(&sprite_type_flag,xp+0.5,yp+0.5,0,0);
            if (!sprite) return -1;
            sprite_flag_set_family(sprite,world.family);
            sprite_flag_set_xform(sprite,xform);
          }
        }
      }
      phase=2;
    }
    if (yp==ylo) {
      if (++xp>xhi) {
        xp=xhi;
        yp++;
        ndx=1;
        ndy=0;
        xform=EGG_XFORM_XREV;
      }
    } else if (xp==xhi) {
      if (++yp>yhi) {
        yp=yhi;
        xp--;
        ndx=0;
        ndy=1;
      }
    } else if (yp==yhi) {
      if (--xp<xlo) {
        xp=xlo;
        yp--;
        ndx=-1;
        ndy=0;
        xform=0;
      }
    } else if (xp==xlo) {
      if (--yp<=ylo) {
        break;
      }
    }
  }
  
  world.family++;
  return 0;
}

/* Generate the initial set of sprites, including dealing out the deck.
 */
 
static int world_populate() {
  world.deck=0x000fffffffffffffll;
  world.family=1;
  
  /* Hero at a fixed point (the very middle).
   */
  struct sprite *hero=sprite_spawn(&sprite_type_hero,32.5,32.5,0,0);
  if (!hero) return -1;
  uint64_t herohand=hand_deal_n(world.deck,6);
  world.deck&=~herohand;
  if (sprite_hero_set_hand(hero,herohand)<0) return -1;
  
  /* The rest are random.
   * A fixed set per region, and those regions and sets are defined right here.
   */
  if (world_poprgn( 3, 3,15,14,1,10,0x53)<0) return -1; // One witch with a ton of cards in the northwest.
  if (world_poprgn(19, 2,61,16,2, 5,0x43)<0) return -1; // Man.
  if (world_poprgn( 2,44,43,61,2, 4,0x33)<0) return -1; // Monsters.
  if (world_poprgn(45,31,62,62,3, 3,0x23)<0) return -1; // Greater Mammals.
  if (world_poprgn( 2,24,25,38,3, 2,0x13)<0) return -1; // Birds.
  if (world_poprgn(28,19,43,42,3, 1,0x03)<0) return -1; // Rodents.
  
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
  for (i=world.spritec,p=world.spritev+world.spritec-1;i-->0;p--) {
    struct sprite *sprite=*p;
    if (!sprite->defunct) continue;
    world.spritec--;
    memmove(p,p+1,sizeof(void*)*(world.spritec-i));
    sprite_del(sprite);
  }
  
  /* One pass of the layer sort.
   */
  //TODO Punt this a bit. We're not very sprite-heavy, and I think we might not even need sorting.
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
