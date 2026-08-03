#include "all52.h"

struct sprite_monster {
  struct sprite hdr;
  uint64_t hand;
  uint8_t tileid;
  double faceclock;
  uint8_t xform;
  int family;
};

#define SPRITE ((struct sprite_monster*)sprite)

/* Cleanup.
 */
 
static void _monster_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _monster_init(struct sprite *sprite,const void *args,int argslen) {
  if (args&&(argslen==sizeof(struct sprite_args_monster))) {
    //TODO digest args
  }
  SPRITE->tileid=0x00; // I'm Dot if they forget to set this.
  SPRITE->faceclock=((rand()&0xffff)/65535.0);
  return 0;
}

/* Update.
 */
 
static void _monster_update(struct sprite *sprite,double elapsed) {
  // Turn to face the hero. But don't bother checking every frame; allow say a quarter second between polls.
  if ((SPRITE->faceclock-=elapsed)<=0.0) {
    SPRITE->faceclock+=0.250;
    struct sprite *hero=world_get_hero();
    if (hero) {
      if (hero->x<sprite->x-0.250) SPRITE->xform=EGG_XFORM_XREV;
      else if (hero->x>sprite->x+0.250) SPRITE->xform=0;
    }
  }
}

/* Bump.
 */
 
static struct {
  int monsterid;
  int heroid;
} monster_battle_userdata={0};

static void monster_cb_battle(struct modal *modal) {
  struct sprite *sprite=sprite_by_id(monster_battle_userdata.monsterid);
  struct sprite *hero=sprite_by_id(monster_battle_userdata.heroid);
  if (!sprite||!hero) {
    fprintf(stderr,
      "%s:PANIC: Sprite not found after battle. monster:%d=>%p, hero:%d=>%p\n",
      __func__,monster_battle_userdata.monsterid,sprite,monster_battle_userdata.heroid,hero
    );
    return;
  }
  uint64_t manhand=modal_battle_get_man_hand(modal);
  uint64_t cpuhand=modal_battle_get_cpu_hand(modal);
  hand_log("cpu after",cpuhand);
  hand_log("man after",manhand);
  sprite_hero_set_hand(hero,manhand);
  if (SPRITE->hand=cpuhand) {
    fprintf(stderr,"...monster is still alive. run away\n");//TODO
  } else {
    fprintf(stderr,"...thou hast done well in defeating the monster\n");//XXX
    sprite->defunct=1;
    sprite_flag_refresh();
    if (manhand==0x000fffffffffffffll) {
      fprintf(stderr,"YOU GOT ALL FIFTY TWO!\n");//TODO
    }
  }
}
 
static int _monster_bump(struct sprite *sprite,struct sprite *hero) {
  monster_battle_userdata.monsterid=sprite->id;
  monster_battle_userdata.heroid=hero->id;
  struct modal_args_battle args={
    .cpu_hand=SPRITE->hand,
    .man_hand=sprite_hero_get_hand(hero),
    .cb=monster_cb_battle,
    .userdata=&monster_battle_userdata,
  };
  modal_world_get_sprite_render_position(&args.hltx,&args.hlty,modal_topmost_of_type(&modal_type_world),sprite);
  struct modal *modal=modal_spawn(&modal_type_battle,&args,sizeof(args));
  if (!modal) {
    fprintf(stderr,"%s: Spawing battle modal failed!\n",__func__);
  }
  return 1;
}

/* Render.
 */
 
static void _monster_render(struct sprite *sprite,int x,int y) {
  graf_tile(&g.graf,x,y,SPRITE->tileid,SPRITE->xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_monster={
  .name="monster",
  .objlen=sizeof(struct sprite_monster),
  .del=_monster_del,
  .init=_monster_init,
  .update=_monster_update,
  .render=_monster_render,
  .bump=_monster_bump,
};

/* Public accessors.
 */

uint64_t sprite_monster_get_hand(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return 0;
  return SPRITE->hand;
}

int sprite_monster_set_hand(struct sprite *sprite,uint64_t hand) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return -1;
  SPRITE->hand=hand;
  return 0;
}

int sprite_monster_set_tileid(struct sprite *sprite,uint8_t tileid) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return -1;
  SPRITE->tileid=tileid;
  return 0;
}

int sprite_monster_set_family(struct sprite *sprite,int family) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return -1;
  SPRITE->family=family;
  return 0;
}

int sprite_monster_get_family(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_monster)) return 0;
  return SPRITE->family;
}
