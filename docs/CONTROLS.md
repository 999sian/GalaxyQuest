# Controls (Meta Quest Touch Plus)

The game still sees a Wii Remote with a Nunchuk attached. The Touch Plus
controllers are mapped onto them like this. The mapping lives in
`updateInput()` in `platform/src/xr/xr_app.cpp`.

| Touch Plus | Wii | In Super Mario Galaxy |
|---|---|---|
| Left thumbstick | Nunchuk stick | Move |
| **A** | A | Jump, talk, confirm. Hold to skip a cutscene or a dialogue |
| **B** | Shake the Remote | Spin |
| **Y** | Shake the Remote | Spin |
| Flick the right controller | Shake the Remote | Spin |
| Flick the left controller | Shake the Nunchuk | Spin, keeping the pointer steady |
| Right trigger | B | Shoot star bits, back/cancel in menus |
| Right controller aim | Pointer | Collect star bits, grab Pull Stars, point at menus |
| Left trigger | Z | Crouch, ground pound, long / backflip jumps (with A) |
| Left grip | C | Put the camera behind Mario |
| Menu (left) | + | Pause menu (one press) |
| X | − | Pause menu (one press) |
| Right stick left / right | D-pad left / right | In the diorama: turn it a step (45°) round Mario, behind a short blink. On the giant screen: turn the game camera around Mario in steps, where the level allows it |
| Right stick up, or click | D-pad up | First-person look (shown on the virtual screen); page up in lists |
| Right stick down | D-pad down | Page down in lists |

Rumble is played on both controllers' haptics.

## Tilt

Rolling the Star Ball and surfing on the Ray steer with the Wii Remote's
tilt. There, the right controller's orientation takes its place:

- **Star Ball:** hold the right controller level (it stands for the Wii
  Remote held straight up, as the sign asks). Tip it down to roll forward,
  raise it to roll back, and roll it left or right to steer.
- **Ray:** hold the right controller level, pointing ahead (the penguin's
  "point your Wii Remote at the screen"), and twist it left or right to
  turn.

Elsewhere, the game sees a Wii Remote held level and still, so aiming the
laser never counts as a shake.

## Pointing

- **In gameplay (diorama):** a laser and reticle show where the right
  controller aims; the game's own cursor is hidden. Star bits, Pull Stars
  and enemies respond to the laser itself, near or far. The reticle swells
  and the controller ticks when the laser touches one. The laser runs on to
  the first surface in its way, or far past Mario into open space. Star
  bits you shoot leave from the controller and fly along the laser to its
  end.
- **In menus:** the pause menu and yes/no prompts appear on the HUD panel in
  front of you, and the title and file select appear on the virtual screen.
  Point at them directly, the same way you point a Wii Remote at a TV.

## View

- To recentre, hold the Meta button on the right controller. The diorama
  and HUD panel are placed relative to your head when the app starts or
  recentres.
- By default the world is at 1/500 scale, with Mario about 1.5 m in front
  of you and 0.8 m below eye level.
- When scenery hides Mario from you, the part between you and him fades
  away until he is in view again.
- Choosing a galaxy in the observatory domes, and the first-person view,
  happen on the virtual screen.

## Skipping cutscenes and dialogues

Hold **A** during a cutscene or a dialogue to skip it. After a moment a ring appears at
the lower right of your view and fills while you keep holding. When it is
full, the cutscene is skipped. If you let go early, the ring drains away and
the cutscene carries on. A short press still advances text as usual.

- Movies, the camera tour when you arrive in a galaxy, and Mario's flight
  into it stop at once, the way the game skips them itself.
- Other cutscenes run to their end at high speed while the view is dark and
  the ring spins. Text boxes are advanced for you. If a cutscene asks a
  yes/no question, skipping stops there so you can answer it.
- A dialogue (talking to a Luma, a Toad and so on, most of which are no
  cutscene) runs to its end the same way, all its pages, not just the one
  on screen; it too stops at a yes/no question.

To skip again, let go of A first. The A you held never reaches the game as
a jump. Holding A from before a cutscene started doesn't count.
`skip_hold` in the comfort settings below sets how long to hold.

## Playing as Luigi

Every save file can be played with Mario or Luigi, a new one included:
you do not need to finish the game with Mario first. On a file's start
screen (the one with **Play This File**, which also follows creating a
file), point at the Mario/Luigi button under the file's details and press
A. The file's bar turns green for Luigi. Mario and Luigi keep separate
progress in the same file; the button switches between them. The file's
icon is your own choice either way (Luigi's is among the icons too).

## Pausing

A single press of X or the Menu button opens the pause menu (on the Wii, +
or - had to be held for a fifth of a second, and not while A or B was held
or during a spin). Where the game does not allow pausing for a moment
(while Mario is being hit, during a screen transition) the menu opens as
soon as it does, up to 1.5 s after the press; during cutscenes it does not
open, as on the Wii.

Opening the system menu or taking the headset off pauses the game. When you
come back during play, the game's pause menu is open.

## VR settings

While the pause menu is open (hold the Menu button or X for a moment), a
VR settings panel appears to the right of its buttons, slightly turned
towards you. Aim the right controller at it and press A or pull the trigger:

- **−** and **+** move Mario 10 cm nearer to you or farther away (hold to
  repeat).
- Drag the slider to set the distance directly, from 0.6 m to 4 m. The mark
  under it is the default.
- **Reset** returns to the default, 1.5 m.
- **Smooth motion** turns SpaceWarp on or off (on by default): with it the
  headset makes the in-between frames from the game's motion, so grass,
  flowers and the ground stay sharp while the view follows Mario; off,
  each game frame is shown twice and moving scenery looks doubled. It
  takes effect at once, to compare.
- **Super resolution** (on by default) hands the world to the headset at the
  size it was rendered and lets Meta Quest Super Resolution scale it to the
  display and sharpen it; off, the app scales it itself (softer). It also
  takes effect at once. The HUD, the pause menu and this panel go to the
  headset as layers of their own, so their text stays sharp either way.
- **FidelityFX CAS** (off by default) sharpens the world with AMD's
  contrast adaptive sharpening as the app puts it into the eye images (and
  scales it up there when Super Resolution is off). Its strength is
  `sharpening_strength` below.
- **Lowest resolution** (- and +, 0.80 by default) is as far as the game may
  lower its render resolution when the GPU is busy; "Now" shows the
  resolution it renders at. 1.00 is Meta's standard eye size. Higher stays
  sharper, but when the GPU cannot keep up frames come late and the view
  stutters.
- **Giant screen** (on by default) plays the game on a big 16:9 virtual
  screen from the game's own camera, as on a TV; off, the game is a
  diorama in front of you. While it is on, the slider at the top sets the
  **screen distance** instead of Mario's: 2.5 m to 10 m in steps of 0.5 m,
  4.5 m by default. The screen is 5.33 m wide, so it spans 61 degrees at
  4.5 m and 30 degrees (a TV seen from the couch) at 10 m.

The paused scene behind the menu moves as you change the distance, so you
can judge it before you carry on. The settings are kept in
`diorama_distance`, `screen_distance`, `space_warp`, `super_resolution`,
`sharpening`, `min_resolution` and `giant_screen` in `petari_vr.ini` (see
below), written when the menu closes.

## Comfort settings

The view can be tuned without rebuilding. Create `petari_vr.ini` next to the
game data, in `/sdcard/Android/data/com.galaxy.quest/files/`, with any of
these lines. The file is read when the app starts.

```
# Size of the world (1 = default, 1.5 = half as big again)
diorama_scale = 1
# Where Mario stands: metres below your eyes and in front of you (the
# distance can also be set on the VR settings panel in the pause menu)
diorama_height = 0.8
diorama_distance = 1.5
# How quickly the world follows Mario and turns with his gravity (seconds
# to close half the gap; larger is gentler). On a small planet it turns a
# little quicker (at most 115 degrees a second) while the view
# would otherwise sink below Mario's horizon; a sudden change of gravity (a
# gravity switch, a room's wall becoming the floor) happens at once behind a
# short blink instead.
# (seconds to close half the gap; larger is gentler)
follow_smoothing = 0.12
turn_smoothing = 0.35
# Darkening at the edges while the world turns (0 = off, 1 = full)
vignette = 1
# Fade out scenery that hides Mario (0 = off)
cutaway = 1
# Seconds to hold A to skip a cutscene (0 = never skip)
skip_hold = 1
# Largest render resolution, relative to the headset's recommended eye
# size (1680x1760 on Quest 3). The resolution rises towards this while the
# GPU has time to spare and drops (down to min_resolution) as soon as
# frames are missed. Lower it to save battery.
resolution = 1.6
# The lowest render resolution (0.8 to 1.25; also on the VR settings panel)
min_resolution = 0.8
# Display refresh rate in Hz. At 120 the game's 60 frames a second fit
# the display exactly; other rates make moving things judder.
refresh_rate = 120
# Application SpaceWarp (at 120 Hz): the headset synthesizes the refresh
# between two game frames from motion vectors the app gives it, so the
# world moving past (as the view follows Mario) looks sharp instead of
# doubled. 0 shows each game frame twice instead.
space_warp = 1
# Meta Quest Super Resolution: the headset scales the world from its render
# size to the display with an edge-aware filter and sharpening. 0 scales it
# in the app (bilinear).
super_resolution = 1
# AMD FidelityFX CAS (contrast adaptive sharpening) on the world, and its
# strength from 0 (least) to 1 (most).
sharpening = 0
sharpening_strength = 0.5
# Play on a giant 16:9 screen from the game's own camera (1, the default)
# or in the diorama (0), and how far away that screen is, in metres (it is
# 5.33 m wide: 61 degrees across at 4.5 m, 30 degrees at 10 m).
giant_screen = 1
screen_distance = 4.5
# The folder of the game's files, as chosen on the setup screen (the app's
# own files/game when there is no such line).
# game_path = /storage/emulated/0/Download/cooked
# 1 turns the diorama with the game camera as it swings round Mario (as
# early versions did); 0 keeps the world's facing, and the right stick
# turns it in 45 degree steps.
turn_with_camera = 0
# Ask the headset for high CPU and GPU clock levels (Meta's SustainedHigh,
# the CPU at 1.92 GHz on Quest 3). 0 leaves the clocks to the system, which
# runs the CPU slower to save battery, and more frames come late.
high_clocks = 1
```

To copy it over: `adb push petari_vr.ini /sdcard/Android/data/com.galaxy.quest/files/`.

## Checking performance

After a session, `tools/app_log.sh` shows the app's log. Every 10 s of
gameplay it contains a line like

```
vr: render scale 1.10-1.25 (now 1.20, 3 changes); eye GPU 5.2 ms avg 6.4 max of 8.3; 2 refreshes missed
```

with the range of render resolutions used, and a line
`vr: the frame loop missed N refreshes (M with its own work late)` whenever
refreshes were missed. Only the misses the frame loop was not late for (the
GPU work overran the refresh) lower the resolution: when the loop's own CPU
work runs late, a lower resolution would not help.

With SpaceWarp on, a line `vr: SpaceWarp: N frames with motion vectors in
the last 10 s, M not to extrapolate` follows every 10 s (about 600 frames,
the game's 60 a second; the frames not to extrapolate are cuts, such as the
fade between the diorama and the virtual screen), and
`adb logcat -s VrApi` shows `FPS=60/120` and `ASW=120, Type=App` (the frames a second shown, the synthesized ones included).
