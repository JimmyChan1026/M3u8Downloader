#ifndef CURL_Download
#define CURL_Download
#include <curl/curl.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <regex>
#include <mutex>
#include "speedTracking.h"

using namespace std;
static size_t WriterList(char *contents, size_t size, size_t nmemb, void *userp);
static size_t WriterTs(char *contents, size_t size, size_t nmemb, void *userp);
CURLcode downloadwithcurl(CURL *curl, struct curlData &chunk, string website);
CURLcode downloadMediaList(string path, string website, string m3u8File);
CURLcode downloadTs(string website, string strippedWebsite, SpeedTracking *spO, ofstream& resultLog, string container);
struct curlData{
    char *memory;
    size_t size;
};

#endif
