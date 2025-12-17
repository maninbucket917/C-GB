# C-GB

A basic Gameboy emulator project written in C.

***NOTE:***
Currently, no MBCs are implemented, so only 32KB ROMs will run.

## Controls

The default keyboard bindings are:

|Button|Keybind    |
|------|-----------|
|A     |X          |
|B     |Z          |
|Select|Right Shift|
|Start |Return     |
|D-Pad |Arrow Keys |

Additional function keys:

|Key   |Function    |
|------|------------|
|F1    |Reset       |
|F2    |Palette swap|

The controls and the default palette may be changed in `/include/config.h`.

## Compiling & Running

### Linux

Dependencies:
- make
- gcc
- SDL2 development libraries (libsdl2-dev)

To compile the source code, navigate to the root and run:  
`make`

To run C-GB with a specific ROM, run:  
`./bin/C-GB path/to/rom.gb`.

## Version History

### V 0.21

- Fixed 8x16 tile sprite rendering

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

- Renders background and window tiles
- Passes all blargg CPU test ROMs, with the exception of #2

## Disclaimer

This program uses SDL2, which is licensed under the zlib license, which can be found 
[here](https://www.libsdl.org/license.php).

This project is ***not*** affiliated with Nintendo or any other copyright holders.  
It is intended for educational purposes, software development practice, and use with legally acquired ROMs.  
The author does ***not*** condone or support piracy in any way, shape, or form.

## License

This project is licensed under the MIT License.