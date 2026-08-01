# All Fifty Two

Requires [Egg v2](https://github.com/aksommerville/egg2) to build.

For [LowRezJam 2026](https://itch.io/jam/lowrezjam-2026), August 2026.
Ignoring the optional themes, to make this game that's been on my mind for a few months.

Open world adventure, think Pokemon, where you play a simple card game against the monsters, with the goal of winning all their cards.
A 52-card poker deck is distributed initially around all the monsters of the world.

The card game:
 - Aces are low, and we'll call them "ones".
 - Each player selects one card of each suit.
 - - If either player is missing a suit, the computer tells you, and you play only the max suit count.
 - Played cards of the same suit, whichever rank is higher, that player wins both cards.
 - Any remaining cards of mismatched suit, they get swapped.
 - One round at a time.
 
Further requirements:
 - 52x52 pixel framebuffer. Must be <=64, and hey the game's all about the number 52, innit?
 - 64x64 world of 8x8 pixel tiles.
 - Every world tile is unique. I'll make it as a single 512x512 pixel image. Write a script to validate uniqueness.
 - World physics are one bit per cell. Passable or vacant, no further detail needed.
 - All movement quantizes to cells but present it fluidly. What we did in Spelling Bee was good.

## Agenda

- S 2026-08-01 Sprites, modals, outerworld movement, enter battles.
- U 2026-08-02 Battles, cutscenes. Aim to be functionally complete by EOD.
- M 2026-08-03
- T 2026-08-04
- W 2026-08-05
- R 2026-08-06
- F 2026-08-07
- S 2026-08-08 Layout and graphics. Choose my secret clue and get it to Skibbl.
- U 2026-08-09 Audio. Plan to submit today but it's ok to miss it.
- M 2026-08-10
- T 2026-08-11
- W 2026-08-12
- R 2026-08-13
- F 2026-08-14
- S 2026-08-15 Submit by EOD or die trying.
- U 2026-08-16T07:00 Jam ends.

## TODO
