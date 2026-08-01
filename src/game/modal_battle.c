#include "all52.h"

#define INTRO_TIME 2.000
#define BGCOLOR 0x0a5617ff

struct modal_battle {
  struct modal hdr;
  int hltx,hlty;
  double introclock; // Counts down.
};

#define MODAL ((struct modal_battle*)modal)

/* Cleanup.
 */
 
static void _battle_del(struct modal *modal) {
}

/* Init.
 */
 
static int _battle_init(struct modal *modal,const void *args,int argslen) {

  modal->interactive=1;
  modal->opaque=0; // We'll become opaque after our intro.
  MODAL->introclock=INTRO_TIME;

  if (!args||(argslen!=sizeof(struct modal_args_battle))) return -1;
  const struct modal_args_battle *ARGS=args;
  MODAL->hltx=ARGS->hltx;
  MODAL->hlty=ARGS->hlty;
    
  return 0;
}

/* Focus.
 */
 
static void _battle_focus(struct modal *modal,int focus) {
}

/* Update.
 */
 
static void _battle_update(struct modal *modal,double elapsed,int input) {

  /* Intro in progress?
   */
  if (MODAL->introclock>0.0) {
    if ((MODAL->introclock-=elapsed)<=0.0) {
      modal->opaque=1;
    } else {
      return;
    }
  }
  
  //TODO
  if (input&EGG_BTN_WEST) modal->defunct=1;
}

/* Render.
 */
 
static void _battle_render(struct modal *modal) {
  
  /* Intro?
   */
  if (MODAL->introclock>0.0) {
    double t=1.0-MODAL->introclock/INTRO_TIME;
    int lmax=MODAL->hltx;
    int rmax=FBW-lmax;
    int tmax=MODAL->hlty;
    int bmax=FBH-tmax;
    if ((lmax<0)||(rmax<0)||(tmax<0)||(bmax<0)) {
      // We got OOB coordinates for the focus point. Do a plain fade instead.
      int alpha=(int)(t*255.0);
      if (alpha>0) {
        if (alpha>0xff) alpha=0xff;
        graf_fill_rect(&g.graf,0,0,FBW,FBH,(BGCOLOR&0xffffff00)|alpha);
      }
    } else {
      // Close a rectangle in on the focus point.
      int lw=lround(t*lmax);
      int rw=lround(t*rmax);
      int th=lround(t*tmax);
      int bh=lround(t*bmax);
      if (th>0) { // The top part gets a shadow under.
        graf_fill_rect(&g.graf,0,0,FBW,th+1,0x00000080);
        graf_fill_rect(&g.graf,0,0,FBW,th,BGCOLOR);
      }
      if (bh>0) graf_fill_rect(&g.graf,0,FBH-bh,FBW,bh,BGCOLOR);
      if (lw>0) graf_fill_rect(&g.graf,0,0,lw,FBH,BGCOLOR);
      if (rw>0) graf_fill_rect(&g.graf,FBW-rw,0,rw,FBH,BGCOLOR);
    }
    return;
  }
  
  graf_fill_rect(&g.graf,0,0,FBW,FBH,BGCOLOR);
  //TODO
}

/* Type definition.
 */
 
const struct modal_type modal_type_battle={
  .name="battle",
  .objlen=sizeof(struct modal_battle),
  .del=_battle_del,
  .init=_battle_init,
  .focus=_battle_focus,
  .update=_battle_update,
  .render=_battle_render,
};
