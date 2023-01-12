# LinHPSDR

Fork of John Melton G0ORX's linhpsdr.

### Development environment

Development and testing has been run on Ubuntu 17.10 and Ubuntu 18.04. If run on early versions there may be a problem with GTK not supporting the gtk_menu_popup_at_pointer function vfo.c. For information on MacOS support see [MacOS.md](./MacOS.md).

### Prerequisites for building

```
  sudo apt-get install libfftw3-dev
  sudo apt-get install libpulse-dev
  sudo apt-get install libsoundio-dev
  sudo apt-get install libasound2-dev
  sudo apt-get install libgtk-3-dev
  sudo apt-get install libsoapysdr-dev
```

### Prerequisites for installing the Debian Package

```
  sudo apt-get install libfftw3-3
  sudo apt-get install libpulse
  sudo apt-get install libsoundio
  sudo apt-get install libasound2
  sudo apt-get install libsoapysdr
```


### linhpsdr requires WDSP to be built and installed

```
  git clone https://github.com/g0orx/wdsp.git
  cd wdsp
  make
  sudo make install
```
### CW support

Hermes and HL2 CWX/cwdaemon support added. If you do not wish to use this, please ignore. This features requires the following to be installed (tested on Ubuntu 19.10, Kubuntu 18.04 LTS):

```
  sudo apt install libtool
  git clone https://github.com/m5evt/unixcw-3.5.1.git
  cd unixcw-3.5.1
  autoreconf -i
  ./configure
  make
  sudo make install
  sudo ldconfig
```
If CWX/cwdaemon is wanted/required. You must enable it in the Makefile. Uncomment the following lines:
```
#CWDAEMON_INCLUDE=CWDAEMON

#ifeq ($(CWDAEMON_INCLUDE),CWDAEMON)
#CWDAEMON_OPTIONS=-D CWDAEMON
#CWDAEMON_LIBS=-lcw
#CWDAEMON_SOURCES= \
#cwdaemon.c
#CWDAEMON_HEADERS= \
#cwdaemon.h
#CWDAEMON_OBJS= \
#cwdaemon.o
#endif
```

### To download, compile and install linHPSDR from here

```
  git clone https://github.com/g0orx/linhpsdr.git
  cd linhpsdr
  make
  sudo make install
```

# LinHPSDR MacOS Support
  
### Development environment

Development and testing has been run on MacOS Sierra 10.12.6 and MacOS high Sierra 10.13.6. Prerequisites are installed using [Homebrew](https://brew.sh/).

### Prerequisites for building

```
  brew install fftw
  brew install gtk+3
  brew install gnome-icon-theme
  brew install libsoundio
  brew install libffi
  brew install soapysdr
```

### linhpsdr requires WDSP to be built and installed

```
  git clone https://github.com/g0orx/wdsp.git
  cd wdsp
  make -f Makefile.mac install
```

### To download, compile and install linHPSDR from https://github.com/g0orx/linhpsdr

```
  git clone https://github.com/g0orx/linhpsdr.git
  cd linhpsdr
  make -f Makefile.mac install
```

The build installs linHPSDR into `/usr/local/bin`. To run it, type `linhpsdr` on the command line.


# LinHPSDR Windows Support
  
### Development environment

  Development and testing has been run on Windows 10 and Windows 11.
  Tools needed for assembly:</br>
    [MSYS2](https://www.msys2.org/)</br>
    [GTK+ for Windows Runtime Environment](https://github.com/tschoonj/GTK-for-Windows-Runtime-Environment-Installer)

### Prerequisites for building (MSYS2 MINGW64)

```
  pacman -S mingw-w64-x86_64-gtk3
  pacman -S mingw-w64-x86_64-toolchain
  pacman -S make
  pacman -S git
  pacman -S mingw-w64-x86_64-pulseaudio
  pacman -S mingw-w64-x86_64-portaudio
```

### linhpsdr requires WDSP to be built and installed

  - From https://github.com/g0orx/wdsp take "wdsp.h" and copy to C:/msys64/mingw64/include/wdsp/wdsp.h
  - From [Thetis Release](https://github.com/TAPR/OpenHPSDR-Thetis) take "wdsp.dll" and copy to C:/msys64/mingw64/lib/wdsp/wdsp.dll

### linhpsdr requires installed libsoundio

  - Download Windows binary from [link](http://libsound.io/release/libsoundio-1.1.0.zip)
  - Unzip the archive and copy "soundio" headers files to C:/msys64/mingw64/include/soundio/
  - Copy "libsoundio.a", "libsoundio.dll" and "libsoundio.dll.a" to C:/msys64/mingw64/lib

### To download and compile linHPSDR from https://github.com/ew8bak/linhpsdr (MSYS2 MINGW64 console)

```
  git clone https://github.com/ew8bak/linhpsdr
  cd linhpsdr
  make
```
  After successful compilation, the linhpsdr.exe file will be located in the "bin" directory

### Troubleshooting

  - Library not found message when starting program:<br>
      copy to "bin" directory wdsp.dll, libsoundio.dll and libfftw3-3.dll
  - The program starts, but there is no panadapter and waterfall:<br>
      add "linhpsdr.exe" to Windows Firewall exceptions. Allow incoming and outgoing UDP/TCP packets in Firewall