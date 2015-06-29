# tinySynth
A synthesiser running on a STM32F4Discovery. This is my final year BEng project.

To run this guy:

- Get yourself an STM32F4Discovery
- Install ST-Link and the GNU ARM Toolchain `brew install st-link arm-none-eabi-gcc`
- Build [this](http://www.stephenhobley.com/blog/2011/03/14/the-last-darned-midi-interface-ill-ever-build/) MIDI interface and connect it to PA3
- Run `make` in the root directory of the project
- Connect the STM32F4Discovery to your computer
- Open a Terminal window and run `st-util`
- Open another Terminal and run `arm-none-eabi-gdb`
- In GDB run `tar ext:4242`, then `file main.elf`, `load` and  `run`
- Hurray! 🎉 The synth is now running on your board. Now connect a MIDI keyboard to it and have fun! 🎹

In this project I used a STM32F4Discovery template project from https://github.com/jeremyherbert/stm32-templates
