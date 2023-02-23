#include "curlDownload.h"
using namespace std;

SpeedTracking * tempS;
int sum = 0;
string FormatBytes(unsigned long long i, string unit, int decimal);

static size_t WriterList(char *contents, size_t size, size_t nmemb, void *userp)
{
    size_t real_size = size * nmemb;
    struct curlData *mem = (struct curlData *)userp;
    char *ptr = (char *)realloc(mem->memory, mem->size + real_size + 1);
    if(ptr == NULL)
        return 0;
    mem->memory = ptr;
    memcpy(&mem->memory[mem->size], contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;
	//sum += real_size;
    return real_size;
}

static size_t WriterTs(char *contents, size_t size, size_t nmemb, void *userp)
{
    size_t real_size = size * nmemb;
/*
	struct curlData *mem = (struct curlData *)userp;
    char *ptr = (char *)realloc(mem->memory, mem->size + real_size + 1); 
    if(ptr == NULL)
        return 0;

	mem->memory = ptr;
    memcpy(&mem->memory[mem->size], contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;
*/
		
	tempS->addDownloadedSize(size*nmemb);

    return fwrite(contents, size, nmemb, (FILE*)userp);
}

CURLcode downloadTs(string webForDownload, string fileName, SpeedTracking *spO, ofstream& resultLog, string container)
{
	CURL *curl;
	CURLcode res;
	double tsSpeed = 0.0;
	double tsSize = 0.0;
	double totalTime = 0.0;
	struct curlData newChunk;
	FILE *fp;
	string locationForAllTs = "/var/www/html/jimmy/m3u8Downloader/" + container;
	const char *p = locationForAllTs.c_str();
	ofstream file(p);
	//strip ts file in case of \r \n at the end
	fileName = regex_replace(fileName,regex("\\r\\n|\\r|\\n"),"");
	fp = fopen(fileName.c_str(), "wb");
	curl = curl_easy_init();

	//stripping special char. Avoid curl error bad url
	string newLink = webForDownload;
	newLink = regex_replace(newLink,regex("\\r\\n|\\r|\\n"),"");
		
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, newLink.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriterTs);
		tempS = spO;
		//curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

		spO->downloadStart();
		res = curl_easy_perform(curl);
		spO->downloadEnd();

		curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &tsSpeed);
		curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &tsSize);
		curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &totalTime);
		if(res != CURLE_OK){
			cout << "Failed to download: " << webForDownload << endl;
			cout << "Ts curl_easy_strerror() return " << curl_easy_strerror(res) << endl;
			return res;
		}
	}
	cout << endl;
	cout << "--------------- Curl Ts Info ---------------" << endl;
	//cout << "Successfully downloaded." << endl;
	cout << webForDownload << endl;
	//cout << endl;
	cout << "Downloaded size = " << fixed << setprecision(3) << tsSize << " Bytes" << endl;
	//FormatBytes convert byte -> bits
	cout << "Download speed = " << FormatBytes(tsSpeed*8, "bps", 1).c_str() << " or " << tsSpeed << " bytes per second" << endl;
	cout << "Download time = " << totalTime << "s" << endl;
//	cout << "--------------- End ---------------" << endl;
/*
	cout << endl;
	resultLog << endl;
	resultLog << "--------------- Curl Ts Info ---------------" << endl;
	resultLog << webForDownload << endl;
	resultLog << "Downloaded size = " << fixed << setprecision(3) << tsSize << " Bytes" << endl;
	resultLog << "Download speed = " << FormatBytes(tsSpeed*8, "bps", 1).c_str() << " or " << tsSpeed << " bytes per second" << endl;
	resultLog << "Download time = " << totalTime << "s" << endl;
	resultLog << endl;
*/
	fclose(fp);
	file.close();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return res;
}

size_t write_media(void *ptr, size_t size, size_t nmemb, void *userp) {
	return fwrite(ptr, size, nmemb, (FILE*)userp);
}

CURLcode downloadMediaList(string path, string website, string m3u8Container)
{
	CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
	
	m3u8Container = regex_replace(m3u8Container,regex("\\r\\n|\\r|\\n"),"");
	path = regex_replace(path,regex("\\r\\n|\\r|\\n"),"");

	//Create a container for media playlist m3u8
	const char *container = m3u8Container.c_str();
	cout << "m3u8Container: " << container << endl;
	ofstream fileContainer(container);
	fileContainer.close();
	
	const char *p = path.c_str();
	ofstream file(p);
	cout << "path: " << path << endl;
	FILE *fp;
	fp = fopen(path.c_str(), "wb");
    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, website.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_media);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            cout << "curl_easy_perform() return " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }
	fclose(fp);
	file.close();
    curl_global_cleanup();
    return res;
}

CURLcode downloadwithcurl(CURL *curl, struct curlData &chunk, string website)
{
    CURLcode res;
	double avspeed = 0;
	double dsize = 0;
    chunk.memory = NULL;
    chunk.size = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, website.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriterList);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

        res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &avspeed);
		curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &dsize);

		sum = 0;
        if(res != CURLE_OK){
            cout << "curl_easy_perform() return " << curl_easy_strerror(res) << endl;
        }else{
            char *extm3u = strstr(chunk.memory, "#EXTM3U");
            if(!extm3u){
                cout << "It isn't HLS." << endl;
            }
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
	return res;
}

string FormatBytes(unsigned long long i, string unit, int decimal)
{
    stringstream ss;
    if (i >= (1ll<<40))
        ss << setprecision(decimal) << std::fixed << (double) i / (1ll<<40) << "G" << unit;
    else if (i >= (1ll<<20))
        ss << setprecision(decimal) << std::fixed << (double) i / (1ll<<20) << "M" << unit;
    else if (i >= (1ll<<10))
        ss << setprecision(decimal) << std::fixed << (double) i / (1ll<<10) << "K" << unit;
    else
        ss << i << unit;
    return ss.str();
}
