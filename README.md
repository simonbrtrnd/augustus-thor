> [!IMPORTANT]
> Just so you know, this whole fork was built with Claude, Anthropic's AI
> assistant, in Claude Code. I decided what to build and tried every change on
> my Thor.

# Caesar III Thor

Caesar III (1998) on the AYN Thor, using both screens. It's a fork of
[Augustus](https://github.com/Keriew/augustus), an open-source engine that runs
the original game from your own copy.

- The top screen shows only the city, over its full height.
- The bottom touchscreen holds the whole sidebar: date, treasury, population,
  ratings, minimap, build buttons and menus. Its buttons are redrawn in HD.
- Messages, building info and small popups open on the bottom screen, so they
  never cover the city.
- The Thor's controls are mapped: the left stick moves the cursor, the right
  stick moves the map and the triggers zoom.

Smaller improvements:

- The zoom steps through fixed levels. Held, a trigger zooms smoothly and
  pauses on 100% and 50%.
- The keyboard for naming a save opens on the bottom screen, over a dimmed
  panel.
- Tap a panel button again to close the window it opened on the top screen.
- Select switches to the original layout on the top screen alone, and back.

<p align="center">
  <img src="doc/thor/top-screen.png" width="640" alt="The city on the Thor's top screen">
  <br>
  <img src="doc/thor/bottom-screen.png" width="413" alt="The panel on the Thor's bottom screen">
</p>

## What you need

- An AYN Thor (tested on Android 13).
- The game files of Caesar III, patched to version 1.0.1.0. A copy from the
  CD, GOG or Steam works.

> [!NOTE]
> This repository and the APK contain no game files.

## Install

1. Download the APK from the
   [releases](https://github.com/simonbrtrnd/augustus-thor/releases) and open
   it on the Thor. The app is called Caesar III and installs alongside the
   official Augustus app.

2. Copy your Caesar III folder to the Thor, such as to the SD card. It holds
   files such as `C3.sg2` and `C3.555`.

3. Open Caesar III and pick that folder when the app asks for it. Saves are
   stored in the same folder.

## Controller

Buttons are named as printed on the Thor: B bottom, A right, Y left, X top.

| Button | Action |
| --- | --- |
| Left stick | Move the cursor |
| Right stick | Move the map, or scroll a window |
| B | Left click |
| A | Right click |
| Y | Pause |
| X | Hide or show the current overlay |
| L1, R1 | Rotate the map left, right |
| L2, R2 | Zoom out, zoom in; hold to zoom smoothly |
| D-pad up, down | Game speed up, down |
| D-pad left, right | Previous, next overlay |
| Select | Switch between both screens and the whole interface on the top screen |

## Touch

Top screen:

| Gesture | Action |
| --- | --- |
| Tap | Left click; with nothing to build, tap a building to see its info |
| Drag | Move the map |
| Pinch | Zoom |

Bottom screen:

| Gesture | Action |
| --- | --- |
| Tap a build button | Pick what to build; most buttons open a list |
| Tap Menu | Save, load, and the file, options and help menus |
| Tap the overlay plate | Choose an overlay, such as water or fire risk |
| Tap the advisors | Choose an advisor |
| Tap the treasury, population or ratings | Open that advisor; tap again to close it |
| Tap the empire map | Open it; tap again to close it |
| Tap the minimap | Move the view there |
| Drag in a long window | Scroll it |

> [!TIP]
> Android shows the keyboard on the screen you touched last. Saving from the
> bottom Menu opens it below. If it pops up on the top screen, tap the bottom
> screen to move it down.

## Options

Select switches between the two screens and the original layout, with the
whole interface on the top screen. The bottom screen then stays black. The
choice is saved, and the game's configuration has the same setting, Use the
second screen.

<p align="center">
  <img src="doc/thor/one-screen.png" width="640" alt="The whole interface on the Thor's top screen">
</p>

Show windows on the second screen, on by default, opens messages, building
info and small popups on the bottom screen. Turn it off in the configuration
to keep them on the top screen.

The city, the empire map, the advisors, saving, loading and settings always
stay on the top screen, where they have room.

## Permissions

The app asks for no Android permissions. It only reads and writes the game
folder you pick.

## Build the APK

With JDK 17, NDK 29.0.14206865 and CMake 4.1.2 from the Android SDK manager:

```sh
git clone -b thor --recursive https://github.com/simonbrtrnd/augustus-thor.git
cd augustus-thor
curl -L https://www.libsdl.org/release/SDL2-2.32.10.tar.gz | tar -xz -C ext/SDL2
curl -L https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.8.2.tar.gz | tar -xz -C ext/SDL2
cd android
./gradlew assembleDebug
```

The APK is created in `android/augustus/build/outputs/apk/debug/`. The project
path must not contain spaces, since `ndk-build` doesn't handle them.

## Feedback

Feedback is welcome, whether it's a bug, an idea or a question: open an
[issue](https://github.com/simonbrtrnd/augustus-thor/issues).

## Credits and license

- Augustus is made by Keriew and contributors, from Julius by Bianca van
  Schaik. Both use the AGPL 3.0 license, and so do this fork's changes. The
  Augustus [manual](https://github.com/Keriew/augustus/tree/master/res/manual)
  explains its gameplay changes.
- The HD panel buttons and the gray stone are redrawn from the game's art,
  partly with ChatGPT image generation.
- Caesar III was made by Impressions Games and published by Sierra. This
  project is not affiliated with or endorsed by the game's rights holders or
  AYN.
