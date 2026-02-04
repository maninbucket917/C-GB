# C-GB

A Gameboy emulator project written in C, focused on high levels of accuracy.

***NOTE:***
MBCs are not yet implemented, so only 32KB ROMs are supported at the moment.

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
|F1    |Reset       |
|F2    |Palette swap|
|F3    |Turbo mode  |
|F4    |Toggle pause|
|F11   |Toggle fullscreen|

The keybinds and the default palettes may be changed in `/include/config.h`.

## Compiling & Running

### Linux

Dependencies:
- make
- gcc
- SDL2 development libraries (libsdl2-2.0-0 and libsdl2-dev)

To compile the source code, navigate to the root and run:  
`make`

To run C-GB with a specific ROM, run:  
`./bin/C-GB path/to/rom.gb`

Alternatively, you can run C-GB without arguments:  
`./bin/C-GB`

This will open a blank window where you can drag and drop a .gb ROM file to load it. You can also drag and drop a new ROM at any time to reset and load it.

## Test ROM results

dmg-acid2: Passed

<img width="482" height="433" alt="image" src="https://github.com/user-attachments/assets/175c0aed-4b37-400f-bcd1-cbe838598c96" />

blargg CPU tests: Passed

<img width="481" height="431" alt="image" src="https://github.com/user-attachments/assets/6b258c3b-bc70-47be-a0f4-de0cd8a0db5c" />
<img width="479" height="429" alt="image" src="https://github.com/user-attachments/assets/a04dfc14-5922-4557-94f5-525e1e3acc5d" />
<img width="479" height="429" alt="image" src="https://github.com/user-attachments/assets/96b43d67-a0ef-4d39-8cbd-9687c05df9df" />
<img width="479" height="431" alt="image" src="https://github.com/user-attachments/assets/6c96f3f5-06d7-448a-bb50-fd48204ecb7b" />
<img width="479" height="432" alt="image" src="https://github.com/user-attachments/assets/4aff8ed2-7bd8-4b33-afc6-fb02c95eda4c" />
<img width="482" height="434" alt="image" src="https://github.com/user-attachments/assets/db11c22d-6722-4a07-aba1-45b123203751" />
<img width="481" height="432" alt="image" src="https://github.com/user-attachments/assets/7e205dcd-b61d-46d9-aff3-f71cf38c83c5" />
<img width="478" height="430" alt="image" src="https://github.com/user-attachments/assets/579c66f2-1d3b-49ad-b735-da28236461b7" />
<img width="481" height="431" alt="image" src="https://github.com/user-attachments/assets/e8cb5f1c-a4f1-4940-9a20-1bb73cbd45f9" />
<img width="479" height="433" alt="image" src="https://github.com/user-attachments/assets/ebe72cb1-943c-468c-ae6b-97b00d6bc2a0" />
<img width="480" height="430" alt="image" src="https://github.com/user-attachments/assets/f88f5f22-54bc-4930-9a6b-c2d4f82762a0" />

## Version History

### V 0.30

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
