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
 - World physics are one bit per cell. Solid or vacant, no further detail needed.
 - - Store binary. A plain 512 byte bitmap.
 - All movement quantizes to cells but present it fluidly. What we did in Spelling Bee was good.
 - Vacant tiles must be of light colors only, and sprites should have dark edges.
 - Can assume there won't be any doors or interiors.
 - I think we can get by with just two images: World and Sprites.
 - - Mmm yeah, but we'll probably need more for the modals. Possibly a separate Battle image too.
 - - And we need a 3x5 font image, definitely.

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
- W 2026-08-12 js13k begins tomorrow. Really prefer to be done with this one by today.
- R 2026-08-13
- F 2026-08-14
- S 2026-08-15 Submit by EOD or die trying.
- U 2026-08-16T07:00 Jam ends.

Actual:
- 2026-08-02T14:49: Completed battle and placement, so the game is technically complete, kind of. And it's fun! Just what I was going for.

## TODO

- [ ] Is it possible for some draw to be unwinnable? I'm pretty sure it's not, but prove it.
- [ ] View hand, maybe by pressing B in the outer world?
- [ ] Extra juice after killing a monster.
- [ ] Consider using two full-size images, one for roofs, and mark the roofed regions so it disappears when you're inside.
- [ ] Figure out how we're giving the cross-game clue, and what our bonus is.
- [ ] Proper map.
- [ ] Proper graphics.
- [ ] Music.
- [ ] Sound effects.
- [x] Hello modal.
- [x] Game over modal.
- [x] Victory modal.
- [ ] Landing page.

After the jam.
- [ ] Restore universal menu, in metadata.

Expansion ideas, probably not in scope for the jam.
- [ ] Play cards outside battle, like spells.
- [ ] Monsters that play a different game with the same cards.
