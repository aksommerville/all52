/* modal_status.c
 * Little drawer that slides up from the bottom when you press B.
 */
 
#include "all52.h"

struct modal_status {
  struct modal hdr;
  double slide; // 0..1 = ready..offscreen
  double dslide; // hz, signed
  uint64_t hand;
  uint8_t arrowtile; // Zero for no arrow.
  uint8_t arrowxform;
  int arrowoffx,arrowoffy; // The tiles are off-center due to odd width, so apply this correction at render.
};

#define MODAL ((struct modal_status*)modal)

/* Cleanup.
 */
 
static void _status_del(struct modal *modal) {
}

/* Init.
 */
 
static int _status_init(struct modal *modal,const void *args,int argslen) {
  modal->opaque=0;
  modal->interactive=1;
  
  MODAL->slide=1.0;
  MODAL->dslide=-4.000;
  
  struct sprite **spritev=0;
  int spritec=world_get_sprites(&spritev);
  
  /* First find the hero sprite.
   * We need her both for the card pips and the direction arrow.
   */
  struct sprite *hero=0;
  int i=spritec;
  while (i-->0) {
    struct sprite *sprite=spritev[i];
    if (sprite->defunct) continue;
    if (sprite->type==&sprite_type_hero) {
      hero=sprite;
      break;
    }
  }
  
  if (hero) {
    MODAL->hand=sprite_hero_get_hand(hero);
    
    /* Now find the nearest monster or card sprite so we can point to it.
     */
    struct sprite *nearest=0;
    double nearestd2=999.999;
    for (i=spritec;i-->0;) {
      struct sprite *sprite=spritev[i];
      if (sprite->defunct) continue;
      if (
        (sprite->type==&sprite_type_monster)||
        (sprite->type==&sprite_type_card)
      ) {
        double dx=sprite->x-hero->x;
        double dy=sprite->y-hero->y;
        double d2=dx*dx+dy*dy;
        if (!nearest||(d2<nearestd2)) {
          nearest=sprite;
          nearestd2=d2;
        }
      }
    }
    if (nearest) {
      double t=atan2(nearest->x-hero->x,hero->y-nearest->y);
           if (t<M_PI*-0.875) { MODAL->arrowtile=0x6c; MODAL->arrowxform=EGG_XFORM_SWAP|EGG_XFORM_XREV; MODAL->arrowoffy=1; }
      else if (t<M_PI*-0.625) { MODAL->arrowtile=0x6d; }
      else if (t<M_PI*-0.375) { MODAL->arrowtile=0x6c; }
      else if (t<M_PI*-0.125) { MODAL->arrowtile=0x6d; MODAL->arrowxform=EGG_XFORM_YREV; MODAL->arrowoffy=1; }
      else if (t<M_PI* 0.125) { MODAL->arrowtile=0x6c; MODAL->arrowxform=EGG_XFORM_SWAP; }
      else if (t<M_PI* 0.375) { MODAL->arrowtile=0x6d; MODAL->arrowxform=EGG_XFORM_XREV|EGG_XFORM_YREV; MODAL->arrowoffx=1; MODAL->arrowoffy=1; }
      else if (t<M_PI* 0.625) { MODAL->arrowtile=0x6c; MODAL->arrowxform=EGG_XFORM_XREV; MODAL->arrowoffx=1; }
      else if (t<M_PI* 0.875) { MODAL->arrowtile=0x6d; MODAL->arrowxform=EGG_XFORM_XREV; MODAL->arrowoffx=1; }
      else                    { MODAL->arrowtile=0x6c; MODAL->arrowxform=EGG_XFORM_SWAP|EGG_XFORM_XREV; MODAL->arrowoffy=1; }
    }
  }
  
  return 0;
}

/* Focus.
 */
 
static void _status_focus(struct modal *modal,int focus) {
}

/* Update.
 */
 
static void _status_update(struct modal *modal,double elapsed,int input) {

  /* Start sliding down if she presses WEST.
   * You can do this during the slide-in too.
   */
  if ((input&EGG_BTN_WEST)&&(MODAL->dslide<=0.0)) {
    MODAL->dslide=4.000;
    SFX(uiback)
  }
  
  /* Sliding?
   */
  if (MODAL->dslide<0.0) {
    if ((MODAL->slide+=MODAL->dslide*elapsed)<=0.0) {
      MODAL->dslide=0.0; // Reached final position.
    }
  } else if (MODAL->dslide>0.0) {
    if ((MODAL->slide+=MODAL->dslide*elapsed)>=1.0) {
      // Off the screen. Kill me and get on with other things.
      modal->defunct=1;
    }
  }
}

/* Render.
 */
 
static void _status_render(struct modal *modal) {
  
  /* Our background image is the full framebuffer, just to keep things neat.
   * Most of its upper space is vacant.
   * So (dsty) is zero at full extension.
   */
  int dsty=(int)(MODAL->slide*28.0);
  graf_set_input(&g.graf,g.texid_modals);
  graf_decal(&g.graf,0,dsty,FBW,FBH,FBW,FBH);
  
  /* Sample the clock right here.
   * This controls the clock display, and also the arrow's blinking.
   */
  int ms=(int)(g.playclock*1000.0);
  if (ms<0) ms=0;
  int sec=ms/1000; ms%=1000;
  int blink=(ms<500)?1:0;
  int min=sec/60; sec%=60;
  if (min>99) {
    min=sec=99;
    ms=999;
  }
  
  /* Pips per card.
   * From here on, everything is tiles from image:sprites.
   * We're going to be rendering a lot of the same stuff over and over. Sixtyish tiles worst case, I think it's fine.
   */
  graf_set_input(&g.graf,g.texid_sprites);
  uint64_t mask=1;
  int row=0; // ie suit
  int pipy=dsty+30;
  for (;row<4;row++,pipy+=4) {
    int col=0; // ie rank
    int pipx=10;
    for (;col<13;col++,pipx+=3,mask<<=1) {
      if (MODAL->hand&mask) graf_tile(&g.graf,pipx,pipy,0x6b,0);
    }
  }
  
  /* Blinking arrow, pointing to the nearest card in world.
   */
  if (MODAL->arrowtile) {
    graf_tile(&g.graf,7+MODAL->arrowoffx,dsty+48+MODAL->arrowoffy,MODAL->arrowtile+blink*2,MODAL->arrowxform);
  }
  
  /* Clock.
   */
  if (min>=10) graf_tile(&g.graf,31,dsty+48,0x60+min/10,0);
  graf_tile(&g.graf,35,dsty+48,0x60+min%10,0);
  graf_tile(&g.graf,41,dsty+48,0x60+sec/10,0);
  graf_tile(&g.graf,45,dsty+48,0x60+sec%10,0);
  
  /* Blinking colon.
   */
  if (blink) {
    graf_tile(&g.graf,38,dsty+48,0x6a,0);
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_status={
  .name="status",
  .objlen=sizeof(struct modal_status),
  .del=_status_del,
  .init=_status_init,
  .focus=_status_focus,
  .update=_status_update,
  .render=_status_render,
};
