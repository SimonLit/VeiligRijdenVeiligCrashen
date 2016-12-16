#ifndef SERIAL_READER_H
#define SERIAL_READER_H

int getIncomingSerialMessage(char* receiveBufferCommand, char* receiveBufferValue);
// The receiveBufferLength is the maximum length of the buffer.
// 
//Pre: the recieveBuffer pointer is not NULL.
//Post: a message over serial read from '#' to '%' is stored in the receiveBuffer
//Return: -1 if there in no serial data to read, one of the char pointers is NULL or receiveBufferLength > 10.
//		   0 if '%' was received.

void sendACK(void);

void sendNACK(void);

void sendConnectACK(void);

void sendRP6Satus(char* state);
//
// Pre: -
// Post:
// Return:

#endif