# 1. ESP32 MQTT Handler & Configurator

The library `lib/comms_handler` was made to demonstrate an end to end approach to having a resilient configurable MQTT connection. 

The end goal was to demonstrate a drop in place component that abstracts all the communication into a few lines, while still being a customizable and configurable system. The target use case was to make it so that a future developer working on an Industrial IoT project would be able to focus on just their direct target and not the tangential communication, updating, and similar parts of an IIoT deployment.

All individual components of this demonstration were tested, however the complete integration is still incomplete. For all intents and purposes this library is (unfortunately) a glorified proof of concept.

There are 4 primary components to this project:
- `configwebpage`, provides an AP with a captive portal, and a Async Web Server with a page for device configuration and for viewing the status of the device
- `confighelper`, wraps around the `Preferences.h` library from Espressif for persistent configuration settings, it uses an ArduinoJSON document to store the configuration in RAM while being used,it also allows for factory resets
- `otahelper`, wraps around the `Update.h` library from Espressif, handles checking a URL to a raw JSON file with details about an available OTA update
- `comms_handler`, handles switching between LTE and WiFi for sending MQTT messages from the `main.cpp` file to an MQTT broker. Also instantiates the other components.
- Python Scripts, there are three python scripts to assist with the library. 
  - `preprocessor.py` - a pre script to compress the webpage HTML file. 
  - `versioning.py` - a pre script to create a header file with the appropriate semantic versioning definitions. 
  - `bincreate.py` - a post script to move the firmware `.bin` file to an appropriate folder depending on the PlatformIO environment, and create a JSON file representing the semantic version of the firmware along with some other details.

[PDF Doxygen Documentation (Incomplete)](https://github.com/Hamza-Anver/esp32-mqtt-handler-pio-0/blob/main/ref240830.pdf)

## Library Structure
```
library.json
src/
  ├── comms_handler.cpp
  ├── comms_handler.h
  ├── config/
  │   ├── confighelper.cpp
  │   ├── confighelper.h
  │   └── configkeys.h
  ├── configwebpage/
  │   ├── configwebpage.cpp
  │   ├── configwebpage.h
  │   └── webpage/
  │       ├── endpoints.h
  │       ├── prepped.html
  │       ├── preprocess.py
  │       ├── webpage.cpp
  │       ├── webpage.h
  │       └── websrc.html
  └── ota/
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
  - [3.1. Configuration Page](#31-configuration-page)
  - [3.2. Configuration Helper](#32-configuration-helper)
  - [3.3. OTA Helper](#33-ota-helper)
  - [3.4. Comms Handler](#34-comms-handler)



# 2. Detailed Component Overview
## 2.1. Meta Headers
`include/metaheaders.h`
This version file contains relative file locations to other 'meta components' of the project. There are various header files containing C style definitions that are shared across multiple files. For example each value in the configuration has a unique string as a key. To avoid human errors the keys are saved as definitions. 

In order to further reduce human errors, the preprocessor python script uses the definitions from the files referenced by the metaheaders file to find and replace placeholders in the HTML file prior to compression. 

In addition to keys, there are references to REST API endpoints that are shared between the Async Web Server and the HTML file.

## 2.2. Configuration Page
```
configwebpage/
  ├── configwebpage.cpp
  ├── configwebpage.h
  └── webpage/
      ├── endpoints.h
      ├── prepped.html
      ├── preprocess.py
      ├── webpage.cpp
      ├── webpage.h
      └── websrc.html
```

The configuration page is currently the most complex component of this project. The webpage source is in `websrc.html`, `preprocess.py` takes the source file and then looks for all the definitions provided by the files referred to from `metaheaders.h`. The correctly formatted file is saved temporarily in `prepped.html`. The script then uses gzip to compress the HTML file, and creates a variable in `webpage.cpp` with the hexadecimal of the compressed page in an array along with a reference to the total length of the array.

The webpage has several JS functions as part of its functionality. All forms send their data as a POST request to the action endpoint specified in the form tag when they are submitted. There are Event listeners the `/events` endpoint to show the user updates and feedback once received. For example if a string is sent to the events endpoint with the message 'updategood'. The appropriate JS function will open a modal window with that string as text and the 'updategood' CSS class applied to the text. This allows for asynchronous updates to be sent to the webpage, whether in terms of active downloads, or attempted WiFi connections. Using events is not fully implemented in the configwebpage.

The `configwebpage.cpp` has the class `ConfigWebPage`. This relies on the class `OTAHelper` and `ConfigHelper`. In its constructor ConfigWebPage creates the other classes as objects and assigns pointers to them. 

The constructor then starts a WebServer on port 80, and sets up a captive portal using ESP's DNS Server library. With all requisite endpoints being set to the root page.

Once the captive portal is set, the configwebpage uses the definitions from `endpoints.h` to declare all the REST API endpoints appropriately. The rest of the class is a series of functions called with the endpoints for the functionality of the site. A typical example would be the endpoint for saving Access Point credentials for next boot. Where the function first verifies the Strings passed in by the form, and then uses the events endpoint to send a message with either what failed or that the credentials were saved successfully.

The WiFi scanning is a more complex endpoint setup which requires further optimization and fixes. The first button calling for the scan to start calls a FreeRTOS task which waits for the scan to be complete before sending the scan results as a JSON string in an XML request. For connection the button calls another FreeRTOS task, which sends periodic updates to the webpage on the current connection status.

The webpage also has a device status section containing various device parameters, such as uptime, max free heap alloc, max heap, WiFi connection status, IP address etc. This is periodically sent from a FreeRTOS task to the events endpoint, where a JS function formats it into a table. Any other function can add to the ArduinoJSONDocument variable to have its data be sent to the webpage periodically.


## 2.3. Configuration Helper
```
config/
  ├── confighelper.cpp
  ├── confighelper.h
  └── configkeys.h
```
`configkeys.h` contains the keys to refer to the values of the configuration. 
This module is a simple wrapper around Espressif's Preferences Library for the Arduino, and has some basic functionality for factory resetting and saving new parameters. This module implementation is also very basic and naive. It simply has a set of overloaded functions for getting and setting values of types int, bool, float and String. 


## 2.4. OTA Helper
```
ota/
  ├── otahelper.cpp
  └── otahelper.h
```

This module abstracts some of the aspects of OTA updates. It relies on Espressif's Update library. Currently it supports checking a URL for a JSON file for metadata about any new version. And then relies on the HTTP Client library to fetch the data from a URL in the JSON file if there is an available update. It also has support for receiving an OTA firmware update from the config portal in segments and writing it to flash. This has many issues, but functionality has been demonstrated.

Example JSON update config:
```json
{
    "name": "v1.0.36",
    "major": 1,
    "minor": 0,
    "patch": 36,
    "env": "nb-iot-nodemcu-32s-4mb",
    "binurl": "https://github.com/Hamza-Anver/esp32-mqtt-handler-pio-0/raw/main/binfiles/nb-iot-nodemcu-32s-4mb/nb-iot-nodemcu-32s-4mb.bin"
}
```

## 2.5. Comms Handler
```
src
  ├── comms_handler.cpp
  ├── comms_handler.h
  ├── config/
  ├── configwebpage/
  └── ota/
```

This 'module' wraps up all the sub modules and is the interface between the main program and the sub modules. This is intended to sort out the logic of whether to use WiFi or LTE to send MQTT packets. Checking when to switch between if necessary. Along with hardware reset functionality. When instantiated it starts up the configwebpage and attempts connecting to a known WiFi network if there are no connections to the ESP32 AP (to avoid user connection issues). 

The comms handler is structured to have one FreeRTOS task for managing LTE, one for managing WiFi, and a main task for handling the queue of MQTT messages. The main task is responsible for checking the queue and sending the messages to the appropriate FreeRTOS task. The FreeRTOS tasks are responsible for sending the messages to the MQTT broker. This is not fully implemented.

## 2.6. Python Scripts
There are three python scripts to assist with the library.
- `preprocessor.py` - a pre script to compress the webpage HTML file.
- `versioning.py` - a pre script to create a header file with the appropriate semantic versioning definitions.
- `bincreate.py` - a post script to move the firmware `.bin` file to an appropriate folder depending on the PlatformIO environment, and create a JSON file representing the semantic version of the firmware along with some other details.

The implementations and usage of these scripts is mostly arbitrary and can be changed as needed. They were made without much thought to the future, and are not very robust.

# 3. Future Improvements & To Dos
This project is incomplete and has many issues. In general: usage of Arduino's String class and dynamic ArduinoJSON has been done with minimal regard to heap fragmentaton and memory leaks. This should be fixed

## 3.1. Configuration Page
- Everything should be moved to use an event or WebSocket system
- Bulk write of configuration via a string or file of JSON is needed
- Downloading of configuration as a JSON file is needed
- Find and replace should be made case insensitive (i.e. for finding and replacing '{UID}' for SSID and password settings)
- Generation of the page via script can include more error handling and checks of definitions and other parameters
- The HTML and JS code of the webpage should be cleaned up and optimized
- Minification of the HTML code should be done prior to compression
- More diagnostic and status functions should be added
- User management and security should be added (i.e. Access Control)
- Stylisation and responsiveness could be improved

As a whole this implementation is inflexible and not very robust. The webpage UI/UX is not optimal, and future additions may be difficult to implement. Ideally this entire configuration page should be rethought and redeisgned into a single independent component that can be dropped into any project instead of being tightly coupled to the rest of the library.

## 3.2. Configuration Helper
- The configuration helper should be able to handle more data types
- There needs to be significantly improved error handling capabilities, e.g. check if memory read matches parameters in factory defaults, or falling back incase some value was incorrectly saved.
- There needs to be significantly improved default value (factory reset) capabilities
- Reading and writing to the configuration should be done in a more efficient manner, since there is limited R/W cycles on the flash memory

This module is a basic wrapper around the Preferences library, it fulfils its functionality but adding more functionality may pose an issue to developers. The module is already based on a very simple library, and making it into its own independent library is simply reinventing the wheel in its current state. 

Perhaps using a JSON file to store the factory configuration is better suited to this projets use case and would allow for more flexibility and ease of use. This does introduce issues of how to parse and split the JSON file to minimize R/Ws and overheads.

## 3.3. OTA Helper
The OTA helper library is currently a naive implementation. For the intended deployment of this software it can be considered one of the most important components. 

Though using HTTP GET requests to receive files for OTA over the internet is the clearest approach it poses a few issues in this case. Mainly being the duplication of an entire communication protocol for the sake of a single use case. Along with added complexities in needing extra overhead for authentication and security.

The OTA helper could be moved to use MQTT packets to receive the firmware update, and report back the state of the update. This approach allows the use of the existing MQTT connection and its security for sending IIoT data to also be used as a secure channel for firmware updates. This approach is in line with Amazon's AWS IoT OTA update approach. It does require deliberation on how best to approach the ideal structuring of this approach for the intended use case, and unintended future use cases.

A simple implementation would be having each device subscribe to a topic of its own UID, and then a python script could watch a GitHub Repo, pull the latest compiled firmware when there is a change. Split the firmware into segments and send them to the device over MQTT. The device could then sequentially write the segments to flash and then restart. This approach is simple and straightfoward, but requires development of a python script to handle the firmware splitting and sending, along with devices needing to be online to receive the update. Expanding this approach to networks of devices in the 1000's may be impractical. However, the benefits of reusing the already necessary MQTT connection for the firmware update are significant.

This module can also be spun off into its own library and be a drop in place component for any ESP32 project.

## 3.4. Comms Handler
This module is supposed to use the sub modules together in order to provide a dead simple interface to send MQTT packets to an end developer and have the system handle the rest. This module is currently missing the majority of its functionality. Earlier iterations demonstrated the ability to switch between WiFi and LTE, and send MQTT packets. However, the current implementation was left incomplete in favor of other components. Logic to periodically check for updates is also missing. A lot of logic to minimize memory impact and to deal with extended periods of disconnection can be implemented here to further augment the robustness of the IIoT system. 

The current structure of three FreeRTOS tasks is almost entirely arbitray and could/should be switched out to a better thought out system. The current structure is based on the idea of having a main task to handle sending messages, and two tasks to handle maintaining connections. This structure may or may not be optimal, and should be rethought.

Significant improvements can also be made here to improve error handling and interfacing. Interfacing with the main program about status updates, and errors is currently not implemented. How to deal with status updates and errors is likely going to be partially dependent on the implementation of the device using this library, and as such should be left to the main program to implement. Further optimizations can also be made here, including but not limited to: heap management (i.e. restart device if heap is too low), saving messages to flash if they cannot be sent, not starting the AP unless necessary, discarding the AP and the webpage when and if not needed, opening the webpage on demand, etc. 

The demands of this library also cross over into the other components.

