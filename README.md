## QtSonyCam

A small test program for working with some old Sony [XCD-SX90][xcd-sx90] and [XCD-SX90CR][xcd-sx90cr] *FireWire* cameras using the *ZCL SDK for Windows 7*.

You can download the **ZCL SDK** from either of those camera link pages under the *Software* section. A login is required.

I tested using this version : *Sony_ZCL_SDK_v2-12.2_Win7_32bit.zip*

The project is for *Visual Studio 2010* using the Qt *Visual Studio Add-In*.

I tested with Qt *5.3.2* and *5.4*, but it should work with any *5.x* version.

OpenCV *2.4.10* is also required, more for compatibility with the end application then for this app.

External trigger mode requires input from another device. I used an Arduino.

Frame rates are limited to 15 fps when internally triggered, ~23 fps externally triggered.

I was only interested in 1280x960 resolutions.

The color images from the camera are Bayer.

[xcd-sx90]: http://www.image-sensing-solutions.eu/xcd_sx90.html
[xcd-sx90cr]: http://www.image-sensing-solutions.eu/xcd_sx90cr.html
