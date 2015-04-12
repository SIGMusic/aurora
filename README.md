tonal-lightfield
==========

Lights for the SIGMusic EOH 2015 project

See [the wiki](../../wiki) for info about the hardware, protocol, and for
instructions on building the lights.

Setup
-----

1. Download the project source code (e.g. fork, clone, etc.)
    * If you don't have Git set up, just
      [download the ZIP](../../archive/master.zip)
2. Install the [Arduino IDE](http://arduino.cc/en/Main/Software)
3. Follow the steps on the wiki to [build the hardware](../../wiki/Building-Instructions)
4. Install [Python 3.3](http://python.org/downloads/) (32-bit only is supported),
   the [PyBluez](https://code.google.com/p/pybluez/downloads/list) version corresponding to your installed version of Python,
   and [Tornado](http://tornado.readthedocs.org/en/latest/index.html#installation).
    * Note that PyBluez only supports Windows and Linux.
5. Plug in all the lights to mains power.
6. Turn on your computer's Bluetooth and ensure each light has been paired before. The default passcode is `1234`.
7. Run `python manager.py discover` from the manager directory.
8. In the future, simply run `python manager.py` to use the previously discovered lights.
