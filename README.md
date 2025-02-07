#Pressure regulating system on STM32L073RZT6 microcontroller

#Warning
The project involves working with high voltages (230 V) that are dangerous for
life, make sure you know what you're doing.

###The system consist of
* STM32L073RZT6 microcontroller board
* KAMAMI KA Nucleo Multisensor shield
* Switch
* Analog manometer
* Voltage divider
* Solid state relay (SSR)
* Air compressor
* Air tank


###simplified diagram is show below:

![Simplified diagram](
https://github.com/Torchikaii/Pressure-regulation/simplified_diagram.png)

###How it works ?
The controller is trying to maintain pressure inside an air tank, if the
pressure becomes lower then set, compressor is turned on, else compressor
stays off. You can start/stop (enter or exit edit mode)
the system by pressing OK button/switch.
You can adjust the desired pressure by pressing two buttons on KAMAMI shield,
one button decreases it and the other increases.
By default system will start in edit mode, with 5.0 bar starting pressure. This
value can be adjusted in Pressure-regulation/Core/Src/main.c `targetPressure`
global variable.

###Additional information
You may want to use relay that has very low input current (test showed that
7.5 mA was too high) or additionally use a transistor with additional power
source to increase control signal going from microcontroller to relay (not shown
in the diagram).

You may also want to calibrate your pressure sensor (analog manometer). For that
you can adjust `pressure` variable located in
Pressure-regulation/Core/Src/main.c  `ReadPressure(void)` function.
