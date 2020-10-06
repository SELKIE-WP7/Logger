#ifndef SELKIELoggerBaseSerial
#define SELKIELoggerBaseSerial

int openSerialConnection(const char *port, const int baudRate);
int baud_to_flag(const int rate);
int flag_to_baud(const int flag);
#endif
