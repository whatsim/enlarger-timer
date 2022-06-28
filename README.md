# enlarger timer

This is an arduino sketch (and rough description of the build) for making an photo enlarger controller. In order to make accurate and repeatable prints, enlargers are often pared with timers (like the [one I learned on](https://www.freestylephoto.biz/300718-GraLab-300-Darkroom-Timer-(GR300))). I came into an enlarger that didn't come with a timer, and rather than sourcing one decided to build my own.

The key features are:

* ability to set exposure in 0.25 second increments
* red led display of time (I've found this to be b/w paper safe)
* ability to toggle enlarger light for framing and focusing
* ability to disable display, if needed
* ability to run the exposure (with a slight delay)
* a one minute timer (without running the exposure) useful for timing chemical baths

While this design relies on the internal clock of the arduino for timing and is therefore not perfectly precise, I've used this in my darkroom since late 2020 and have found it to be reliable.

## components:

* [Adafruit 0.56" 4-Digit 7-Segment Display w/I2C Backpack - Red](https://www.adafruit.com/product/878)
* [Rotary Encoder](https://www.adafruit.com/product/377)
* [Colorful 12mm Square Tactile Button Switch Assortment](https://www.adafruit.com/product/1010)
* [Adafruit Metro Mini 328 - Arduino-Compatible - 5V 16MHz](https://www.adafruit.com/product/2590?main_page=product_info&products_id=2590&gclid=CjwKCAjwquWVBhBrEiwAt1KmwoS7WpHzJAsz5GnjmAvNrvBubl0hzlUHzjdlElit6GfaIhWHiBaLVhoCV-0QAvD_BwE)
* [Controllable Four Outlet Power Relay](https://www.adafruit.com/product/2935)
* [RCA Female Jack with wire terminal](https://www.amazon.com/gp/product/B07VPZWK2J/ref=ppx_yo_dt_b_asin_title_o06_s00?ie=UTF8&psc=1)
* [protoboard with standoffs](https://www.amazon.com/gp/product/B081YGHS73/ref=ppx_yo_dt_b_asin_title_o08_s00?ie=UTF8&psc=1)

## Dependencies

once you have the sketch open in the [Arduino IDE](https://www.arduino.cc/) you need to install MD_REncoder via `Tools > Manage Library` in the Arduino menu. You can search for 'MD_REncoder' and select install. I am running version 1.0.1. The sketch also requires Wire which should come with the Arduino IDE.

## pinout

The code is somewhat self documenting in this regard if you're familiar with wiring for simple microcontrollers. You probably will need a soldering iron to build this out, though if you sourced a version of the Display backback and Metro Mini with headers already soldered you could probably get by building this on a breadboard, though that won't be terribly rugged.

```
pinout

ROTARY_PIN1 2
ROTARY_PIN2 3

RUN_BUTTON_PIN 6
TIMER_BUTTON_PIN 5
PREVIEW_BUTTON_PIN 4
POWER_PIN 8

I2C_DATA A4
I2C_CLOCK A5
```

The rotary encoder needs to be connected to ground on the Mini Metro (on mine ground was the middle pin of the encoder) and then the remaining two pins need to be connected to pins 2 and 3 of the Metro Mini.

Similarly, the buttons need to be attached to ground on the Mini Metro, and to their respective button pins. The run button which activates the enlarger and runs the timer is attached to pin 6 of the Mini Metro. The 1 minute timer button which does not activate the enlarger is on pin 5, the preview button which activate the enlarger light without activating the timer is on pin 4.

In my build, the preview button uses the push button capability of my rotary encoder (the 2 pin side of the encoder linked above), but there's no reason it couldn't be another button.

The power pin, which is used to communicate with the relay, uses pin 8, and ground.

The display is attached to the I2C pins of the microcontroller, on the Mini Metro, I2C Data is A4 and I2C Clock is A5. The display also needs to be connected to +5V and to ground on the Mini Metro.

I used an RCA jack in order to connect to the Relay Plug (in order to have it be easy to disconnect, and also to allow the Relay Plug to be some distance away from the rest of the controller on the ground). You could directly hook up to the relay from Pin 8 and ground if you rather.