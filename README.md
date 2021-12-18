# touchpad-gesture-drag

[Wayfire] plugin for 3-finger-drag (or any N finger drag).

Limitations:

- currently requires [THIS WAYFIRE PR](https://github.com/WayfireWM/wayfire/pull/1388)
- does not accelerate the pointer movement :( [no libinput API for manually applying accel yet](https://gitlab.freedesktop.org/libinput/libinput/-/issues/715)
  - as a workaround deltas are just multiplied by 100 to make them not unusably slow
- cursor won't move in nested (windowed wayland backend) mode :D

[Wayfire]: https://github.com/WayfireWM/wayfire

## Usage

Install and add to the list. There is no configuration for now.

## License

This is free and unencumbered software released into the public domain.  
For more information, please refer to the `UNLICENSE` file or [unlicense.org](https://unlicense.org).
