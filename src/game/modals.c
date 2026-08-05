#include "all52.h"

/* Globals.
 */
 
static struct {
  int init;
  struct modal **v;
  int c,a;
} modals={0};

/* Quit.
 */
 
void modals_quit() {
  while (modals.c-->0) {
    struct modal *modal=modals.v[modals.c];
    modal->registered=0;
    modal_del(modal);
  }
  if (modals.v) free(modals.v);
  memset(&modals,0,sizeof(modals));
}

/* Init.
 */
 
int modals_init() {
  if (modals.init) return -1;
  modals.init=1;
  return 0;
}

/* Update.
 */

void modals_update(double elapsed,int input) {

  /* First update everything that isn't defunct.
   * Capture the topmost interactive, and if it's not our current focus, update them.
   */
  struct modal *focus=0;
  int i=modals.c;
  while (i-->0) {
    struct modal *modal=modals.v[i];
    if (modal->defunct) continue;
    
    if (modal->interactive&&!focus) {
      focus=modal;
      if (!modal->focus) {
        modal->focus=1;
        if (modal->type->focus) modal->type->focus(modal,1);
      }
    } else if (modal->focus) {
      modal->focus=0;
      if (modal->type->focus) modal->type->focus(modal,0);
    }
    
    if (!focus||(focus==modal)||modal->update_when_blurred) {
      int minput=input;
      if (modal->input_blackout) {
        if (!(minput&(EGG_BTN_WEST|EGG_BTN_SOUTH))) modal->input_blackout=0;
        else minput&=~(EGG_BTN_WEST|EGG_BTN_SOUTH);
      }
      if (modal->type->update) modal->type->update(modal,elapsed,minput);
    }
  }
  
  /* Then another pass to kill the defunct ones.
   */
  for (i=modals.c;i-->0;) {
    struct modal *modal=modals.v[i];
    if (!modal->defunct) continue;
    modals.c--;
    memmove(modals.v+i,modals.v+i+1,sizeof(void*)*(modals.c-i));
    modal->registered=0;
    modal_del(modal);
  }
}

/* Render.
 */
 
void modals_render() {

  /* Find the topmost opaque modal.
   * There shouldn't be anything defunct during render, but check anyway to be consistent.
   */
  int opaquep=-1;
  int i=modals.c;
  while (i-->0) {
    struct modal *modal=modals.v[i];
    if (modal->defunct) continue;
    if (modal->opaque) {
      opaquep=i;
      break;
    }
  }
  
  /* If nothing is opaque, black out the framebuffer and set (opaquep) zero.
   */
  if (opaquep<0) {
    opaquep=0;
    graf_fill_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
  }
  
  /* Render everything from (opaquep) upward, in order.
   */
  for (;opaquep<modals.c;opaquep++) {
    struct modal *modal=modals.v[opaquep];
    if (modal->defunct) continue;
    if (modal->type->render) modal->type->render(modal);
  }
}

/* Read stack.
 */

int modals_count() {
  return modals.c;
}

struct modal *modal_by_index(int p) {
  if ((p<0)||(p>=modals.c)) return 0;
  return modals.v[p];
}

struct modal *modal_get_focus() {
  int i=modals.c;
  while (i-->0) {
    struct modal *modal=modals.v[i];
    if (modal->focus) return modal;
  }
  return 0;
}

struct modal *modal_topmost_of_type(const struct modal_type *type) {
  int i=modals.c;
  while (i-->0) {
    struct modal *modal=modals.v[i];
    if (modal->type==type) return modal;
  }
  return 0;
}

/* Push. Handoff.
 */

int modals_push(struct modal *modal) {
  if (!modal) return -1;
  if (modal->registered) return -1;
  if (modals.c>=modals.a) {
    int na=modals.a+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(modals.v,sizeof(void*)*na);
    if (!nv) return -1;
    modals.v=nv;
    modals.a=na;
  }
  modals.v[modals.c++]=modal;
  modal->registered=1;
  return 0;
}

/* Remove. Handoff.
 */

int modals_remove(struct modal *modal) {
  if (!modal||!modal->registered) return -1;
  int i=modals.c;
  while (i-->0) {
    if (modals.v[i]==modal) {
      modals.c--;
      memmove(modals.v+i,modals.v+i+1,sizeof(void*)*(modals.c-i));
      modal->registered=0;
      if (modal->focus) { // Send the blur notification now, since update won't be able to.
        modal->focus=0;
        if (modal->type->focus) modal->type->focus(modal,0);
      }
      return 0;
    }
  }
  return -1;
}

/* Kill soon.
 */
 
void modal_kill(struct modal *modal) {
  if (!modal) return;
  modal->defunct=1;
  if (modal->focus) {
    modal->focus=0;
    if (modal->type->focus) modal->type->focus(modal,0);
  }
}

/* Spawn.
 */
 
struct modal *modal_spawn(const struct modal_type *type,const void *args,int argslen) {
  struct modal *modal=modal_new(type,args,argslen);
  if (!modal) return 0;
  if (modals_push(modal)<0) {
    modal_del(modal);
    return 0;
  }
  return modal;
}

/* Delete.
 */
 
void modal_del(struct modal *modal) {
  if (!modal) return;
  if (modal->registered) fprintf(stderr,"%s:%d:ERROR: modal %p (%s) being deleted but claims to be registered.\n",__FILE__,__LINE__,modal,modal->type->name);
  if (modal->type->del) modal->type->del(modal);
  free(modal);
}

/* New.
 */
 
struct modal *modal_new(const struct modal_type *type,const void *args,int argslen) {
  if (!type||(argslen<0)||(argslen&&!args)) return 0;
  struct modal *modal=calloc(1,type->objlen);
  if (!modal) return 0;
  
  modal->type=type;
  modal->input_blackout=1;
  
  if (type->init&&((type->init(modal,args,argslen)<0)||modal->defunct)) {
    modal_del(modal);
    return 0;
  }
  return modal;
}
