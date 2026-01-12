<h1 align="center">
    WavePhoenix
</h1>

<p align="center">
    Open-source implementation of the Nintendo WaveBird receiver using Silicon Labs Wireless Gecko SoCs
</p>
<p align="center">
    <a href="https://discord.gg/W44D5uUq4v"><img src="https://img.shields.io/discord/1377009241654689932?style=for-the-badge&logo=discord&logoColor=%23FFFFFF&label=DISCORD&labelColor=%235865F2&color=%234954c9" alt="Discord"></a>
</p>

<p align="center">
    <img src="images/logo-animated.gif" alt="WavePhoenix" />
</p>

## Motivation

The WaveBird controller is, in my opinion, one of the best controllers ever made. It is wireless, has an insane battery life, and is very comfortable to hold. The WaveBird set a new standard for wireless controllers. It was the first major console controller to use radio frequency (RF) technology, providing a reliable, lag-free connection that didn't require line-of-sight, unlike infrared controllers.

Unfortunately, Nintendo stopped producing the WaveBird more than a decade ago, causing a dwindling supply of controllers and, especially, receivers. Given the decreasing supply of receivers and the increasing resale prices, I decided to design my own from scratch.

## Features

The reference implementation of WavePhoenix includes the following features:

- Full compatibility with the original WaveBird controller
- One-button virtual pairing, like modern wireless devices
- Status LED to indicate pairing status and radio activity
- Over-the-air firmware updates via Bluetooth
- Open source hardware and firmware
- 3D printable case

## Buying a WavePhoenix

Don't want to build your own? A number of vendors are now selling pre-assembled WavePhoenix receivers!

- [Laser Bear Industries](https://www.laserbear.net/products/wavephoenix-replacement-wavebird-gamecube-receiver) - US-based seller, custom enclosure and PCB by Greg
- [Colester Productions](https://shop.colesterproductions.com/product/wave-phoenix) - US-based seller, uses my original PCB and [
SadSnifit's fantastic case](https://makerworld.com/en/models/1463984-wavephoenix-wavebird-shell)
- [Various AliExpress sellers](https://www.aliexpress.us/w/wholesale-%22wavephoenix%22.html)

## Building a WavePhoenix

The bare minimum hardware needed to build a WavePhoenix is a EFR32BG22 radio module (such as the [RF-BM-BG22C3](https://www.rfstariot.com/rf-bm-bg22c3-efr32bg22-bluetooth-module_p93.html)), a GameCube controller connector, and 3 wires!

Since most people prefer a more streamlined solution, I designed a custom PCB and 3D-printable case called the *Mini Receiver*.

You can find detailed build instructions in the [hardware/mini-receiver](hardware/mini-receiver) directory.

## Firmware Components

The firmware for WavePhoenix is composed of the following components:

- [`libwavebird`](https://github.com/loopj/libwavebird) - my implementation of Nintendo's WaveBird protocol for Silicon Labs Gecko SoCs
- [`libjoybus`](https://github.com/loopj/libjoybus) - my implementation of the Joybus protocol used by N64 and GameCube controllers
- [`firmware/app`](firmware/app) - the main app firmware; a reference implementation of a WaveBird receiver
- [`firmware/bootloader`](firmware/bootloader) - bootloader to provide over-the-air firmware updates via Bluetooth

## Worklog

Check out the [worklog](WORKLOG.md) for a detailed history of the development of the WavePhoenix project.

## Special Thanks

- [Sam Edwards](https://github.com/CFSworks) for his incredible WaveBird reverse engineering documentation, which was essential in getting this working
- [Jeff Longo](https://jefflongo.dev) - for his detailed GameCube controller protocol documentation
- [Aurelio Mannara](https://github.com/Aurelio92) - for open sourcing GC+, another great reference for the GameCube controller protocol
- [YveltalGriffin](https://bsky.app/profile/mks.bsky.social) - for help and advice along the way, and helping me figure out those mysterious origin packets
- piotref1 - for being the very first person to build a WavePhoenix receiver and provide feedback, a true early adopter!
- Everyone in the incredible [BitBuilt community](https://bitbuilt.net/) for their support and encouragement throughout this project

## License

WavePhoenix is a permissively licensed open source project.

The firmware is licensed under the [MIT License](firmware/LICENSE).

The hardware is licensed under the [Solderpad Hardware License v2.1](hardware/LICENSE).
