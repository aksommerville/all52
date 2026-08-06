/* modal_world.c
 * Top level coordinator for the outer world.
 * We remain in scope during battles and cutscenes.
 * The model underlying a session should mostly live elsewhere. Our job is just coordination.
 * We do own the camera.
 */

#include "all52.h"

struct modal_world {
  struct modal hdr;
  int camx,camy; // Most recent scroll position in world pixels (NB not meters). Top left.
  int cardc_texid,cardc_w,cardc_h; // Text texture showing the card count.
  int cardc_visible; // The card count we're displaying, so we can detect changes.
};

#define MODAL ((struct modal_world*)modal)

/* Cleanup.
 */
 
static void _world_del(struct modal *modal) {
  egg_texture_del(MODAL->cardc_texid);
}

/* Init.
 */
 
static int _world_init(struct modal *modal,const void *args,int argslen) {

  modal->interactive=1;
  modal->opaque=1;
  MODAL->cardc_visible=-1;

  g.finish=0;
  g.playclock=0.0;
  if (world_reset()<0) return -1;
  
  return 0;
}

/* Focus.
 */
 
static void _world_focus(struct modal *modal,int focus) {
  //fprintf(stderr,"%s %p %d\n",__func__,modal,focus);
  if (focus) {
  } else {
    // eg dialogue appearing over me. restart the blackout.
    modal->input_blackout=1;
  }
}

/* Update.
 */
 
static void _world_update(struct modal *modal,double elapsed,int input) {
  world_update(elapsed,input);
}

/* Render.
 */
 
static void _world_render(struct modal *modal) {

  /* Select scroll position.
   * I think we can be pretty naive about this.
   * Center on the hero, then offset if she is peeking, then clamp to the world.
   * We do record the most recent scroll, in case the hero disappearoes.
   */
  int camx=MODAL->camx,camy=MODAL->camy;
  struct sprite *hero=world_get_hero();
  if (hero) {
    double prex=hero->x*NS_sys_tilesize-FBW*0.5;
    double prey=hero->y*NS_sys_tilesize-FBH*0.5;
    double px,py;
    if (sprite_hero_get_peeking(&px,&py,hero)) {
      const double peekrange=FBW*0.5-4.0;
      px*=peekrange;
      py*=peekrange;
      prex+=px;
      prey+=py;
    }
    camx=lround(prex);
    camy=lround(prey);
    if (camx<0) camx=0; else if (camx>MAPW*NS_sys_tilesize-FBW) camx=MAPW*NS_sys_tilesize-FBW;
    if (camy<0) camy=0; else if (camy>MAPH*NS_sys_tilesize-FBH) camy=MAPH*NS_sys_tilesize-FBH;
    MODAL->camx=camx;
    MODAL->camy=camy;
  }
  
  /* Rendering the map is way easier than anything we're used to: It's just a flat image, the whole world.
   */
  graf_set_input(&g.graf,g.texid_world);
  graf_decal(&g.graf,0,0,camx,camy,FBW,FBH);
  
  /* Render sprites.
   */
  int xlo=camx-NS_sys_tilesize;
  int xhi=camx+FBW+NS_sys_tilesize;
  int ylo=camy-NS_sys_tilesize;
  int yhi=camy+FBH+NS_sys_tilesize;
  graf_set_input(&g.graf,g.texid_sprites);
  struct sprite **p;
  int i=world_get_sprites(&p);
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->defunct) continue;
    if (!sprite->type->render) continue; // Invisible sprites are totally legal. And there's no default regime.
    int x=lround(sprite->x*NS_sys_tilesize);
    int y=lround(sprite->y*NS_sys_tilesize);
    if (!sprite->render_always) { // Can we cull it due to off screen?
      if ((x<xlo)||(x>xhi)) continue;
      if ((y<ylo)||(y>yhi)) continue;
    }
    x-=camx;
    y-=camy;
    sprite->type->render(sprite,x,y);
  }
  
  /* Refresh card-count texture if necessary.
   */
  if (hero) {
    int cardc=hand_count_cards(sprite_hero_get_hand(hero));
    if (cardc!=MODAL->cardc_visible) {
      graf_flush(&g.graf);
      MODAL->cardc_visible=cardc;
      egg_texture_del(MODAL->cardc_texid);
      char text[2];
      int textc=0;
      if (cardc>=10) text[textc++]='0'+cardc/10;
      text[textc++]='0'+cardc%10;
      MODAL->cardc_texid=font_render_multiline(text,textc,FBW,0xffffffff,0);
      egg_texture_get_size(&MODAL->cardc_w,&MODAL->cardc_h,MODAL->cardc_texid);
    }
  }
  
  /* Top bar overlay.
   */
  int region_complete=world_describe_local_completion();
  if (region_complete) {
    if (region_complete>0) {
      graf_decal(&g.graf,0,0,49,106,7,5);
    } else {
      graf_decal(&g.graf,0,0,42,106,7,5);
    }
  }
  graf_set_input(&g.graf,MODAL->cardc_texid);
  graf_decal(&g.graf,8,0,0,0,MODAL->cardc_w,MODAL->cardc_h);
}

/* Type definition.
 */
 
const struct modal_type modal_type_world={
  .name="world",
  .objlen=sizeof(struct modal_world),
  .del=_world_del,
  .init=_world_init,
  .focus=_world_focus,
  .update=_world_update,
  .render=_world_render,
};

/* Where is this sprite rendered?
 */
 
void modal_world_get_sprite_render_position(int *x,int *y,const struct modal *modal,const struct sprite *sprite) {
  if (!modal||(modal->type!=&modal_type_world)) return;
  if (!sprite) return;
  *x=lround(sprite->x*NS_sys_tilesize)-MODAL->camx;
  *y=lround(sprite->y*NS_sys_tilesize)-MODAL->camy;
}
