/* sprite_bonusguard.c
 * The lil guy standing on the bridge between the graveyard and bonus zone.
 * We're also the controller for the overall puzzle, why not.
 */
 
#include "all52.h"

// Stepping outside the graves resets the puzzle. Inclusive both ends.
#define XLO 12
#define XHI 18
#define YLO 25
#define YHI 31

#define PATH_LIMIT 13

struct sprite_bonusguard {
  struct sprite hdr;
  int hx,hy; // Last observed hero position, in quantized world meters.
  
  struct path_entry {
    uint8_t x,y; // 0..2
  } pathv[PATH_LIMIT];
  int pathc;
  int pathp; // How many correct steps has the hero taken?
};

#define SPRITE ((struct sprite_bonusguard*)sprite)

/* Update.
 */
 
static void _bonusguard_update(struct sprite *sprite,double elapsed) {

  /* Get the hero.
   * If not found, quietly drop the path, same as OOB.
   */
  struct sprite *hero=world_get_hero();
  if (!hero) {
    SPRITE->pathc=0;
    SPRITE->hx=SPRITE->hy=0; // Yes, zero is a valid cell, but it's definitely not reachable.
    return;
  }
  
  /* Get hero's quantized position.
   * We'll cache it too, no need to do anything further for the overwhelming majority of frames when it hasn't changed.
   */
  int hx=(int)hero->x;
  int hy=(int)hero->y;
  if ((hx==SPRITE->hx)&&(hy==SPRITE->hy)) return;
  SPRITE->hx=hx;
  SPRITE->hy=hy;
  
  /* If OOB, quietly drop the path.
   * Mind that we update always, no matter where the hero is, so this fires redundantly a lot.
   */
  if ((hx<XLO)||(hx>XHI)||(hy<YLO)||(hy>YHI)) {
    SPRITE->pathc=0;
    return;
  }
  
  /* If we don't have a path, generate it.
   * The edge cells are not path candidates, only the interior intersections are.
   * So we can safely assume that the hero is off-path on this frame when we generate the path.
   *
   * Path control points are a grid of 0..2,0..2.
   * Path must end at (1,2), south central, so the bridge is in view when you finish.
   * Control points are cardinally adjacent to each other.
   * It's fine to go back and forth. It's a dance, not a treasure map.
   */
  if (!SPRITE->pathc) {
    SPRITE->pathp=0;
    SPRITE->pathc=PATH_LIMIT; // Could go under the limit if there were some reason to. There won't be.
    int i=SPRITE->pathc-1;
    struct path_entry *entry=SPRITE->pathv+i;
    entry->x=1;
    entry->y=2;
    while (i>0) {
      i--;
      entry--;
      struct path_entry candidatev[4];
      int candidatec=0;
      if (entry[1].x>0) candidatev[candidatec++]=(struct path_entry){entry[1].x-1,entry[1].y};
      if (entry[1].x<2) candidatev[candidatec++]=(struct path_entry){entry[1].x+1,entry[1].y};
      if (entry[1].y>0) candidatev[candidatec++]=(struct path_entry){entry[1].x,entry[1].y-1};
      if (entry[1].y<2) candidatev[candidatec++]=(struct path_entry){entry[1].x,entry[1].y+1};
      int p=rand()%candidatec;
      *entry=candidatev[p];
    }
  }
  
  /* Assert (pathp<pathc).
   * If that's not true, something is broken. Clear the path and get out.
   */
  if (SPRITE->pathp>=SPRITE->pathc) {
    SPRITE->pathc=0;
    return;
  }
  
  /* Turn the hero's position into normalized intersection coords, comparable to the path.
   * If she's in between intersections, no worries, get out.
   */
  int px=hx-XLO;
  if (!(px&1)) return;
  px>>=1;
  if ((px<0)||(px>2)) return;
  int py=hy-YLO;
  if (!(py&1)) return;
  py>>=1;
  if ((py<0)||(py>2)) return;
  
  /* OK, she's standing on an intersection.
   * If it's not (pathv[pathp]), reset (pathp).
   * And in that case, check whether she's standing on [0], bump to [1] if so.
   */
  if ((px!=SPRITE->pathv[SPRITE->pathp].x)||(py!=SPRITE->pathv[SPRITE->pathp].y)) {
    SPRITE->pathp=0;
    if ((px==SPRITE->pathv[0].x)&&(py==SPRITE->pathv[0].y)) {
      SPRITE->pathp=1;
    }
    return;
  }
  
  /* Advance (pathp).
   * If it reaches the end, celebrate by killing yourself.
   * All we have to do is die: The bonus room becomes reachable, and this puzzle logic stops running.
   */
  SPRITE->pathp++;
  if (SPRITE->pathp>=SPRITE->pathc) {
    SFX(secret)
    sprite->defunct=1;
  }
}

/* Bump.
 */
 
static int _bonusguard_bump(struct sprite *sprite,struct sprite *hero) {
  struct modal_args_dialogue args={
    .rid=1,
    .strix=27,
  };
  struct modal *modal=modal_spawn(&modal_type_dialogue,&args,sizeof(args));
  return 1;
}

/* Render.
 */
 
static void _bonusguard_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,0x34,0);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_bonusguard={
  .name="bonusguard",
  .objlen=sizeof(struct sprite_bonusguard),
  .update=_bonusguard_update,
  .bump=_bonusguard_bump,
  .render=_bonusguard_render,
};

/* Public: Get compass override.
 */
 
uint8_t sprite_bonusguard_get_compass_override(struct sprite *sprite,struct sprite *hero) {
  if (!sprite||(sprite->type!=&sprite_type_bonusguard)) return 0;
  if (SPRITE->pathp>=SPRITE->pathc) return 0; // OOB
  struct path_entry *dst=SPRITE->pathv+SPRITE->pathp;
  int dstx=XLO+1+(dst->x<<1);
  int dsty=YLO+1+(dst->y<<1);
  if (SPRITE->hy>dsty) {
    if (SPRITE->hx>dstx) return 0x80;
    if (SPRITE->hx==dstx) return 0x40;
    return 0x20;
  }
  if (SPRITE->hy==dsty) {
    if (SPRITE->hx>dstx) return 0x10;
    if (SPRITE->hx<dstx) return 0x08;
    return 0xff;
  }
  if (SPRITE->hx>dstx) return 0x04;
  if (SPRITE->hx==dstx) return 0x02;
  return 0x01;
}
