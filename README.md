# NES Reverse-Engineering

Random NES reverse-engineering...

## Solomon's Key GDV

Algorithm to compute GDV (Game Deviation Value) in Solomon's key.

`GDV = (((has_page_of_space + has_page_of_time + 1 + (fairies/10) + saved_princess) * 2 + beaten_levels + solomon_seals) * 2 + hard_levels) / 8 + min(5, score/100000) + has_solomons_key + 47`

[FCEUX script to show real-time GDV.](https://github.com/pellsson/nes_re/blob/master/solomons_key/gdv.lua)

### Interesting GDV-Bug
Bits `#$40` and `#$80` at address `$78` dictate "collected status" for the "Page of Space" and the "Page of Time" respectively. Both pages are included in the algorithm and count towards your GDV, but, are both unconditionally overwritten at the instance you press a button to see your GDV score at the end of the game (Buggy code at `$BD62`).

To patch the bug use Game Genie code `OZTLXS`.

### Values
* `has_page_of_space (addr $78, bit #$40)` Count as 1 if the Page of Space was collected, otherwise 0.
* `has_page_of_time (addr $78, bit #$80)` Count as 1 if the Page of Time was collected, otherwise 0.
* `fairies (addr $86)` Number of fairies collected.
* `saved_princess (addr $7c, bit #$04)` Count as 1 if the Princess was saved, otherwise 0.
* `beaten_levels (addr $85)` Total number of beaten levels, **including** hidden/secret levels.
* `solomon_seals (addr $79)` Number of Solomon Seals collected. [Image of a Solomon Seal.](https://cdn.wikimg.net/en/strategywiki/images/6/67/Solomon%27s_Key_NES_Solomon_Seal.png)
* `hard_levels (addr $84)` All levels beaten after level 10 count towards this **except** hidden/secret levels.
* `score (addr $44A, $44B, $44C)` Is your current score.
* `has_solomons_key (addr $7c, bit #$08)` Count as if if Solomons Key was collected (game beaten), otherwise 0.

## Hydlide Password Generator

[Hydlide Password Generator in C.](https://github.com/pellsson/nes_re/blob/master/hydlide/keygen.c)

