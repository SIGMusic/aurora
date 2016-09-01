aurora
======

*Wirelessly networked RGB floodlights*

Aurora is a SIGMusic project to build a set of colorful floodlights that can
be used modularly for various projects. These lights are wirelessly controlled
by a Raspberry Pi that accepts clients over the Internet, which allows anyone
to write and run interactive lighting visualizations.

See [the wiki](../../wiki) for info about the hardware, protocol, and for
instructions on building the lights.

Repository layout
-----------------

* The `examples` folder contains sample client applications
* The `firmware` folder contains the Arduino sketch that runs on the lights
* The `hardware` folder contains the Eagle files for the base station shield
  and the logic board in each of the lights
* The `software` folder contains the Python scripts that run on the
  Raspberry Pi and manage the socket servers, scheduler, and radio network

Setup
-----

TODO