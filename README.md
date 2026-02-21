# C-GB

An instruction-accurate Game Boy (DMG) emulator written in C, focused on high levels of accuracy.

## Features

- **Cycle-accurate CPU** - Passes all Blargg CPU instruction and timing tests
- **Accurate PPU** - Passes dmg-acid2; proper sprite priority, window behavior, and STAT timing
- **6 color palettes** - Grayscale, DMG green, Pocket, Sepia, Light Blue, Virtual Boy (F2 to cycle)
- **Turbo mode** - Press F3 to toggle fast-forward
- **Fullscreen support** - Toggle with F11
- **Drag-and-drop** - Load ROMs by dragging onto the window
- **Cross-platform** - Runs on both Windows and Linux
- **Compatible with all tested MBC0 (ROM only) Game Boy games**


***NOTE:***
MBCs are not yet implemented, so only 32KB ROMs are supported at the moment. See [COMPATIBILITY.md](COMPATIBILITY.md) for a list of tested games.

## Controls

The default keyboard bindings are:

|Button|Keybind    |
|:----:|:---------:|
|A     |X          |
|B     |Z          |
|Select|Right Shift|
|Start |Return     |
|D-Pad |Arrow Keys |

Additional function keys:

|Key   |Function    |
|:----:|:----------:|
|F1    |Open config menu|
|F2    |Reset       |
|F3    |Palette cycle|
|F4    |Toggle turbo mode|
|F5    |Toggle pause|
|F11   |Toggle fullscreen|

The keybinds may be changed in the config menu (F1 by default).
By default, keybind configurations are stored in `keybinds.cfg`.

## Compiling & Running

### Windows

Download the latest release from the "Releases" section and unzip the file's contents.

Double-click `C-GB.exe` to run the emulator, and drag a ROM file into the window to run it.

### Linux

Building was tested and verified on Linux Mint 22.3 Zena, but should work on any reasonably modern Linux distribution with the necessary dependencies installed.

Dependencies:
- make
- gcc
- SDL2 development libraries (libsdl2-2.0-0 and libsdl2-dev)
- SDL2 TTF development library (libsdl2-ttf-dev)

To compile the source code, navigate to the root and run:  
`make`

To run C-GB with a specific ROM, run:  
`./bin/C-GB path/to/rom.gb`

Alternatively, you can run C-GB without arguments:  
`./bin/C-GB`

This will open a blank window where you can drag and drop a .gb ROM file to load it. You can also drag and drop a new ROM at any time to reset and load it.

Alternatively, the Windows executable in the "Releases" tab can be run safely with Wine.

## Test ROM results

Test ROM results can be found in the main directory's test-results folder.

## Version History

### V 0.40

- Added keybind configuration menu

- Fixed a bug where slow ROM reads could cause temporary timer desynchronization and speed up gameplay

### V 0.35

- Implemented full STAT line handling, fixing certain games being unplayable and minor screen effects in several others

- Fixed mid-instruction timing to pass blargg's read_timing, write_timing, and modify_timing tests

- Fixed EI delay to pass mooneye's rapid_di_ei test

- Moved test ROM results screenshots to their own folder, test-results.

- Total number of fully compatible games: 65 (All MBC0 games are now fully functional!)

### V 0.30

- Added Windows support
  
- Added drag-and-drop ROM support

- Added three new colour palettes (Pocket Green, Sepia, and Light Blue)

- Added fullscreen hotkey

- Improved function comment headers to make purpose more clear

- Improved performance by inlining opcode helper functions and using ternary operators for specific operations

- Improved code readability and fixed descriptions for several functions

- Fixed erroneously attempting to initialize SDL twice during initialization

- Added more comprehesive error handling

### V 0.26

- Greatly improved code readability by creating standard with clang-format

- Improved performance by reducing per frame overhead in main loop

- Added pause hotkey

- Moved input and toggles to central GB struct for improved modularity

### V 0.25

- Improved code cohesiveness by adding central GB struct and reducing cross-module dependency

- Improved error handling by adding status to certain functions

- Added turbo mode hotkey

- Added all-red colour palette option inspired by the Virtual Boy

### V 0.24

- Improved sprite rendering performance by saving OAM sprite data in structs

- Fixed sprite rendering and window logic to pass dmg-acid2 test

- Total number of tested working ROMs: 54

### V 0.23

- Added serial interrupts, fixing certain games not booting properly

- Fixed CPU mid-instruction order to pass IME timing tests
- Fixed certain RST instructions mistakenly returning M-cycles rather than T-cycles

- Total number of tested working ROMs: 43

### V 0.22

- Added compatibility list at COMPATIBILITY.md
- Added STAT interrupts, fixing broken/corrupted graphics in certain games

- Improved opcode execution and flag read/writes to be more performant, resulting in a ~10% raw performance boost
- Improved README structuring

- Fixed PPU mode changes not being written to STAT, removing several freezes and fixing broken/corrupted graphics in certain games

- Total number of tested working ROMs: 28

### V 0.21

- Fixed 8x16 tile sprite rendering

- Total number of tested working ROMs: 14

### V 0.20

- Added sprite rendering
- Added input
- Added reset and palette swap hotkeys
- Added FPS counter
- Added Controls section to README

- Improved FPS accuracy (~62 FPS -> ~59.7 FPS)
- Improved tile rendering performance

- Fixed timer behaviour to pass blargg CPU test #2
- Fixed several minor formatting issues
- Fixed Makefile to update if a header file is changed

### V 0.11

- Fixed interrupt cancelling behaviour to pass mooneye's ie_push test
- Fixed IF read/write behaviour to pass mooneye's if_ie_registers test

- The following mooneye tests were also performed and passed: mem_oam, reg_f, daa, basic, boot_regs-dmgABC

### V 0.1

Initial version

- Added background and window tile rendering

- Passes all blargg CPU test ROMs, with the exception of #2

## Disclaimer

This program uses SDL2, which is licensed under the zlib license, which can be found 
[here](https://www.libsdl.org/license.php).

This project is ***not*** affiliated with Nintendo or any other copyright holders.  
It is intended for educational purposes, software development practice, and use with legally acquired ROMs.  
The author does ***not*** condone or support piracy in any way, shape, or form.

## License

This project is licensed under the MIT License.
