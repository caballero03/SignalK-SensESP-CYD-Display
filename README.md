SignalK-SensESP-CYD-Display
===========================

# Intro
This is my attempt to use a Cheap Yellow Display (ESP32-2432S028R or CYD for short) and SensESP ^3.0 for a Signal K server remote touchscreen display. 

# Hardware
The CYD board is a low-cost ESP32-based color LCD display with a pixel resolution of 320x240 and a resistive touchscreen. Here I use the display to show various SK data points at appropriate intervals. 

The CYD board can be had for around $12-$14 USD and has a bunch of great features for the price.

Read more about the CYD board here: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

# Basics of this design (so far)
Instead of fussing with pixel precision, I chose to divide the 320x240 pixel screen area into a 16x16 grid of "cells". Postioning display elements is done by specifying which cell the text or graphical element should start at. This will likely change in the future as this project matures. For now, this is how it works.

Here is an example of several text elements and one VBAR graphical element being positioned on the grid system.
![Grid system for positioning data on the display](src/displays/images/CYD_Display_Scaffold.svg?raw=false)

Here is what it looks like currently with some recently added fields from another SensESP project that I will be sharing sometime soonish.

![photo of the Cheap Yellow Display displaying data from an SK server](src/displays/images/photo_of_CYD_SK_Display.JPG?raw=false)


# Development Environment
Here I will describe the basics of how I set up my development environment. I use a laptop with Linux, docker and docker-compose installed. This way I can run a temporary Signal K server for development away from my boats' network and server. I didn't want to have to only work on this project while on my boat. I have much more freedom now to work on this project from anywhere I can drag my laptop to.

## Temporary Signal K server for development off the boat. 

### Using Docker
I first tried to pull the signalk-server docker image and run it locally, but there was no easy success so I tried another way. I just didn't have the patience to figure it out. Maybe I'm a little lazy too. :-P

Instead, what I did was clone the Signal K server git hub repo and used the docker-compose.yml file to start a container.
1) ```git clone https://github.com/SignalK/signalk-server```
2) ```cd signalk-server/docker```
3) ```vi docker-compose.yml``` # and edit lines 23-25 to remove the comments. This will allow you to edit your local copy of startup.sh in the next step so we can add some sample boat data for development.
4) ```vi startup.sh``` # Edit the last line by adding ``` --sample-nmea0183-data``` at the end. This gives us a repeating boat data stream to use for testing.
5) ```docker-compose up -d```

Once the docker container is running, point your browser to ```localhost:3000``` to get to the demo/temporary Signal K server.

After you get the Signal K server container up and running, you will need to setup an admin users' credentials. Since I didn't bother to make a volume to store the servers' data on the docker host, this process would need to be repeated if the container is stopped and removed.

Now, you should be able to launch Freeboard-SK from the Signal K server's Webapps menu and see the happy little boat underway in Finland somewhere. This is the boat data that will replay over and over to give some nice sample data to read and put on the display. Also, the Data Browser will show the sample data in it.

