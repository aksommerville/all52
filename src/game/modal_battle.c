/* modal_battle.c
 * We manage the whole card game.
 */

#include "all52.h"

#define INTRO_TIME 1.000
#define BGCOLOR 0x0a5617ff
#define TEXTCOLOR 0xe8ddbbff

#define STAGE_INTRO    1 /* Lower layers remain visible, transition to bg color. */
#define STAGE_PICK     2 /* Player picks her cards. */
#define STAGE_REVEAL   3 /* Each card turns over, non-interactive. */
#define STAGE_ALIGN    4 /* Cards slide to align with the appropriate opponent card. */
#define STAGE_DISBURSE 5 /* Cards slide up or down per suit, to the winner. */

struct modal_battle {
  struct modal hdr;
  int hltx,hlty;
  int pvinput;
  void (*cb)(struct modal *modal);
  void *userdata;
  
  /* Which cards does everybody have, overall?
   * Played cards are also in a hand at all times.
   */
  uint64_t cpu_hand;
  uint64_t man_hand;
  
  /* Which cards are being played?
   */
  uint64_t cpu_play;
  uint64_t man_play;
  
  /* Minimum of the two players' suit counts, ie how many cards get played in this battle.
   */
  int suitc;
  
  int stage;
  double stageclock;
  int prompt,promptw,prompth; // "Pick N"
  int okprompt,okpromptw,okprompth; // "OK?"
  
  int pickp; // 1..4 ie suit
};

#define MODAL ((struct modal_battle*)modal)

/* Cleanup.
 */
 
static void _battle_del(struct modal *modal) {
  egg_texture_del(MODAL->prompt);
  egg_texture_del(MODAL->okprompt);
}

/* Init.
 */
 
static int _battle_init(struct modal *modal,const void *args,int argslen) {

  modal->interactive=1;
  modal->opaque=0; // We'll become opaque after our intro.
  MODAL->stage=STAGE_INTRO;
  MODAL->stageclock=INTRO_TIME;
  MODAL->pickp=1;

  if (!args||(argslen!=sizeof(struct modal_args_battle))) return -1;
  const struct modal_args_battle *ARGS=args;
  MODAL->hltx=ARGS->hltx;
  MODAL->hlty=ARGS->hlty;
  MODAL->cpu_hand=ARGS->cpu_hand;
  MODAL->man_hand=ARGS->man_hand;
  MODAL->cb=ARGS->cb;
  MODAL->userdata=ARGS->userdata;
  
  /* Both players must have at least one card, and they must not overlap.
   */
  if (!MODAL->cpu_hand||!MODAL->man_hand||hands_overlap(MODAL->cpu_hand,MODAL->man_hand)) {
    fprintf(stderr,"%s: Invalid input hands.\n",__func__);
    hand_log("cpu",MODAL->cpu_hand);
    hand_log("man",MODAL->man_hand);
    return -1;
  }
  hand_log("cpu",MODAL->cpu_hand);//XXX
  hand_log("man",MODAL->man_hand);
  
  /* Count the suits in each hand.
   */
  int cpusuitc=hand_count_suits(MODAL->cpu_hand);
  int mansuitc=hand_count_suits(MODAL->man_hand);
  MODAL->suitc=(cpusuitc<mansuitc)?cpusuitc:mansuitc;
  if ((MODAL->suitc<1)||(MODAL->suitc>4)) {
    fprintf(stderr,"%s: Invalid input hands at suit count.\n",__func__);
    hand_log("cpu",MODAL->cpu_hand);
    hand_log("man",MODAL->man_hand);
    return -1;
  }
  fprintf(stderr,"suitc=%d (cpu=%d, man=%d)\n",MODAL->suitc,cpusuitc,mansuitc);//XXX
  
  /* Pick the CPU's play, might as well do it immediately.
   */
  if (!(MODAL->cpu_play=hand_pick_n(MODAL->cpu_hand,MODAL->suitc))) {
    fprintf(stderr,"%s:%d: hand_pick_n() failed\n",__FILE__,__LINE__);
    return -1;
  }
  hand_log("cpu_play",MODAL->cpu_play);//XXX
  
  /* Generate the prompts.
   */
  const char *msg=0;
  switch (MODAL->suitc) {
    case 1: msg="Pick one"; break;
    case 2: msg="Pick two"; break;
    case 3: msg="Pick three"; break;
    case 4: msg="Pick four"; break;
    default: return -1;
  }
  MODAL->prompt=font_render_multiline(msg,-1,FBW,TEXTCOLOR,-1);
  egg_texture_get_size(&MODAL->promptw,&MODAL->prompth,MODAL->prompt);
  MODAL->okprompt=font_render_multiline("OK?",3,FBW,TEXTCOLOR,-1);
  egg_texture_get_size(&MODAL->okpromptw,&MODAL->okprompth,MODAL->okprompt);
  
  return 0;
}

/* Focus.
 */
 
static void _battle_focus(struct modal *modal,int focus) {
}

/* Pick a CPU card at random.
 */
 
static void battle_cpu_pick(struct modal *modal) {
  //TODO Pull one from (cpu_hand) and put it in (cpuplay). Ensure it's a suit not already represented in (cpuplay).
}

/* Update, INTRO.
 */
 
static void battle_update_INTRO(struct modal *modal,double elapsed,int input) {
  if ((MODAL->stageclock-=elapsed)<=0.0) {
    MODAL->stage=STAGE_PICK;
    MODAL->stageclock=0.0;
    modal->opaque=1;
  }
}

/* Update, in PICK stage. The only interactive stage.
 */
 
static void battle_update_PICK(struct modal *modal,double elapsed,int input) {
  // Track input for the player's pick.
  //TODO
}

/* Update, REVEAL.
 */
 
static void battle_update_REVEAL(struct modal *modal,double elapsed,int input) {
  //TODO
}

/* Update, ALIGN.
 */
 
static void battle_update_ALIGN(struct modal *modal,double elapsed,int input) {
  //TODO
}

/* Update, DISBURSE.
 */
 
static void battle_update_DISBURSE(struct modal *modal,double elapsed,int input) {
  //TODO
}

/* Update.
 */
 
static void _battle_update(struct modal *modal,double elapsed,int input) {
  switch (MODAL->stage) {
    case STAGE_INTRO: battle_update_INTRO(modal,elapsed,input); break;
    case STAGE_PICK: battle_update_PICK(modal,elapsed,input); break;
    case STAGE_REVEAL: battle_update_REVEAL(modal,elapsed,input); break;
    case STAGE_ALIGN: battle_update_ALIGN(modal,elapsed,input); break;
    case STAGE_DISBURSE: battle_update_DISBURSE(modal,elapsed,input); break;
    default: modal->defunct=1;
  }
  MODAL->pvinput=input;
  
  //TODO
  if (input&EGG_BTN_WEST) {
    modal->defunct=1;
    if (MODAL->cb) {
      MODAL->cb(modal);
      MODAL->cb=0;
    }
  }
}

/* Render, INTRO.
 */
 
static void battle_render_INTRO(struct modal *modal) {
  double t=1.0-MODAL->stageclock/INTRO_TIME;
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
}

/* Render, PICK.
 */
 
static void battle_render_PICK(struct modal *modal) {
  graf_fill_rect(&g.graf,0,0,FBW,FBH,BGCOLOR);
  graf_set_input(&g.graf,MODAL->prompt);
  graf_decal(&g.graf,0,0,0,0,MODAL->promptw,MODAL->prompth);
  //TODO
  graf_set_input(&g.graf,g.texid_cards);
  graf_decal(&g.graf,1,20,0,0,CARDW,CARDH);
  graf_decal(&g.graf,13,20,CARDW,0,CARDW,CARDH);
  graf_set_input(&g.graf,MODAL->okprompt);
  graf_decal(&g.graf,0,40,0,0,MODAL->okpromptw,MODAL->okprompth);
}

/* Render, REVEAL.
 */
 
static void battle_render_REVEAL(struct modal *modal) {
  //TODO
}

/* Render, ALIGN.
 */
 
static void battle_render_ALIGN(struct modal *modal) {
  //TODO
}

/* Render, DISBURSE.
 */
 
static void battle_render_DISBURSE(struct modal *modal) {
  //TODO
}

/* Render.
 */
 
static void _battle_render(struct modal *modal) {

  /* INTRO alone does not blot the background.
   */
  if (MODAL->stage==STAGE_INTRO) {
    battle_render_INTRO(modal);
    return;
  }
  
  /* All the rest start with the green blotter, then do their thing.
   */
  graf_fill_rect(&g.graf,0,0,FBW,FBH,BGCOLOR);
  switch (MODAL->stage) {
    case STAGE_PICK: battle_render_PICK(modal); break;
    case STAGE_REVEAL: battle_render_REVEAL(modal); break;
    case STAGE_ALIGN: battle_render_ALIGN(modal); break;
    case STAGE_DISBURSE: battle_render_DISBURSE(modal); break;
  }
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

/* Public accessors.
 */
 
void *modal_battle_get_userdata(const struct modal *modal) {
  if (!modal||(modal->type!=&modal_type_battle)) return 0;
  return MODAL->userdata;
}

uint64_t modal_battle_get_cpu_hand(const struct modal *modal) {
  if (!modal||(modal->type!=&modal_type_battle)) return 0;
  return MODAL->cpu_hand;
}

uint64_t modal_battle_get_man_hand(const struct modal *modal) {
  if (!modal||(modal->type!=&modal_type_battle)) return 0;
  return MODAL->man_hand;
}
