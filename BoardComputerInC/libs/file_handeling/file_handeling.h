#ifndef _FILE_HANDELING_H_
#define _FILE_HANDELING_H_

#include <stdio.h>
#include "../datastruct/datastruct.h"


int createFile(char* filename);
int writeDataStructToFile(char* filename, const DATAPACKET* value);
int writeDataStructArrayToFile(char* filename, DATAPACKET* array, int amountOfDatapackets);
int getNrOfDatStructs(char* filename);
int readDataStructFromFile(char* filename, DATAPACKET* info, int pos);
int readAllDataFromFile(char* filename, DATAPACKET* array, int number);
int removeFile(char* filename);

#endif