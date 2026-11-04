# CANdash

A vehicle dashboard simulator built around the CAN bus, written in C++17.

The simulator generates vehicle data (speed, RPM, coolant temperature, battery charge),
encodes it into CAN frames according to a DBC file and sends them to the bus.
The reader receives the frames, decodes them and passes the values to a dashboard UI.

> Work in progress. A hands-on deep dive into the CAN bus stack (DBC signal encoding, SocketCAN) and a refresh of Qt for the dashboard UI.
