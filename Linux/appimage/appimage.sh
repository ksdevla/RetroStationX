#!/bin/bash

export PUBLISHFLAGS="-r linux-x64 -p:PublishSingleFile=true -p:PublishReadyToRun=true"
make -j$(nproc) -O USE_AOT=true LTO=true STATICLINK=true SYSTEM_LIBEVDEV=false

curl -SL https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -o appimagetool

mkdir -p AppDir/usr/bin
cp bin/linux-x64/Release/linux-x64/publish/RetroStationX AppDir/usr/bin
chmod +x AppDir/usr/bin
ln -sr AppDir/usr/bin/RetroStationX AppDir/AppRun

cp Linux/appimage/RetroStationX.48x48.png AppDir/RetroStationX.png
cp Linux/appimage/RetroStationX.desktop AppDir/RetroStationX.desktop
mkdir -p AppDir/usr/share/applications && cp ./AppDir/RetroStationX.desktop ./AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons && cp ./AppDir/RetroStationX.png ./AppDir/usr/share/icons
mkdir -p AppDir/usr/share/icons/hicolor/48x48/apps && cp ./AppDir/RetroStationX.png ./AppDir/usr/share/icons/hicolor/48x48/apps

chmod a+x appimagetool
./appimagetool AppDir/ RetroStationX.AppImage
