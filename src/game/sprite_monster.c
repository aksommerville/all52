#include "all52.h"

/* After surviving a battle, we'll run up to so many turns away.
 * It's normal to terminate before that too.
 */
#define FLEESTEP_LIMIT 5

struct sprite_monster {
  struct sprite hdr;
  uint64_t hand;
  uint8_t tileid;
  double faceclock;
  uint8_t xform;
  
  struct fleestep {
    double x,y;
    int qx,qy;
    double dx,dy; // Speed baked in.
  } fleestepv[FLEESTEP_LIMIT];
  int fleestepc;
  int fleestepp;
};

#define SPRITE ((struct sprite_monster*)sprite)

/* Cleanup.
 */
 
static void _monster_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _monster_init(struct sprite *sprite,const void *args,int argslen) {
  if (args&&(argslen==sizeof(struct sprite_args_monster))) {
    //TODO digest args
  }
  SPRITE->tileid=0x00; // I'm Dot if they forget to set this.
  SPRITE->faceclock=((rand()&0xffff)/65535.0);
  return 0;
}

/* Update.
 */
 
static void _monster_update(struct sprite *sprite,double elapsed) {

  /* If fleeing, it's something quite different.
   * Follow the predetermined plan.
   * Our speed and head start is such that the hero can't catch us, and nothing else should be moving.
   */
  if (SPRITE->fleestepp<SPRITE->fleestepc) {
    struct fleestep *step=SPRITE->fleestepv+SPRITE->fleestepp;
    sprite->x+=step->dx*elapsed;
    sprite->y+=step->dy*elapsed;
    int done=0;
         if (step->dx<0.0) { done=(sprite->x<=step->x); SPRITE->xform=EGG_XFORM_XREV; }
    else if (step->dx>0.0) { done=(sprite->x>=step->x); SPRITE->xform=0; }
    else if (step->dy<0.0) done=(sprite->y<=step->y);
    else if (step->dy>0.0) done=(sprite->y>=step->y);
    else done=1;
    if (done) {
      sprite->x=step->x;
      sprite->y=step->y;
      SPRITE->fleestepp++;
    }
    //TODO animation?
    return;
  }

  // Turn to face the hero. But don't bother checking every frame; allow say a quarter second between polls.
  if ((SPRITE->faceclock-=elapsed)<=0.0) {
    SPRITE->faceclock+=0.250;
    struct sprite *hero=world_get_hero();
    if (hero) {
      if (hero->x<sprite->x-0.250) SPRITE->xform=EGG_XFORM_XREV;
      else if (hero->x>sprite->x+0.250) SPRITE->xform=0;
    }
  }
}

/* How far can a monster at (x,y) travel along the unit vector (dx,dy)?
 * If cell (x,y) itself is solid, we won't notice.
 */
 
static int monster_check_flee_distance(struct sprite *sprite,int x,int y,int dx,int dy) {
  if ((x<0)||(y<0)||(x>=MAPW)||(y>=MAPH)) return 0;
  if (dx&&dy) return 0;
  if (!dx&&!dy) return 0;
  
  /* First measure only by the map, that much is super easy.
   */
  int fx=x,fy=y,d=0;
  for (;;) {
    int nx=fx+dx;
    int ny=fy+dy;
    if ((nx<0)||(ny<0)||(nx>=MAPW)||(ny>=MAPH)) break;
    if (CKPH(nx,ny)) break;
    fx=nx;
    fy=ny;
    d++;
  }
  
  /* Prepare a box for comparing other sprites to.
   */
  double bl=x+0.250;
  double br=x+0.750;
  double bt=y+0.250;
  double bb=y+0.750;
       if (dx<0) bl=fx+0.250;
  else if (dx>0) br=fx+0.750;
  else if (dy<0) bt=fy+0.250;
  else if (dy>0) bb=fy+0.750;
  else return 0;
  
  /* If we find a solid sprite in the box, truncate.
   */
  struct sprite **p;
  int i=world_get_sprites(&p);
  for (;i-->0;p++) {
    struct sprite *other=*p;
    if (other->defunct) continue;
    if (other==sprite) continue; // this one's ok, i know him.
    // If the other is a monster, use its final position instead.
    // It's a little unusual, but not at all impossible, for two monsters to be in motion at once.
    double ox=other->x,oy=other->y;
    if (other->type==&sprite_type_monster) {
      int oqx,oqy;
      sprite_monster_get_resting_position(&oqx,&oqy,other);
      ox=oqx+0.5;
      oy=oqy+0.5;
    }
    // Assume everything else is solid.
    if (other->x<bl) continue;
    if (other->x>br) continue;
    if (other->y<bt) continue;
    if (other->y>bb) continue;
    int oqx=(int)other->x;
    int oqy=(int)other->y;
         if (dx<0) { fx=oqx+1; bl=fx+0.250; d=x-fx; }
    else if (dx>0) { fx=oqx-1; br=fx+0.750; d=fx-x; }
    else if (dy<0) { fy=oqy+1; bt=fy+0.250; d=y-fy; }
    else if (dy>0) { fy=oqy-1; bb=fy+0.750; d=fy-y; }
    else d=0;
    if (d<1) return 0;
  }
  
  return d;
}

/* I've just survived a battle.
 * Start running away.
 */
 
static void monster_flee(struct sprite *sprite,struct sprite *hero) {
  sprite_hero_set_blackout(hero,0.500); // Not long, just give us a wee head start.
  SPRITE->fleestepp=SPRITE->fleestepc=0;
  
  /* The broad idea:
   *  - Check how far from the pointer we can travel in each of the cardinal directions.
   *  - - Need to respect other sprites as well as the map.
   *  - Pick one direction randomly, weighted by length. Or square of length? Strongly favor the longest direction.
   *  - - Debatable: Nix the one or two directions that travel toward the hero.
   *  - Pick a random nonzero distance along that line. If it's long enough, restrict to the far half.
   *  - Repeat until whatever. If we end up not fleeing at all, it's not the end of the world.
   */
  int sqx=(int)sprite->x;
  int sqy=(int)sprite->y;
  int hqx=(int)hero->x;
  int hqy=(int)hero->y;
  while (SPRITE->fleestepc<FLEESTEP_LIMIT) {
    struct candidate {
      int dx,dy;
      int distance;
      int weight;
    } candidatev[4];
    int candidatec=0;
    int wsum=0;
    #define CK(_dx,_dy) { \
      struct candidate *c=candidatev+candidatec; \
      /* Debatable: Ignore the 1 or 2 directions that go toward the hero. */ \
      if ((hqx<sqx)&&(_dx<0)) ; \
      else if ((hqx>sqx)&&(_dx>0)) ; \
      else if ((hqy<sqy)&&(_dy<0)) ; \
      else if ((hqy>sqy)&&(_dy>0)) ; \
      else { /* OK re hero. Check the available distance. */ \
        c->distance=monster_check_flee_distance(sprite,sqx,sqy,_dx,_dy); \
        if (c->distance>0) { \
          c->dx=_dx; \
          c->dy=_dy; \
          c->weight=c->distance*c->distance; \
          wsum+=c->weight; \
          candidatec++; \
        } \
      } \
    }
    CK(-1,0)
    CK(1,0)
    CK(0,-1)
    CK(0,1)
    #undef CK
    if (candidatec<1) break; // We must be cornered. No worries, just stop here.
    int choice=rand()%wsum;
    struct candidate *c=0;
    struct candidate *ci=candidatev;
    int i=candidatec;
    for (;i-->0;ci++) {
      if ((choice-=ci->weight)<0) {
        c=ci;
        break;
      }
    }
    if (!c) break; // oops
    // Direction is chosen. Now, if it's longer than say 3 meters, pick a distance in the upper half. Otherwise take all of it.
    int d;
    if (c->distance>3) {
      int nope=c->distance>>1;
      d=nope+rand()%(c->distance-nope);
    } else {
      d=c->distance;
    }
    // Append to (fleepstepv) and update (sqx,sqy).
    const double speed=7.0; // m/s. The hero's speed is 6.0 and we should be just a little faster.
    struct fleestep *step=SPRITE->fleestepv+SPRITE->fleestepc++;
    step->dx=c->dx*speed;
    step->dy=c->dy*speed;
    step->qx=sqx+c->dx*d;
    step->qy=sqy+c->dy*d;
    step->x=step->qx+0.5;
    step->y=step->qy+0.5;
    sqx=step->qx;
    sqy=step->qy;
  }
}

/* Bump.
 */
 
static struct {
  int monsterid;
  int heroid;
} monster_battle_userdata={0};

static void monster_cb_battle(struct modal *modal) {
  struct sprite *sprite=sprite_by_id(monster_battle_userdata.monsterid);
  struct sprite *hero=sprite_by_id(monster_battle_userdata.heroid);
  if (!sprite||!hero) {
    fprintf(stderr,
      "%s:PANIC: Sprite not found after battle. monster:%d=>%p, hero:%d=>%p\n",
      __func__,monster_battle_userdata.monsterid,sprite,monster_battle_userdata.heroid,hero
    );
    return;
  }
  uint64_t manhand=modal_battle_get_man_hand(modal);
  uint64_t cpuhand=modal_battle_get_cpu_hand(modal);
  //hand_log("cpu after",cpuhand);
  //hand_log("man after",manhand);
  sprite_hero_set_hand(hero,manhand);
  SPRITE->hand=cpuhand;
  if (!manhand) { // Hero cleaned out!
    g.finish=-1;
  } else if (cpuhand) {
    monster_flee(sprite,hero);
  } else {
    fprintf(stderr,"...thou hast done well in defeating the monster\n");//TODO fanfare. maybe soulballs?
    sprite->defunct=1;
    sprite_spawn(&sprite_type_ghost,sprite->x,sprite->y,0,0);
    if (manhand==0x000fffffffffffffll) {
      g.finish=1;
    }
  }
}
 
static int _monster_bump(struct sprite *sprite,struct sprite *hero) {

  /* It shouldn't be possible, but if the hero catches up with us while fleeing, make her wait.
   */
  if (SPRITE->fleestepp<SPRITE->fleestepc) return 1;

  monster_battle_userdata.monsterid=sprite->id;
  monster_battle_userdata.heroid=hero->id;
  struct modal_args_battle args={
    .cpu_hand=SPRITE->hand,
    .man_hand=sprite_hero_get_hand(hero),
    .cb=monster_cb_battle,
    .userdata=&monster_battle_userdata,
  };
  if (!args.man_hand) { // Not sure whether this will be possible. But do keep this clause at least as a safety; do not try to enter battle!
    fprintf(stderr,"%s:%d:TODO: Reject battle due to no cards.\n",__FILE__,__LINE__);//TODO dialogue or something. mind that this retriggers instantly if we don't go modal.
    return 1;
  }
  modal_world_get_sprite_render_position(&args.hltx,&args.hlty,modal_topmost_of_type(&modal_type_world),sprite);
  struct modal *modal=modal_spawn(&modal_type_battle,&args,sizeof(args));
  if (!modal) {
    fprintf(stderr,"%s: Spawing battle modal failed!\n",__func__);
  }
  return 1;
}

/* Render.
 */
 
static void _monster_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,SPRITE->tileid,SPRITE->xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_monster={
  .name="monster",
  .objlen=sizeof(struct sprite_monster),
  .del=_monster_del,
  .init=_monster_init,
  .update=_monster_update,
  .render=_monster_render,
  .bump=_monster_bump,
};

/* Public accessors.
 */

uint64_t sprite_monster_get_hand(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return 0;
  return SPRITE->hand;
}

int sprite_monster_set_hand(struct sprite *sprite,uint64_t hand) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return -1;
  SPRITE->hand=hand;
  return 0;
}

int sprite_monster_set_tileid(struct sprite *sprite,uint8_t tileid) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return -1;
  SPRITE->tileid=tileid;
  return 0;
}

/* Get resting position.
 */

int sprite_monster_get_resting_position(int *x,int *y,const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return -1;
  if (SPRITE->fleestepp<SPRITE->fleestepc) {
    const struct fleestep *step=SPRITE->fleestepv+SPRITE->fleestepc-1;
    *x=step->qx;
    *y=step->qy;
  } else {
    *x=(int)sprite->x;
    *y=(int)sprite->y;
  }
  return 0;
}
