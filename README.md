# C-GB

A basic Gameboy emulator project written in C.

## Compiling & Running

### Linux

Dependencies:
- make
- gcc
- SDL2 development libraries (libsdl2-dev)

To compile the source code, navigate to the root and run:  
```make```

To run C-GB with a specific ROM, run:  
```./bin/C-GB path/to/rom.gb```

## Version History

### V 0.11

- Fixed interrupt cancelling behaviour to pass mooneye's ie_push test
- Fixed IF read/write behaviour to pass mooneye's if_ie_registers test
- The following mooneye tests were also performed and passed: mem_oam, reg_f, daa, basic, boot_regs-dmgABC, 

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