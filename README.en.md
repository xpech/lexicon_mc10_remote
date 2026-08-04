# Lexicon MC-10 Remote (ESP32)

[Français](README.md) | [English](README.en.md)

> **Disclaimer — independent project**
>
> Lexicon® and Harman Kardon® are trademarks of HARMAN International Industries, Incorporated, registered in the United States and/or other countries. This independent project has no connection with HARMAN, Lexicon, or Harman Kardon and is not affiliated with, sponsored by, or endorsed by any of those companies. It is developed solely out of a passion for keeping high-quality audio equipment working.

This project turns an ESP32 into a local web remote control for a high-end audio system:

- Lexicon MC-10 (main control)
- Azur 840 preamplifier (serial control)
- General power control

The project reuses a legacy "coolhome" codebase for heating, irrigation, and temperature reporting, and extends it for audio/video use.

## Goals

- Provide a simple and responsive local web interface for controlling the Lexicon MC-10.
- Add control for the Azur 840 preamplifier.
- Keep existing networking, OTA, sensor, and telemetry components when useful.
- Allow a gradual evolution toward a unified, fully web-based remote control.

## Technical stack

- Target board: ESP32 (PlatformIO environment: `wemos_d1_mini32`)
- Framework: Arduino
- Embedded HTTP server: WebServer
- Embedded filesystem: LittleFS (web pages in `data/`)
- Software serial connections:
  - Lexicon: RX GPIO 18, TX GPIO 19
  - Azur 840: RX GPIO 16, TX GPIO 17

## Useful project structure

- `src/main.cpp`: global setup (Wi-Fi, mDNS, OTA, and legacy routes)
- `src/lexicon.cpp`: Lexicon protocol and Lexicon web/API endpoints
- `src/azur840.cpp`: Azur 840 protocol and Azur web/API endpoints
- `data/lexicon.html`: Lexicon web UI
- `data/azur.html`: Azur 840 web UI
- `data/shared-ui.css`: shared styles
- `data/i18n.js`: UI translations and language preference storage
- `platformio.ini`: build and dependency configuration

## Requirements

- PlatformIO Core installed (`pio` or `platformio` available in `PATH`)
- ESP32 board connected over USB
- UART wiring suitable for the controlled equipment

## Build and flash the firmware

From the project root:

```bash
pio run -e wemos_d1_mini32
pio run -e wemos_d1_mini32 -t upload
```

To specify the serial port explicitly:

```bash
pio run -e wemos_d1_mini32 -t upload --upload-port /dev/tty.usbserial-XXXX
```

## Upload the web assets (LittleFS)

The web files from `data/` must be uploaded to LittleFS.

macOS/Linux:

```bash
./upload_data_mac.sh
# or with an explicit port
./upload_data_mac.sh wemos_d1_mini32 /dev/tty.usbserial-XXXX
```

Windows:

```bat
upload_data_windows.bat
:: or with an explicit port
upload_data_windows.bat wemos_d1_mini32 COM4
```

## Accessing the interface

At startup, the device:

- attempts to connect to Wi-Fi using known credentials;
- starts in access-point mode if the connection fails;
- publishes a dynamic hostname and HTTP service over mDNS.

Main endpoints:

- `GET /lexicon`: Lexicon web page
- `GET /azur840`: Azur web page
- `GET /shared-ui.css`: shared stylesheet
- `GET /i18n.js`: French/English UI translations

## Interface localization

The web interface is available in French and English without duplicating the HTML pages.

- The language is selected from the `Setup` page.
- The choice is stored in the browser under the `localStorage` key `lexicon.locale`.
- On first access, the browser language is used when supported.
- French is the fallback language.
- Static translations and dynamically generated button labels are centralized in `data/i18n.js`.

The preference belongs to the web origin being used. Access through the mDNS hostname and direct access through the device IP address may therefore have separate preferences.

## Lexicon API

### Standard command

- `GET /lexicon_cmd?zone=..&command=..&data=...`
- `GET /lexicon_cmd?zone=..&command=..&datahex=...`

Parameters:

- `zone` and `command`: hexadecimal bytes (for example `01`, `0A`, or `FF`)
- `data`: ASCII payload
- `datahex`: hexadecimal binary payload with an even length; spaces, `:`, and `-` separators are accepted

Typical response:

- `AC=<hex_response_code> DATA=<payload>`

### RC5 command

- `GET /lexicon_rc5?zone=..&command1=..&command2=..`

Parameters:

- `zone`, `command1`, and `command2`: hexadecimal bytes

Typical response:

- `AC=<hex_response_code> RC5=<2 hexadecimal bytes>`

## Azur 840 API

Endpoints:

- `GET /azur840_api`
- `POST /azur840_api`

Supported modes:

- Direct ASCII mode through `tx`
- Hexadecimal mode through `tx_hex`
- Structured mode through `group`, `command`, and `data`

Structured mode rules:

- `group` between 1 and 5
- `command` between 0 and 99
- `data` up to 10 characters
- emitted format: `#<group>,<two_digit_command>,<data>`

## OTA

Firmware update endpoints:

- `GET /update`
- `POST /update` (binary upload)

## Legacy features from the coolhome codebase

The code still contains legacy features:

- DHT and Dallas sensors
- heating and irrigation logic
- periodic data reporting to a server

These components may remain enabled depending on the configuration, but they are not part of the audio remote's core functionality.

## Current status and next steps

Current status:

- functional ESP32 base
- Lexicon and Azur web interfaces available
- serial APIs implemented for both Lexicon and Azur

Recommended next steps:

1. Merge the web screens into a single remote control for sources, volume, and power macros.
2. Stabilize the power macros with a safe startup and shutdown order.
3. Improve logging and user-facing state feedback for acknowledgements, timeouts, and errors.
4. Gradually isolate or remove the legacy heating and irrigation code when unused.

## Authors

Original codebase:

- Pierre Le Noan
- Xavier Péchoultres
