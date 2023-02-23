#include "parseMasterList.h"
string audioListDown;
mutex tsFile_mutex;
mutex logFile_mutex;

bool start(const char *memory, string website, SpeedTracking * spO)
{
	//SpeedTracking* speed_tracking = new SpeedTracking();
	CURLcode res;
	string mediaList, playlistForDownload, tsLink;
	mediaList = parseMasterList(memory, website, spO);

   	//if source is master list, do the following
   	if(mediaList != ""){
        playlistForDownload = modifiedList(website, mediaList);
	    if(playlistForDownload != ""){
			CURL *curl;
        	CURLcode res;
	   	    struct curlData tsData;
    	    res = downloadwithcurl(curl, tsData, playlistForDownload);
           	tsLink = parseMasterList(tsData.memory, playlistForDownload, spO);
            if(res != CURLE_OK){
   	            cout << "(Media Playlist) curl error: " << curl_easy_strerror(res) << endl;
       	        return 0;
           	}else{
                //cout << tsData.memory << endl;
   	        }
       	}else{
           	cout << "No playlist to download." << endl;
			return 0;
   	    }
	}
	return 1;
}

string audioList(const char *memory, string website, string targetGroupId)
{
	string mem = memory;
	string line;
	istringstream iss(mem);

	while(getline(iss, line)){
		if(line.compare(0, strlen("#EXT-X-MEDIA:"), "#EXT-X-MEDIA:") == 0){
			const char* whole_command = line.c_str();
			const char* URI = strstr(whole_command, "URI");
			const char* TYPE = strstr(whole_command, "TYPE");
			const char* GROUPID = strstr(whole_command, "GROUP-ID");
            char temp_type[1024] = {0};
			char temp_groupid[1024] = {0};
            sscanf(TYPE, "TYPE=%1023[^,]", temp_type);
            if (strcmp(temp_type, "AUDIO") == 0 || strcmp(temp_type, "\"AUDIO\"") == 0){
                const char* GROUPID = strstr(whole_command, "GROUP-ID");
                if (GROUPID){
					sscanf(GROUPID, "GROUP-ID=\"%1023[^\"]", temp_groupid);
					string groupId = temp_groupid;
					if(groupId.compare(targetGroupId) == 0){
						const char* URI = strstr(whole_command, "URI");
						if (URI){
							char uri[1024] = {0};
							sscanf(URI, "URI=\"%1023[^\"]", uri);
							return uri;
						}
					}
				}
            }
		}
	}
	return "";
}

string parseMasterList(const char *memory, string website, SpeedTracking* spO)
{
	cout << "Webite in parseMasterList: " << website << endl;
	int version = 0;
	unsigned int bw = 0;
	unsigned int tempBw = 0;
	int targetDuration = 0;
	int sequence, newSequence;
	double tsDuration = 0;
	bool isVod = false;
	bool downloaded = false;
	bool checkAudio = false;
	char audio_id[1024] = {0};
	string base = "/var/www/html/jimmy/m3u8Downloader/";
	
	string playlistType;
	playlistType.clear();
	string playlistToDownload;
	playlistToDownload.clear();
	const char * a = strstr(website.c_str(), "audio");
	string strippedWebsite = stripListForFile(website);
	if(a){
		strippedWebsite = "audio_" + strippedWebsite;
	}else{
		strippedWebsite = "video_" + strippedWebsite;
	}
	string logFileName = "log_"+ strippedWebsite + ".txt";
	string mem = memory;
	string line;
	istringstream iss(mem);

	//Check Live or Vod
	const char *checkType = strstr(memory, "#EXT-X-ENDLIST");
	//Check if it is master/media list
	const char *checkMediaList = strstr(memory, "#EXT-X-STREAM-INF");

	if(checkMediaList){
		cout << "It is a Master playlist. Will choose highest bandwidth to download." << endl;
	}else{
		cout << "It is a Media playlist." << endl;
		if(checkType){
	//		cout << "It is VOD playlist. Need to download all .ts " << endl;
			isVod = true;
			cout << "playlistForDownload: " << strippedWebsite << endl;
			string mediaList = stripForMedia(website);
			string n = stripListForFile(website);
			string base = "/var/www/html/jimmy/m3u8Downloader/";
			string m3u8Container = base + strippedWebsite + "/";
			string mediaListPath = m3u8Container + mediaList;
		    CURLcode res;
/*
			res = downloadMediaList(mediaListPath, website, m3u8Container);
			if(res != CURLE_OK){
        		cout << "curl error creating medialist: " << curl_easy_strerror(res) << endl;
        		return 0;
    		}
*/
		}else{
			//cout << "It is Live playlist. Will donwload from 1st .ts" << endl;
			isVod = false;
		}
	}
	ofstream logForDownloaded(logFileName);

	while (getline(iss, line)){
		if(line.compare(0,strlen("#EXT-X-VERSION:"),"#EXT-X-VERSION:") == 0){
			sscanf(line.c_str(),"#EXT-X-VERSION:%d\n", &version);
		}else if(line.compare(0, strlen("#EXT-X-STREAM-INF"), "#EXT-X-STREAM-INF") == 0){
			const char* whole_command = line.c_str();
			const char* BANDWIDTH = strstr(whole_command,"BANDWIDTH");
			const char* AUDIO = strstr(whole_command, "AUDIO");
			if(!BANDWIDTH)
            {
	            cout << "#EXT-X-STREAM-INF wrong format! Ignored line = " << line.c_str();
                continue;
            }
			sscanf(BANDWIDTH,"BANDWIDTH=%u", &tempBw);
			if(bw <= tempBw){
				bw = tempBw;
				if(AUDIO){
					sscanf(AUDIO,"AUDIO=\"%1023[^\"]", audio_id);
				}
				getline(iss, playlistToDownload);
			}
		}else if(line.compare(0, strlen("#EXT-X-INDEPENDENT-SEGMENTS"), "#EXT-X-INDEPENDENT-SEGMENTS") == 0){
			//getline(iss, line);
		}else if(line.compare(0, strlen("#EXT-X-MEDIA:"), "#EXT-X-MEDIA:") == 0){
		}else if(line.compare(0, strlen("#EXT-X-TARGETDURATION:"), "#EXT-X-TARGETDURATION:") == 0){
			sscanf(line.c_str(), "#EXT-X-TARGETDURATION:%d\n", &targetDuration);
			cout << "target-duration: " << targetDuration << endl;
		}else if(line.compare(0, strlen("#EXT-X-MEDIA-SEQUENCE:"), "#EXT-X-MEDIA-SEQUENCE:") == 0){
			sscanf(line.c_str(), "#EXT-X-MEDIA-SEQUENCE:%d\n", &sequence);
		}else if(line.compare(0,strlen("#EXT-X-PLAYLIST-TYPE:"), "#EXT-X-PLAYLIST-TYPE:") == 0){
			playlistType = line.substr(strlen("#EXT-X-PLAYLIST-TYPE:"));
			if(playlistType == "VOD"){
				cout << "Type: " << playlistType << endl;
			}
		}else if(line.compare(0, strlen("#EXTINF:"), "#EXTINF:") == 0){ // next line will be .ts
			sscanf(line.c_str(), "#EXTINF:%le\n", &tsDuration);
			if(isVod){ //VOD: Download .ts from the begining
				getline(iss, playlistToDownload);

                // Create dir for video or audio
                // 1st arg: file name
                // 2nd arg: .ts address
                // 3rd arg: website to curl
				string tsLink = modifiedList(website, playlistToDownload);
				if(logForDownloaded.is_open()){
					bool c = createDirAndDownload(strippedWebsite.c_str(), tsLink, playlistToDownload, spO, logForDownloaded);
				}
				downloaded = true;
			}else{		//Live: get the first .ts
				getline(iss, playlistToDownload);
				const char *line = playlistToDownload.c_str();
				const char * key = strstr(line, "#EXT-X-KEY");
				const char * program = strstr(line, "#EXT-X-PROGRAM-DATE-TIME");
				if(key || program){
					getline(iss, playlistToDownload);
				}else{
					string tsLink = modifiedList(website, playlistToDownload);
					if(logForDownloaded.is_open()){
						bool c = createDirAndDownload(strippedWebsite.c_str(), tsLink, playlistToDownload, spO, logForDownloaded);
					}
					break;
				}
			}
		}
	}
	if(downloaded || !checkMediaList){
		playlistToDownload = "";
	}
	
	if(!checkAudio && checkMediaList){
		string temp = audioList(memory, website, audio_id);
		if (temp != ""){
			checkAudio = true;
			string audioListToDownload = modifiedList(website, temp);
			pthread_t audio_thread;
			const char *al = audioListToDownload.c_str();
			audioListDown = audioListToDownload; //Assign audio list to new Thread
			int ret = pthread_create(&audio_thread, NULL, parseListTh, NULL); //Create new thread to call itself
			if (ret != 0){
				cout << "Failed to create pthread" << endl;
				return "";
			}
		}
	}

	if(!isVod && !checkMediaList){ // Live implementation
		do {
			CURL *curl;
			CURLcode res;
			struct curlData newChunk;

			newChunk.memory = NULL;
			newChunk.size = 0;
			curl_global_init(CURL_GLOBAL_ALL);
			curl = curl_easy_init();
			res = downloadwithcurl(curl, newChunk, website);
			if(res != CURLE_OK){
				cout << "curl error: " << curl_easy_strerror(res) << endl;
				return 0;
			}
			string newMem = newChunk.memory;
			string newLine;
			istringstream newIss(newMem);
			const char *newSeq = strstr(newMem.c_str(), "#EXT-X-MEDIA-SEQUENCE:");
			sscanf(newSeq, "#EXT-X-MEDIA-SEQUENCE:%d\n", &newSequence);
		
			if (sequence == newSequence){ // playlist not update yet
				cout << "Playlist not modified yet. sleep for " << targetDuration/2 << " seconds" << endl;
				sleep(targetDuration/2);
			}else{
				//logFile_mutex.lock();
				while (getline(newIss, newLine)){
					if(newLine.compare(0, strlen("#EXTINF:"), "#EXTINF:") == 0){
		 				getline(newIss, playlistToDownload);
                		const char *line = playlistToDownload.c_str();
        		        const char * key = strstr(line, "#EXT-X-KEY");
		                const char * program = strstr(line, "#EXT-X-PROGRAM-DATE-TIME");
                		if(key || program){
                    		getline(newIss, playlistToDownload);
		                }else{
						// Create dir for video or audio
            		    // strippedWebsite:     file name
               		    // playlistToDownload: .ts address
		                // tsLink:              website to curl
						//cout << "playlistToDownload: " << playlistToDownload << endl;
        		        string tsLink = modifiedList(website, playlistToDownload);
						if(logForDownloaded.is_open()){
		               		bool c = createDirAndDownload(strippedWebsite.c_str(), tsLink, playlistToDownload, spO, logForDownloaded);
						}
						sequence = newSequence;
						break;
						}
					}
				}
				//logFile_mutex.unlock();
			}
		}while(1);
	}
	logForDownloaded.close();
	return playlistToDownload;
}

void *parseListTh(void * input)
{
    cout << "Audio Thread In" << endl;
    CURL *curl;
    CURLcode res;
    struct curlData tsData;
    string tsLink;
	
	//new speedTracking for audio list
	SpeedTracking * aSpO = new SpeedTracking();
    res = downloadwithcurl(curl, tsData, audioListDown);
    if(res != CURLE_OK){
        cout << "(Media Playlist) curl error: " << curl_easy_strerror(res) << endl;
        return NULL;
    }   
    tsLink = parseMasterList(tsData.memory, audioListDown, aSpO);
    return NULL;    
}

string stripForMedia(string website)
{
	size_t found = website.find_last_of("/") + 1;
    string temp;
    if(const char * str = strstr(website.c_str(), ".m3u8")){
        temp = website.substr(website.rfind("/")+1);
    }
    return temp;
}

string stripListForFile(string website)
{
	size_t found = website.find_last_of("/") + 1;
    size_t found2 = website.find_last_of(".");
	string temp;
	if(const char * str = strstr(website.c_str(), ".ts")){
		temp = website.substr(website.rfind("/")+1);
	}else{
   		if (found != string::npos && (found2 - found) > 1){
    		temp = website.substr(found, found2 - found);
		}
    }
	return temp;
}

string modifiedList(string masterList, string extension)
{
	string moList = masterList;
	if ((extension.rfind("http", 0) != 0) && ((extension.rfind("https", 0) != 0))){
		size_t found = masterList.find_last_of("/");
		if (found != string::npos){
			moList = moList.substr(0, found);
			moList = moList	+ "/" + extension;
		}
		return moList;
	}else{
		return extension;
	}
	return moList;
}

bool createDirAndDownload(const char *fileName, string tsLink, string playlistToDownload, SpeedTracking * spO, ofstream& logForDownloaded)
{
	int check;
	int check2;

	tsFile_mutex.lock();
	cout << "FFFFFFFFFFFFFFfileName: " << fileName << endl;
	check = mkdir(fileName, 0777);
	string temp = fileName;
	string container = "/var/www/html/jimmy/" + temp + "/";
	cout << "fileName: " << fileName << endl;
	cout << "container: " << container << endl;
	check2 = mkdir(container.c_str(), 0777);
	
	string cur = fileName;
	string nextDir = "./" + cur;
	string tsFileName = stripListForFile(playlistToDownload);
	if(!check){
		cout << "Folder created:  " << cur << endl;
		chdir(nextDir.c_str());
		// tsLink for curl
		// playlistToDownload for create the same name
		CURLcode res = downloadTs(tsLink, tsFileName, spO, logForDownloaded, fileName); 
		chdir("..");
		tsFile_mutex.unlock();
		return 1;
	}else{
		//Check if file exists
		chdir(nextDir.c_str());
		CURLcode res = downloadTs(tsLink, tsFileName, spO, logForDownloaded, fileName);
		chdir("..");
		tsFile_mutex.unlock();
		return 0;
	}
	tsFile_mutex.unlock();

	return 0;
}
