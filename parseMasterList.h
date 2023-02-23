#ifndef PARSE_MASTERLIST
#define PARSE_MASTERLIST
#include <string>
#include <pthread.h>
#include <sstream>
#include <vector>
#include "curlDownload.h"
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

using namespace std;
bool start(const char *memory, string website, SpeedTracking * spO);
void *parseListTh(void * input);
string audioList(const char *memory, string website, string targetGroupId);

string parseMasterList(const char *memory, string website, SpeedTracking * spO);
string stripListForFile(string website);
string modifiedList(string masterList, string extension);
string stripForMedia(string website);

bool createDirAndDownload(const char *fileName, string tsLink, string playlistToDownload, SpeedTracking *spO, ofstream& resultLog);

#endif
