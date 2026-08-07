/* modal_hello.c
 * A simple splash, first thing the user sees.
 * Responsible for initializing the game on dismissal.
 */
 
#include "all52.h"

#define BGCARD_LIMIT 80
#define BGCARD_SPEED_MIN  10.0
#define BGCARD_SPEED_PLUS 10.0

#define SUBTITLE_PERIOD 4.000
#define SUBTITLE_FADE_IN_TIME  0.300
#define SUBTITLE_FADE_OUT_TIME 0.300

struct modal_hello {
  struct modal hdr;
  struct bgcard {
    double x,y,dx,dy;
    int defunct;
  } bgcardv[BGCARD_LIMIT];
  int bgcardc;
  int subtitle; // 0..2
  double subtitle_clock; // Counts down.
};

#define MODAL ((struct modal_hello*)modal)

static int _hello_init(struct modal *modal,const void *args,int argslen) {
  modal->opaque=1;
  modal->interactive=1;
  
  /* Initial set of background cards is uniformly random around the screen.
   */
  while (MODAL->bgcardc<BGCARD_LIMIT) {
    struct bgcard *bgcard=MODAL->bgcardv+MODAL->bgcardc++;
    bgcard->x=rand()%FBW;
    bgcard->y=rand()%FBH;
    bgcard->dx=BGCARD_SPEED_MIN+((rand()&0xffff)*BGCARD_SPEED_PLUS)/65535.0;
    bgcard->dy=BGCARD_SPEED_MIN+((rand()&0xffff)*BGCARD_SPEED_PLUS)/65535.0;
    if (rand()&1) bgcard->dx*=-1.0;
    if (rand()&1) bgcard->dy*=-1.0;
  }
  
  /* Subtitle initially fades in with "Presented in Lowreznicolor".
   */
  MODAL->subtitle=0;
  MODAL->subtitle_clock=SUBTITLE_PERIOD;
  
  all52_song(RID_song_cross_currents,1);
  
  return 0;
}

static void _hello_update(struct modal *modal,double elapsed,int input) {

  if ((MODAL->subtitle_clock-=elapsed)<=0.0) {
    MODAL->subtitle_clock+=SUBTITLE_PERIOD;
    if (++(MODAL->subtitle)>=3) MODAL->subtitle=0;
  }

  struct bgcard *bgcard=MODAL->bgcardv;
  int i=MODAL->bgcardc;
  int defunctc=0;
  for (;i-->0;bgcard++) {
    bgcard->x+=bgcard->dx*elapsed;
    bgcard->y+=bgcard->dy*elapsed;
    if ((bgcard->x<-6.0)||(bgcard->y<-9.0)||(bgcard->x>FBW+7.0)||(bgcard->y>FBH+9.0)) {
      bgcard->defunct=1;
      defunctc++;
    }
  }
  if (defunctc) {
    for (i=MODAL->bgcardc,bgcard=MODAL->bgcardv+MODAL->bgcardc-1;i-->0;bgcard--) {
      if (!bgcard->defunct) continue;
      MODAL->bgcardc--;
      memmove(bgcard,bgcard+1,sizeof(struct bgcard)*(MODAL->bgcardc-i));
    }
  }
  while (MODAL->bgcardc<BGCARD_LIMIT) {
    bgcard=MODAL->bgcardv+MODAL->bgcardc++;
    bgcard->defunct=0;
    bgcard->dx=BGCARD_SPEED_MIN+((rand()&0xffff)*BGCARD_SPEED_PLUS)/65535.0;
    bgcard->dy=BGCARD_SPEED_MIN+((rand()&0xffff)*BGCARD_SPEED_PLUS)/65535.0;
    switch (rand()&3) {
      case 0: {
          bgcard->x=-6.0;
          bgcard->y=rand()%FBH;
          if (rand()&1) bgcard->dy*=-1.0;
        } break;
      case 1: {
          bgcard->x=rand()%FBW;
          bgcard->y=-9.0;
          if (rand()&1) bgcard->dx*=-1.0;
        } break;
      case 2: {
          bgcard->x=FBW+6.0;
          bgcard->y=rand()%FBH;
          bgcard->dx*=-1.0;
          if (rand()&1) bgcard->dy*=-1.0;
        } break;
      case 3: {
          bgcard->x=rand()%FBW;
          bgcard->y=FBH+9.0;
          if (rand()&1) bgcard->dx*=-1.0;
          bgcard->dy*=-1.0;
        } break;
    }
  }

  if (input&EGG_BTN_SOUTH) {
    struct modal_args_world args={0};
    struct modal *world=modal_spawn(&modal_type_world,&args,sizeof(args));
    if (world) {
      SFX(uiactivate)
      modal->defunct=1;
    }
  }
}

static void _hello_render(struct modal *modal) {

  /* Fill with a background slightly darker than the cards.
   */
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x760833ff);

  /* A mess of moving cards.
   */
  graf_set_input(&g.graf,g.texid_sprites);
  struct bgcard *bgcard=MODAL->bgcardv;
  int i=MODAL->bgcardc;
  for (;i-->0;bgcard++) {
    graf_decal(&g.graf,(int)bgcard->x-6,(int)bgcard->y-9,0,96,11,18);
  }
  
  /* The main content: "Dot Vine in ALL FIFTY TWO", including a semitransparent blotter zone for the subtitle.
   */
  graf_set_input(&g.graf,g.texid_modals);
  graf_decal(&g.graf,0,0,0,0,FBW,FBH);
  
  /* Subtitle.
   */
  int alpha=0xff;
  if (MODAL->subtitle_clock<SUBTITLE_FADE_OUT_TIME) alpha=(int)((MODAL->subtitle_clock*255.0)/SUBTITLE_FADE_OUT_TIME);
  else if (MODAL->subtitle_clock>SUBTITLE_PERIOD-SUBTITLE_FADE_IN_TIME) {
    alpha=(int)(((SUBTITLE_PERIOD-MODAL->subtitle_clock)*255.0)/SUBTITLE_FADE_IN_TIME);
  }
  if (alpha>0) {
    int srcx=0,srcy=104+7*MODAL->subtitle;
    if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
    graf_decal(&g.graf,0,44,srcx,srcy,52,7);
    graf_set_alpha(&g.graf,0xff);
  }
}

const struct modal_type modal_type_hello={
  .name="hello",
  .objlen=sizeof(struct modal_hello),
  .init=_hello_init,
  .update=_hello_update,
  .render=_hello_render,
};
