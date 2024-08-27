# 1. ESP32 MQTT Handler & Configurator

The library `lib/comms_handler` was made to demonstrate an end to end approach to having a resilient configurable MQTT connection. 

The end goal was to demonstrate a drop in place component that abstracts all the communication into a few lines, while still being a customizable and configurable system. The target use case was to make it so that a future developer working on an Industrial IoT project would be able to focus on just their direct target and not the tangential communication, updating, and similar parts of an IIoT deployment.

All individual components of this demonstration were tested, however the complete integration is still incomplete.

There are 4 primary components to this project:
- `configwebpage`, provides an AP with a captive portal, and a Async Web Server with a page for device configuration and for viewing the status of the device
- `confighelper`, wraps around the `Preferences.h` library from Espressif for persistent configuration settings, it uses an ArduinoJSON document to store the configuration in RAM while being used,it also allows for factory resets
- `otahelper`, wraps around the `Update.h` library from Espressif, handles checking a URL to a raw JSON file with details about an available OTA update
- `comms_handler`, handles switching between LTE and WiFi for sending MQTT messages from the `main.cpp` file to an MQTT broker. Also instantiates the other components.
- Python Scripts, there are three python scripts to assist with the library. 
  - `preprocessor.py` - a pre script to compress the webpage HTML file. 
  - `versioning.py` - a pre script to create a header file with the appropriate semantic versioning definitions. 
  - `bincreate.py` - a post script to move the firmware `.bin` file to an appropriate folder depending on the PlatformIO environment, and create a JSON file representing the semantic version of the firmware along with some other details.

## Library Structure
```
library.json
src
  ├── comms_handler.cpp
  ├── comms_handler.h
  ├── config
  │   ├── confighelper.cpp
  │   ├── confighelper.h
  │   └── configkeys.h
  ├── configwebpage
  │   ├── configwebpage.cpp
  │   ├── configwebpage.h
  │   └── webpage
  │       ├── endpoints.h
  │       ├── prepped.html
  │       ├── preprocess.py
  │       ├── webpage.cpp
  │       ├── webpage.h
  │       └── websrc.html
  └── ota
      ├── otahelper.cpp
      └── otahelper.h

```

## 1.1. Table of contents
- [1. ESP32 MQTT Handler \& Configurator](#1-esp32-mqtt-handler--configurator)
  - [Library Structure](#library-structure)
  - [1.1. Table of contents](#11-table-of-contents)
- [2. Detailed Component Overview](#2-detailed-component-overview)
  - [2.1. Meta Headers](#21-meta-headers)
  - [2.2. Configuration Page](#22-configuration-page)
  - [2.3. Configuration Helper](#23-configuration-helper)
  - [2.4. OTA Helper](#24-ota-helper)
  - [2.5. Comms Handler](#25-comms-handler)
  - [2.6. Python Scripts](#26-python-scripts)
- [3. Future Improvements \& To Dos](#3-future-improvements--to-dos)



# 2. Detailed Component Overview
## 2.1. Meta Headers
`include/metaheaders.h`
This version file contains relative file locations to other 'meta components' of the project. There are various header files containing C style definitions that are shared across multiple files. For example each value in the configuration has a unique string as a key. To avoid human errors the keys are saved as definitions. 

In order to further reduce human errors, the preprocessor python script uses the definitions from the files referenced by the metaheaders file to find and replace placeholders in the HTML file prior to compression. 

In addition to keys, there are references to REST API endpoints that are shared between the Async Web Server and the HTML file.

## 2.2. Configuration Page
```
configwebpage
  ├── configwebpage.cpp
  ├── configwebpage.h
  └── webpage
      ├── endpoints.h
      ├── prepped.html
      ├── preprocess.py
      ├── webpage.cpp
      ├── webpage.h
      └── websrc.html
```

The configuration page is currently the most complex component of this project. The webpage source is in `websrc.html`, `preprocess.py` takes the source file and then looks for all the definitions provided by the files referred to from `metaheaders.h`. The correctly formatted file is saved temporarily in `prepped.html`. The script then uses gzip to compress the HTML file, and creates a variable in `webpage.cpp` with the hexadecimal of the compressed page in an array along with a reference to the total length of the array.



## 2.3. Configuration Helper
```
config
  ├── confighelper.cpp
  ├── confighelper.h
  └── configkeys.h
```
## 2.4. OTA Helper
```
ota
  ├── otahelper.cpp
  └── otahelper.h
```

## 2.5. Comms Handler
```
src
  ├── comms_handler.cpp
  ├── comms_handler.h
  ├── config
  ├── configwebpage
  └── ota
```

## 2.6. Python Scripts


# 3. Future Improvements & To Dos


