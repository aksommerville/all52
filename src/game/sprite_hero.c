#include "all52.h"

#define PEEK_OUT_SPEED 1.000
#define PEEK_IN_SPEED  2.000
#define TURN_TIME 0.150

struct sprite_hero {
  struct sprite hdr;
  int pvinput;
  int qx,qy; // World meters. When in transit, it's the cell we're approaching.
  int facedx,facedy; // For picking tile only. One is always zero and the other nonzero.
  int indx,indy; // Dpad state as reported. Not necessarily what we're moving on.
  int inpeek;
  int peekdx,peekdy;
  double peekmag;
  int walking;
  double walkanimclock;
  int walkanimframe;
  double turnclock; // Counts down during a motion blackout due to turning. So we can change direction without changing position.
  uint64_t hand;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite,const void *args,int argslen) {

  SPRITE->qx=(int)sprite->x;
  SPRITE->qy=(int)sprite->y;
  SPRITE->facedx=0;
  SPRITE->facedy=1;

  if (args&&(argslen==sizeof(struct sprite_args_hero))) {
    //TODO digest args
  }
  return 0;
}

/* Begin walking in the given direction, if it's vacant.
 * Nonzero if walking now.
 */
 
static int hero_maybe_begin_walking(struct sprite *sprite,int dx,int dy) {
  int nx=SPRITE->qx+dx;
  int ny=SPRITE->qy+dy;
  //TODO rejection feedback?
  if (SPRITE->inpeek) return 0; // OK to walk while peek'd, but not when actually holding the button.
  if ((nx<0)||(ny<0)||(nx>=MAPW)||(ny>=MAPH)) return 0;
  if (CKPH(nx,ny)) return 0;
  
  /* Is there a monster or other solid sprite here?
   */
  struct sprite **otherp;
  int i=world_get_sprites(&otherp);
  for (;i-->0;otherp++) {
    struct sprite *other=*otherp;
    if (other->defunct) continue;
    if (!other->type->bump) continue;
    int oqx=(int)other->x;
    if (oqx!=nx) continue;
    int oqy=(int)other->y;
    if (oqy!=ny) continue;
    if (!other->type->bump(other,sprite)) continue;
    return 0;
  }
  
  SPRITE->qx=nx;
  SPRITE->qy=ny;
  SPRITE->walking=1;
  // (faced) should already have been set up at input reception, but let's be sure of it.
  SPRITE->facedx=dx;
  SPRITE->facedy=dy;
  return 1;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  /* Extend or withdraw peek.
   */
  if (SPRITE->inpeek) {
    if ((SPRITE->peekmag+=PEEK_OUT_SPEED*elapsed)>=1.0) SPRITE->peekmag=1.0;
  } else {
    if ((SPRITE->peekmag-=PEEK_IN_SPEED*elapsed)<=0.0) SPRITE->peekmag=0.0;
  }
  
  /* If we're already walking, tick animation and advance toward the target cell.
   * When we pass that target cell, check the input state and either proceed to the next cell or clamp to this one.
   */
  if (SPRITE->walking) {
    if ((SPRITE->walkanimclock-=elapsed)<=0.0) {
      SPRITE->walkanimclock+=0.140;
      if (++(SPRITE->walkanimframe)>=4) SPRITE->walkanimframe=0;
    }
    const double walkspeed=6.0; // m/s
    double dstx=SPRITE->qx+0.5;
    double dsty=SPRITE->qy+0.5;
    if (sprite->x<dstx) {
      if ((sprite->x+=walkspeed*elapsed)>=dstx) {
        if ((SPRITE->indx==1)&&hero_maybe_begin_walking(sprite,1,0)) ;
        else { sprite->x=dstx; SPRITE->walking=0; }
      }
    } else if (sprite->x>dstx) {
      if ((sprite->x-=walkspeed*elapsed)<=dstx) {
        if ((SPRITE->indx==-1)&&hero_maybe_begin_walking(sprite,-1,0)) ;
        else { sprite->x=dstx; SPRITE->walking=0; }
      }
    } else if (sprite->y<dsty) {
      if ((sprite->y+=walkspeed*elapsed)>=dsty) {
        if ((SPRITE->indy==1)&&hero_maybe_begin_walking(sprite,0,1)) ;
        else { sprite->y=dsty; SPRITE->walking=0; }
      }
    } else if (sprite->y>dsty) {
      if ((sprite->y-=walkspeed*elapsed)<=dsty) {
        if ((SPRITE->indy==-1)&&hero_maybe_begin_walking(sprite,0,-1)) ;
        else { sprite->y=dsty; SPRITE->walking=0; }
      }
    } else { // huh?
      SPRITE->walking=0;
    }
    
  /* If we're not already walking, has a dpad axis gone nonzero?
   */
  } else if ((SPRITE->indx||SPRITE->indy)&&(SPRITE->turnclock<=0.0)) {
    SPRITE->walkanimclock=0.0;
    SPRITE->walkanimframe=0;
    if (SPRITE->indx) hero_maybe_begin_walking(sprite,SPRITE->indx,0);
    if (!SPRITE->walking&&SPRITE->indy) hero_maybe_begin_walking(sprite,0,SPRITE->indy);
  }
  
  /* Turn clock ticks down whenever positive.
   * Do this after the motion block when we read it.
   */
  if (SPRITE->turnclock>0.0) SPRITE->turnclock-=elapsed;
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {
  uint8_t tileid=0x00;
  uint8_t xform=0;
  
  if (SPRITE->facedy>0) ; // Down, the natural orientation.
  else if (SPRITE->facedy<0) tileid+=1;
  else if (SPRITE->facedx>0) tileid+=2;
  else { tileid+=2; xform=EGG_XFORM_XREV; }
  
  if (SPRITE->walking) switch (SPRITE->walkanimframe) {
    case 1: tileid+=0x10; break;
    case 3: tileid+=0x20; break;
  }
  
  graf_tile(&g.graf,x,y,tileid,xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};

/* Receive input.
 */
 
void sprite_hero_input(struct sprite *sprite,int input) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return;
  if (input==SPRITE->pvinput) return;
  
  /* If either dpad axis changes to something nonzero, face that direction.
   * If they both change on the same frame, I don't care who wins the tie.
   * If direction changes and we're stationary, begin a brief motion blackout.
   */
  int indx,indy;
  switch (input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: indx=-1; break;
    case EGG_BTN_RIGHT: indx=1; break;
    default: indx=0; break;
  }
  switch (input&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: indy=-1; break;
    case EGG_BTN_DOWN: indy=1; break;
    default: indy=0; break;
  }
  int facedx=SPRITE->facedx,facedy=SPRITE->facedy;
  if (indx!=SPRITE->indx) {
    if (SPRITE->indx=indx) {
      facedx=indx;
      facedy=0;
    } else if (SPRITE->indy) {
      facedx=0;
      facedy=SPRITE->indy;
    }
  }
  if (indy!=SPRITE->indy) {
    if (SPRITE->indy=indy) {
      facedx=0;
      facedy=indy;
    } else if (SPRITE->indx) {
      facedx=SPRITE->indx;
      facedy=0;
    }
  }
  if ((facedx!=SPRITE->facedx)||(facedy!=SPRITE->facedy)) {
    SPRITE->facedx=facedx;
    SPRITE->facedy=facedy;
    if (!SPRITE->walking) SPRITE->turnclock=TURN_TIME;
  }
  
  /* Peek direction gets committed when you first strike it.
   * Is that right? Or do we want to be able to swing it around? ...we definitely don't want swinging around.
   */
  int inpeek=(input&EGG_BTN_SOUTH);
  if (inpeek!=SPRITE->inpeek) {
    if (SPRITE->inpeek=inpeek) {
      if (SPRITE->peekmag<=0.0) {
        SPRITE->peekdx=SPRITE->facedx;
        SPRITE->peekdy=SPRITE->facedy;
      }
    }
  }
  
  SPRITE->pvinput=input;
}

/* Public accessors.
 */
 
int sprite_hero_get_peeking(double *px,double *py,const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return 0;
  if (SPRITE->peekmag<=0.0) return 0;
  *px=SPRITE->peekmag*SPRITE->peekdx;
  *py=SPRITE->peekmag*SPRITE->peekdy;
  return 1;
}

uint64_t sprite_hero_get_hand(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return 0;
  return SPRITE->hand;
}

int sprite_hero_set_hand(struct sprite *sprite,uint64_t hand) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return -1;
  SPRITE->hand=hand;
  return 0;
}
