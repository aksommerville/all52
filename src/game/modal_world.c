/* modal_world.c
 * Top level coordinator for the outer world.
 * We remain in scope during battles and cutscenes.
 * We own the session state. (We *are* the session state).
 */

#include "all52.h"

struct modal_world {
  struct modal hdr;
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
  
  return 0;
}

/* Focus.
 */
 
static void _world_focus(struct modal *modal,int focus) {
  fprintf(stderr,"%s %p %d\n",__func__,modal,focus);
}

/* Update.
 */
 
static void _world_update(struct modal *modal,double elapsed,int input) {
  //TODO
}

/* Render.
 */
 
static void _world_render(struct modal *modal) {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000080ff);//TODO
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
