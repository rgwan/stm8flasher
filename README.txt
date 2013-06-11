stm8flash currently supports only 32K (Medium Density) and 128K (High Denisty) Devices. Where only STM8S105 has been tested.

stm8flash only works in "REPLY Mode" at the moment - meaning that every received byte has to be echoed back to the STM8

Reading from STM8 is very slow due to a serial write lag of around 10ms per single transfer - this issue needs more investigation "serinfo.flags |= ASYNC_LOW_LATENCY;" didn't resolve this issue. As we are working in "REPLY Mode" this issue really slows things down when reading from the STM8.


