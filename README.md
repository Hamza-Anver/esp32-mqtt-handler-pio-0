TODO:

Replace strings with MAC

Enable disable config options depending on usecase

Switch to WiFi Multi

# Planning:
## Configuration Options

### Status
Connected to WiFi
Firmware version?
Heap usage max heap
Device up time

### Device Settings
Device UID (Use MAC by default)
Checkbox - Password lock configuration
Device username
Device Password

(Future addition: access rights based on user)
### Access Point
AP turning on options
    1. Turn on AP always
    2. Turn on AP when there is no WiFi network
    3. Turn off AP always (caution)

### Internet Access
Options:
    1. Use WiFi only
    2. Use LTE only
    3. Use LTE and WiFi, prioritize WiFi (default)
    4. Use LTE and WiFi, prioritize LTE

WiFi
    Station SSID
    Station Password
    (Reconnecting timeout?)
    (Support for multiple SSIDs?)

LTE
    APN
    (Reconnecting timeout?)

(Test connections?)

### MQTT
MQTT client ID
MQTT server address
MQTT server port
MQTT keep alive time
MQTT clean session
MQTT LWT Topic
MQTT LWT Message
MQTT message queue size (?)


### OTA
URL to check for updater.json (decide on format etc)
URL to immediately update firmware OTA
Upload file to upload through the portal

