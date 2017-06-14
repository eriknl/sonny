# sonny
[WIP] Sonoff firmware replacement with OTA update, MQTT and more

Implemented features:
- HAL to setup features depending on Sonoff hardware used
	- Sonoff S20, dual implemented, but other models will follow
	- Custom triggers for inputs can be set to allow stand alone operation
	- LEDs for status
- MQTT support
	- Publishers for inputs
	- Subscribers and publishers for outputs
- Settings manager
	- Based on SPIFFS files
	- Settings stored binary
	- Reset to defaults
- HTML generation
	- Generate pages on the fly without javascript and minimum code size
- OTA firmware updating

IO related functionality is dynamically allocated, in theory this would allow remapping functionality during runtime.
Set SONOFF_DEVICE macro to device type used. Perhaps this could be detected at runtime to improve usability

Todo:
- Add and test more device types
- Non-binary IO
- Low priority
	- Improve web interface
