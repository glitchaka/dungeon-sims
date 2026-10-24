# Dios Obrando Misteriosamente

Developer notes; this feature has no HUD button, hint or cooldown indicator.

Hold the left mouse button for at least 0.2 seconds, drag at least 7 screen pixels and release over the map. Short clicks still inspect. Escape or release outside the map cancels without consuming the power. A successful relocation triggers the curved, extruded rainbow WordArt title and golden particles. The cooldown is 180 real seconds, independent of simulation speed or pause, and its deadline is saved; elapsed time while the game is closed counts.

Residents, land animals, props (including homes and trees), crops, camps, gifts, roads, fish and boats are supported. Sea/freshwater tiles can be carried onto empty land: the source becomes a small dry tile and the destination becomes water. Navigation and the terrain texture are rebuilt. Land entities require free land; fish require water; boats require a valid sea route or a new coastal berth. Invalid destinations show a red halo and do not change the world or cooldown. Home and family references move with houses. Jobs and travel reservations are released when carrying a resident.

`MysteriousPower` owns world mutations and validation; `MysteriousGesture` owns input recognition; `MysteriousView` owns held sprites, particles and WordArt. `SeaLife` owns fish seeding and swimming. The optional `seaFish` and `mysteriousReadyAt` save fields keep existing v9 profiles compatible. Fish use the generated `assets/sea-fish-v8.png`; its prompt is in `assets/MYSTERY-V8-PROMPTS.md`.

`mysterious_divine_drag` checks transfers, occupancy, family homes, map edges, atomic failures, fish persistence, legacy saves, mouse gestures and cooldown boundaries. A real renderer capture can be generated without saving a profile:

```
isla_viva_native.exe --capture preview.png --seed 1749 --mystery-preview
```


