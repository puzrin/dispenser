User Manual <!-- omit in toc -->
===========

- [Control](#control)
- [Settings](#settings)
- [Viscosity](#viscosity)
- [Syringe refill](#syringe-refill)

Dispenser works in 2 modes - dose and flow. In dose, it dispense fixed portions
on each press. In flow mode, it continue dispense until button pressed.

Dose volume and dispense speed are selectable.


## Control

Joystick:

- Short push - mode switch (portion/flow).
- Long push - goto settings.
- Up/Down - move selection.
- Left/Right - change value (settings only).

You can edit `src/doses.cpp` file to change list of selectable portions.


## Settings

This params are used to estimate optimal dispense speed and portion volumes.

- **Syringe dia** - inner diameter of syringe. Required to properly calculate
  portion sizes. If you plan to dispense very small doses (0603 and below), we
  recommend to use smallest possible syringe (1cc).
- **Viscosity** - defines dispense speed and retract size. Typical values are
  described in next chapter.
- **Flux volume** - % of flux in paste. Needed to calculate proper volume of
  solder. If you need to dispence glues or fluxes - set to zero.
- **Fast move** - quick move pusher back and forward for syringe refill.


## Viscosity

Viscosity depend very much on temperature. Table below contain only average
value for room env.

_cP = centipoise = 0.1 Pa·s (pascal-second) = mPa·s_

Substance | Viscosity (cP)
----------|---------------
Soldering paste for SMT stencil | 650K-1200K
Soldering paste for dispenser | 300K-450K
Glycerol | 1400
Water | 1


## Syringe refill

Everything is done without disassembly. You will need only additional syringe
and luer coupler.

Notes:

- It's important to fill syringe without air bubbles.
- Avoid buy paste in syringes, because it can have air bubbles with very high
  probability. Prefer flasks.

Steps:

1. Fill helper (charger) syringe with paste. You can do that in different ways:
   - As on this video.
2. Move dispenser's pusher back via settings menu.
3. Attach needle to charger and fill dispenser's nozzle to leave no air.
4. Change needle to luer coupler and make sure i't filled with paste.
   Fill coupler before attach
5. Attach another end of coupler to dispenser and fill it.
6. Attach needle with protective cap to dispenser.

Tips:

- You can heat paste to 40-50C in hot water, to make it more liquid. But don't
  heat it too much and too long, because it can stratify.
- You can experiment with adding some flux to make old paste more liquid. But
  consider just buy paste in smaller packaging and use new one :). Paste is
  cheap, don't search difficulties to save couple of bucks.
