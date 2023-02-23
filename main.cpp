#include "curlDownload.h"
#include "parseMasterList.h"
#include <iostream>
#include <string>
using namespace std;

int main(){
	
	CURL *curl;
    CURLcode res;
    struct curlData newChunk;
	string website;
	newChunk.memory = NULL;
	newChunk.size = 0;
	SpeedTracking* spO = new SpeedTracking();

	cout << "Enter m3u8 to download: ";
	cin >> website;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
	
    res = downloadwithcurl(curl, newChunk, website);
    if(res != CURLE_OK){
	    cout << "curl error: " << curl_easy_strerror(res) << endl;
        return 0;
    }
	
	int ret = start(newChunk.memory, website, spO);
	if(!ret){
		cout << "Couldn't start" << endl;
	}
	return 0;
}
