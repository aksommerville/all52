/* shared_symbols.h
 * This file is consumed by eggdev and editor, in addition to compiling in with the game.
 */

#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define EGGDEV_importUtil "res,text,graf,stdlib" /* Comma-delimited list of Egg 'util' units to include in the build. */
#define EGGDEV_ignoreData "" /* Comma-delimited glob patterns for editor and builder to ignore under src/data/ */

#define NS_sys_tilesize 8
#define NS_sys_bgcolor 0x000000

#define CMD_sprite_image 0x20 /* u16:imageid */
#define CMD_sprite_tile  0x21 /* u8:tileid, u8:xform */
#define CMD_sprite_type  0x22 /* u16:sprtype */

#define NS_sprtype_dummy 0 /* (u32)0 */
#define FOR_EACH_SPRTYPE \
  _(dummy)

#endif
