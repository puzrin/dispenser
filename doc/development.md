Development notes<!-- omit in toc -->
=================

- [PC Simulator](#pc-simulator)
- [Messaging via USB Serial](#messaging-via-usb-serial)
- [Bare metal debug](#bare-metal-debug)


## PC Simulator

Firmware has simulator option for quick GUI development on desktop. It requires
SDL2 and was tested with Ubuntu 18.04LTS.

For dependencies install:

```sh
sudo apt-get install libsdl2-dev
```

To build & execute, via PlatformIO menu: `Terminal` -> `Run Build Task` ->
`PlatformIO: Execute (emulator)`.


## Messaging via USB Serial

Dispenser has built-in USB CDC descriptor with `stdout` retargeting. All
`printf()` output will be sent to virtual COM-port. By default, it sends
 LVGL memory pool usage info.

In linux, to see `tty` name of attached device type:

```sh
dmesg | grep tty
```

Then, to see messages, use:

```sh
screen /dev/<tty_name_from_dmesg>
```

For example: `screen /dev/ttyACM0`.


## Bare metal debug

In some cases you may wish to debug bare metal. PCB has SWD connector pins,
where to can attach ST-Link/V2. See PlatformIO docs for more details about
debugger.

We do not promote SWD connector in other documents, because for ordinary use USB
is enough and more simple.
