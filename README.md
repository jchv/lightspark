Lightspark
==========

![GitHub release](https://img.shields.io/github/release/lightspark/lightspark.svg)
![GitHub Last Commit](https://img.shields.io/github/last-commit/lightspark/lightspark.svg)
[![Travis Status](https://img.shields.io/travis/com/lightspark/lightspark/master.svg?label=master%20branch)](https://travis-ci.com/lightspark/lightspark)

Lightspark is an open source Flash player implementation for playing
files in SWF format. Lightspark can run as a web browser plugin or as
a standalone application.

Lightspark supports SWF files written on all versions of the
ActionScript language.

Building and Installation
-------------------------

To compile this software you need to install development packages for:
* opengl
* curl
* zlib
* libavcodec
* libavresample
* libglew
* pcre
* librtmp
* cairo
* libboost-filesystem,
* sdl2
* sdl2_mixer
* libjpeg
* libavformat
* pango
* liblzma

If JIT compilation using llvm is enabled (this is disabled by default),
you also need the development packages for llvm (version 2.8 or >= 3.0)

If compiling the PPAPI (Chromium) plugin is enabled (on by default), keep in mind that
it will replace the Adobe Flash plugin, as only one flash plugin is allowed in Chromium.

Also install the following tools:
* cmake
* nasm
* gcc (version 4.6.0 or newer) or clang

To build the software please follow these steps.

```bash
cd lightspark
mkdir obj
cd obj
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

DEBUG MODE:
To enable debug mode change the cmake command like this:

``cmake -DCMAKE_BUILD_TYPE=Debug``

The ``CMAKE_BUILD_TYPE`` options are: Debug LeanDebug Release RelWithDebInfo Profile

Execution
---------

Using `make install`, lightspark is installed in the system wide

### Browser plugin

Firefox plugin path and Firefox should show it in the about:plugins
list and in the Tools -> Add-ons -> Plugins window.

Lightspark registers itself as the plugin for
application/x-shockwave-flash and for application/x-lightspark, so it
should be recognisable in the about:plugins page. Its description
string is ``Shockwave Flash 12.1 r<current version>``. The current
version is now "r710".

Firefox is not able to deal very well with multiple plugins for the
same MIME type. If you only see a black box where a flash app should
be try to remove any other flash plugin you have installed.

### Command line

The command line version of Lightspark can play a local SWF file.
Execution: ``lightspark file.swf``

Type `lightspark` to see all command line options.

### Keyboard shortcuts

Ctrl+Q Quit (standalone player only)
Ctrl+M Mute/unmute sounds
Ctrl+P Show profiling data
Ctrl+C Copy an error to the clipboard (when Lightspark fails)

### Environment variables

* ``LIGHTSPARK_USE_GNASH``: if set to 1, lightspark will fall back to gnash for older swf files
* ``LIGHTSPARK_PLUGIN_LOGLEVEL``: sets the log level (0-4) (browser plugins only)
* ``LIGHTSPARK_PLUGIN_LOGFILE``: sets the file the log will be written to (browser plugins only)
* ``LIGHTSPARK_PLUGIN_PARAMFILE``: if set, the flash variables set by the website will be written to this file (browser plugins only)

SWF Support
-----------

Many web sites do not yet work yet because the implementation is
incomplete. But we do support a number of them. See our [site compatibility page]
for more details.

[site compatibility page]: https://github.com/lightspark/lightspark/wiki/Site-Support

Reporting Bugs
--------------

If you think you have found a bug in Lightspark, please file a bug.
See our [bug reporting help] for details.

[bug reporting help]: https://github.com/lightspark/lightspark/wiki/Reporting-Bugs
