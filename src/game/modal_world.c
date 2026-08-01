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
};

#define MODAL ((struct modal_world*)modal)

/* Cleanup.
 */
 
static void _world_del(struct modal *modal) {
}

/* Init.
 */
 
static int _world_init(struct modal *modal,const void *args,int argslen) {

  modal->interactive=1;
  modal->opaque=1;

  if (!args||(argslen!=sizeof(struct modal_args_world))) return -1;
  //TODO digest args
  
  if (world_reset()<0) return -1;
  
  return 0;
}

/* Focus.
 */
 
static void _world_focus(struct modal *modal,int focus) {
  //fprintf(stderr,"%s %p %d\n",__func__,modal,focus);
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

  //TODO weather etc?
  //TODO overlay?
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
