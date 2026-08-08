/* modal_victory.c
 * Splash triggered after you got the 52nd card.
 */
 
#include "all52.h"

struct modal_victory {
  struct modal hdr;
  int texid,texw,texh;
};

#define MODAL ((struct modal_victory*)modal)

static void _victory_del(struct modal *modal) {
  egg_texture_del(MODAL->texid);
}

static void victory_render_time(struct modal *modal,int x,int y,const char *time/*7:mmssfff*/) {
  if (time[0]>'0') graf_decal(&g.graf,x,y,(time[0]-'0')*3,5,3,5); x+=4;
  graf_decal(&g.graf,x,y,(time[1]-'0')*3,5,3,5); x+=6;
  graf_decal(&g.graf,x,y,(time[2]-'0')*3,5,3,5); x+=4;
  graf_decal(&g.graf,x,y,(time[3]-'0')*3,5,3,5); x+=6;
  graf_decal(&g.graf,x,y,(time[4]-'0')*3,5,3,5); x+=4;
  graf_decal(&g.graf,x,y,(time[5]-'0')*3,5,3,5); x+=4;
  graf_decal(&g.graf,x,y,(time[6]-'0')*3,5,3,5);
}

static int _victory_init(struct modal *modal,const void *args,int argslen) {
  modal->opaque=1;
  modal->interactive=1;
  
  /* Not really a presentation concern, but we'll manage sampling and storing the play time right here for simplicity.
   */
  int ms=(int)(g.playclock*1000.0);
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  if ((min<0)||(min>99)) { // Invalid or too high to display, force to "99:99.999"
    min=sec=99;
    ms=999;
  }
  char ntime[7]={
    '0'+min/10,
    '0'+min%10,
    '0'+sec/10,
    '0'+sec%10,
    '0'+ms/100,
    '0'+(ms/10)%10,
    '0'+ms%10,
  };
  char pvtime[7];
  int pvtimec=egg_store_get(pvtime,sizeof(pvtime),"hiscore",7);
  if (pvtimec==sizeof(pvtime)) {
    int i=pvtimec;
    while (i-->0) {
      if ((pvtime[i]<'0')||(pvtime[i]>'9')) {
        memset(pvtime,'9',sizeof(pvtime));
        break;
      }
    }
  } else {
    memset(pvtime,'9',sizeof(pvtime));
  }
  if (memcmp(ntime,pvtime,sizeof(pvtime))<0) {
    egg_store_set("hiscore",7,ntime,sizeof(ntime));
    memcpy(pvtime,ntime,sizeof(ntime));
  }
  
  /* Prepare a texture with the times.
   * Not using font for this, because we want the colon and dot 1 pixel wide instead of 3.
   * (they're baked into the image)
   */
  MODAL->texw=31;
  MODAL->texh=11;
  MODAL->texid=egg_texture_new();
  int err=egg_texture_load_raw(MODAL->texid,MODAL->texw,MODAL->texh,MODAL->texw<<2,0,0);
  egg_texture_clear(MODAL->texid);
  graf_reset(&g.graf);
  graf_set_output(&g.graf,MODAL->texid);
  graf_set_input(&g.graf,g.texid_font);
  victory_render_time(modal,0,0,ntime);
  victory_render_time(modal,0,6,pvtime);
  graf_flush(&g.graf);
  graf_set_output(&g.graf,1);
  
  all52_song(RID_song_cross_currents,1);
  
  return 0;
}

static void _victory_update(struct modal *modal,double elapsed,int input) {
  if (input&EGG_BTN_SOUTH) {
    struct modal *world=modal_spawn(&modal_type_hello,0,0);
    if (world) {
      modal->defunct=1;
    }
  }
}

static void _victory_render(struct modal *modal) {
  graf_set_input(&g.graf,g.texid_modals);
  graf_decal(&g.graf,0,0,FBW,0,FBW,FBH);
  graf_set_input(&g.graf,MODAL->texid);
  graf_decal(&g.graf,20,40,0,0,MODAL->texw,MODAL->texh);
  
  // If she has the pineapple hat, amend the image a little:
  if (g.hat) {
    graf_set_input(&g.graf,g.texid_modals);
    graf_decal(&g.graf,20,31,105,31,7,8);
  }
  
  // Similar overlay if she cheated.
  if (g.cheated) {
    graf_set_input(&g.graf,g.texid_modals);
    graf_decal(&g.graf,28,23,104,14,24,16);
  }
}

const struct modal_type modal_type_victory={
  .name="victory",
  .objlen=sizeof(struct modal_victory),
  .del=_victory_del,
  .init=_victory_init,
  .update=_victory_update,
  .render=_victory_render,
};
