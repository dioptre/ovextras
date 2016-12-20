HidDevice class description
***************************

The class "HidDevice" is an interface developped in C++ for the interaction with HID devices.

There is a constructor without argument that allow to create an object and instantiate it later, and also a constructor with instantiation values.

A function setHidDeviceInfos() is used to update the device information: the vendor ID, the product ID, and the number of bytes that the device should expect to receive. When the object is created and its information is defined, the user can then call the connect() function. This function returns "true" when the connection is successful. Communication with the equipment can then begin.

The function writeToDevice() allow to send data to the connected device. This function takes two arguments, a byte array which contains the data and a number that correspond to the number of byte within the data array.

Four "callback" methods are available:

- dataReceived

- dataConnected

- deviceAttached

- deviceDetached

Users can assign these function pointers to their own functions to execute the code they want.

The "HidDeviceNotifier" is another class inside "HidDevice" that lets you know the status of the device at any time. It is this class that gives information about whether the device was plugged-in or unplugged after the first connection.
