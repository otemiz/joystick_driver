# joystick_driver
This is a simple joystick driver for gamepad devices in Ubuntu/L4T. It is tested with PS5 controller connected via bluetooth.

- To use the driver fist call the constructor of your choice:

`joystickInterface()`

or

`joystickInterface(std::string device_, bool debug_)`.

Default contructor tries to open the hoystick device at: "/dev/input/js0".

- Then start the joystick thread with bool makeJoystickThread()

- Read the joystick buttons with: `getJoystickStruct()`

