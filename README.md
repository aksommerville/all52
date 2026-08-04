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
- W 2026-08-12
- R 2026-08-13
- F 2026-08-14
- S 2026-08-15 Submit by EOD or die trying.
- U 2026-08-16T07:00 Jam ends.

Actual:
- 2026-08-02T14:49: Completed battle and placement, so the game is technically complete, kind of. And it's fun! Just what I was going for.

## TODO

- [x] Scratch graphics for world and Dot.
- [x] Modal, sprite, and map plumbing.
- [x] Hero movement.
- [x] Camera.
- [x] Battle triggers.
- [x] Battle.
- [x] Should there be persistence? ...no. It plays pretty easy in one sitting. And I don't think there will be any high score worth saving.
- [x] Map tooling. I'm picturing we show the big image, and MacPaint pencil over it for physics.
- [x] Font replacement. Along the lines of Egg's API-wise, but Egg's is overkill.
- [x] Pressing A to dismiss the battle causes the hero's peek to begin briefly. Black it out.
- [x] Battle at DISBURSEMENT stage, it picked the wrong vertical positions when I had both a swap and a take.
- [x] Battle: After picking rank, focus should return to the suit row.
- [x] I think default rank to the first card too, not the "none".

- [x] How many monsters, and how many cards apiece?
- - Dot starts with 6, I like that number. Leaves 46 to distribute.
- - Monster positions and hands will be random, but do keep the distribution fixed (by count at least).
- - And not entirely random either. A given monster will appear in a certain region and have a hand of a certain size.
- - Try: 10, 5,5, 4,4, 3,3,3, 2,2,2, 1,1,1 -- 14 monsters. Seems to work good.

- [ ] Monsters run away.
- [ ] View hand, maybe by pressing B in the outer world?
- [x] Show card count always in outerworld.

- [x] Play time is too long, and the world too big. Need a rethink of the initial shuffle.
- - I'm taking 15 minutes to clear it, and that's knowing exactly what to do.
- - My time should be consistently under 10 minutes. I'd feel better with about 5.
- - ...7 minutes, after eliminating the NW island.
- - ...with free cards placed, it's more like 10 minutes. ok, still close enough.
- - [x] Arrange the world with a big central no-combat zone, and four smaller combat zones peripheral to it.
- - [x] Place a bunch of free cards all around the world initially.
- - [x] Maybe start without any cards? ...yeah i like that

- [x] Instead of the flag borders, can we put a region-completion indicator in the status bar? And include free-card regions too.
- [x] Can we give some fanfare when a region is cleared?
- - How about a ring of flags or something, around the region's edge.
- [x] Probably need some kind of guidance to the remaining monsters.
- [ ] Extra juice after killing a monster.
- [ ] Eliminate battle logging, once I'm comfortable with it.
- [ ] Consider using two full-size images, one for roofs, and mark the roofed regions so it disappears when you're inside.
- [ ] Figure out how we're giving the cross-game clue, and what our bonus is.
- [ ] Proper map.
- [ ] Proper graphics.
- [ ] Music.
- [ ] Sound effects.
- [ ] Hello modal.
- [ ] Game over modal.
- [ ] Victory modal.
- [ ] Dialogue.
- [x] Tile validator.
- [ ] Landing page.

After the jam.
- [ ] Restore universal menu, in metadata.

Expansion ideas, probably not in scope for the jam.
- [ ] Play cards outside battle, like spells.
- [ ] Monsters that play a different game with the same cards.
