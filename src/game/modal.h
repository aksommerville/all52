/* modal.h
 * Defines the modal stack and generic interface.
 * This should be consumed by main.
 */
 
#ifndef MODAL_H
#define MODAL_H

/* Global.
 ***********************************************************************************/

/* Global lifecycle hooks.
 * Rendering overwrites the entire framebuffer.
 */
void modals_quit();
int modals_init();
void modals_update(double elapsed,int input);
void modals_render();

int modals_count();
struct modal *modal_by_index(int p); // 0 in the back
struct modal *modal_get_focus();
struct modal *modal_topmost_of_type(const struct modal_type *type);

/* Pushing and removing transfer ownership.
 * Killing arranges for the modal to be deleted at the end of the next or current update.
 */
int modals_push(struct modal *modal);
int modals_remove(struct modal *modal);
void modal_kill(struct modal *modal);
struct modal *modal_spawn(const struct modal_type *type,const void *args,int argslen);

/* Generic modal.
 *****************************************************************************/
 
struct modal {
  const struct modal_type *type;
  int defunct;
  int interactive; // Topmost with this nonzero receives all input, exclusively.
  int opaque; // Topmost with this nonzero, nothing below it renders.
  int update_when_blurred; // If nonzero, we request update events even when an interactive modal is above us.
  int input_blackout; // Initially nonzero. You may clear at init if you want the raw initial state, otherwise globally we wait for it to go zero first.
  int focus; // VOLATILE, READONLY. Only the global modal stack should set this. Nonzero if you have input focus.
  int registered; // PRIVATE.
};

struct modal_type {
  const char *name;
  int objlen;
  void (*del)(struct modal *modal);
  
  /* Return <0 or set (defunct) to abort construction.
   */
  int (*init)(struct modal *modal,const void *args,int argslen);
  
  /* Called every frame for the focus modal and all above it, and all below if they have (update_when_blurred) set.
   * BEWARE: This is generally not called before the first render, in the very likely case that the modal is spawned by another modal's update.
   * So make sure you're in a coherent renderable state after (init).
   */
  void (*update)(struct modal *modal,double elapsed,int input);
  
  /* Called when you gain or lose focus.
   * This normally happens during the update loop, immediately before your (update).
   * But can also trigger synchronously on certain removal events.
   */
  void (*focus)(struct modal *modal,int focus);
  
  /* If you have (opaque) nonzero, you must overwrite the entire framebuffer.
   */
  void (*render)(struct modal *modal);
};

void modal_del(struct modal *modal);
struct modal *modal_new(const struct modal_type *type,const void *args,int argslen);

extern const struct modal_type modal_type_world;
extern const struct modal_type modal_type_battle;
extern const struct modal_type modal_type_dialogue; // Overlays world.
extern const struct modal_type modal_type_hello;
extern const struct modal_type modal_type_gameover;
extern const struct modal_type modal_type_victory;
extern const struct modal_type modal_type_status; // During outerworld play, slides up from bottom on demand.

/* Species.
 ***************************************************************************/
 
struct modal_args_world {
  int TODO;
};

void modal_world_get_sprite_render_position(int *x,int *y,const struct modal *modal,const struct sprite *sprite);

struct modal_args_battle {
  int hltx,hlty; // Framebuffer position to highlight where we originate, should be the center of the monster.
  uint64_t cpu_hand;
  uint64_t man_hand;
  void (*cb)(struct modal *modal);
  void *userdata;
};

void *modal_battle_get_userdata(const struct modal *modal);
uint64_t modal_battle_get_cpu_hand(const struct modal *modal);
uint64_t modal_battle_get_man_hand(const struct modal *modal);

struct modal_args_dialogue {
  int rid;
  int strix;
  const struct text_insertion *insv;
  int insc;
};
 
#endif
