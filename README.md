Heat Map Creator
================

A very simple program to create heat maps from timestamped point data. Mostly used for personal project to visualize eye tracking data.

Building and Usage
==================

####Dependencies

- [OpenCV](http://opencv.org/) 2.4+

####Building

To build run `make` which should produce an executable `heatmap`.

####Usage

heatmap [OPTIONS] video point_data

OPTIONS:

-am, --alpha-max ARG             Max alpha value for the overlay. [0-1]

-ec, --end-color ARG1[,ARGn]     Ending heat map color in HSV. [0-255]

-fc, --four-cc ARG               Specify the FOURCC code for the output video.
                                 Must be exactly four characters. Use quotes for
                                 whitespace.

-ft, --fade-time ARG             Fade time in seconds. Supports floating
                                 values.Must be greater than zero.

-in, --intensity ARG             Base intensity, higher values saturate faster.
                                 [0-1]

-ks, --kernel-size ARG           Kernel size in pixels.

-l, --linear                     Use a linear kernel instead of gaussian.

-nv, --no-video                  Don't show the video window. Useful for
                                 creating an output video.

-nw, --no-wait                   Play the video back as fast as possible.

-ov, --out-video ARG             Write heat map video to a file.

-pp, --print-progress            Print percent progress while playing. Updates
                                 in one percent increments.

-sc, --start-color ARG1[,ARGn]   Starting heat map color in HSV. [0-255]

Visuals
=======

Sample video showing an overlay of the [Big Buck Bunny](http://www.bigbuckbunny.org/index.php) video.

[Video Link](https://vimeo.com/86652898)
