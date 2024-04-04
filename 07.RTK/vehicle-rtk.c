#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <pthread.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <curl/curl.h>
#include <wiringPi.h>

static int fd = 0;
#define MAX_BUF_SIZE 512
#define DEV_UART "/dev/ttyACM0"
//#define TRUE 1
//#define FALSE 0
#define uint8_t	unsigned char 
int httppost(char ch, float lat, float lon);
int httpget(float *x1, float *y1, float *x2, float *y2);

typedef struct _GPS_PARSE_DEF_{
    uint8_t		flagRead;        // flag used by the parser, when a valid sentence has begun
    uint8_t	    flagTIMEDataReady;   // valid UTC time data available, user can call reader functions
    uint8_t	    flagUTCDataReady;   // valid UTC date data available
    int8_t    	words[20][15];  // hold parsed words for one given NMEA sentence
    uint8_t    	szChecksum[15];	// hold the received checksum for one given NMEA sentence

    // will be set to true for characters between $ and * only
    uint8_t		flagComputedCks;     // used to compute checksum and indicate valid checksum interval (between $ and * in a given sentence)
    uint8_t     checksum;            // numeric checksum, computed for a given sentence
    uint8_t    	flagReceivedCks;     // after getting  * we start cuttings the received checksum
    uint8_t     index_received_checksum;// used to parse received checksum

    // word cutting variables
    uint8_t     wordIdx;         // the current word in a sentence
    uint8_t     prevIdx;		// last character index where we did a cut
    uint8_t     nowIdx ;		// current character index

    // globals to store parser results
    float   	longitude;     // GPRMC and GPGGA
    float   	latitude;	// GPRMC and GPGGA
    uint8_t   	UTCHour, UTCMin, UTCSec, UTCDay, UTCMonth, UTCYear;
    uint8_t		flagDetect;		// flag NMEA sentence
}GPS_ParseTypeDef;

GPS_ParseTypeDef parser;

int init_uart(char * dev, int baud, int * fd)
{
    struct termios newtio;
    * fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if ( * fd < 0) {
        printf("%s> uart dev '%s' open fail [%d]n", __func__, dev, * fd);
        return -1;
    }
    memset( & newtio, 0, sizeof(newtio));
    newtio.c_iflag = IGNPAR; // non-parity 
    newtio.c_oflag = OPOST | ONLCR;;
    newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts 
    switch (baud)
    {
    case 115200:
      newtio.c_cflag |= B115200;
      break;
    case 57600:
      newtio.c_cflag |= B57600;
      break;
    case 38400:
      newtio.c_cflag |= B38400;
      break;
    case 19200:
      newtio.c_cflag |= B19200;
      break;
    case 9600:
      newtio.c_cflag |= B9600;
      break;
    case 4800:
      newtio.c_cflag |= B4800;
      break;
    case 2400:
      newtio.c_cflag |= B2400;
      break;
    default:
      newtio.c_cflag |= B115200;
      break;
    }

    newtio.c_lflag = 0;
    //newtio.c_cc[VTIME] = vtime; // timeout 0.1초 단위 
    //newtio.c_cc[VMIN] = vmin; // 최소 n 문자 받을 때까진 대기 
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush( * fd, TCIFLUSH);
    tcsetattr( * fd, TCSANOW, & newtio);
    return 0;
}

void *test_read_loop(void * arg)
{
    int result;
    char buffer[MAX_BUF_SIZE];
    fd_set reads, temps;

    FD_ZERO( & reads);
    FD_SET(fd, & reads);

    while (1)
    {
        temps = reads;
        result = select(FD_SETSIZE, & temps, NULL, NULL, NULL);

        if (result < 0){
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(fd, & temps)){
            memset(buffer, 0, sizeof(buffer));
            
            if (read(fd, buffer, MAX_BUF_SIZE) == -1){
                continue;
            }
            
            printf("%s", buffer);
        }
    }
}
int charToInt(char c)
{
	return (c - 48);
}

char int2char(char ch)
{
  if( 0<= ch || ch <= 9){
    return ch + 0x30;
  }
  else{
    return ch + 0x31;
  }
}

float prvlat = 0.0, prvlon = 0.0;

static float haversine(float lat1, float lon1, float lat2, float lon2)
{
    // distance between latitudes
    // and longitudes
    float dLat = (lat2 - lat1) *  M_PI / 180.0;
    float dLon = (lon2 - lon1) *  M_PI / 180.0;

    // convert to radians
    lat1 = (lat1) * M_PI / 180.0;
    lat2 = (lat2) * M_PI / 180.0;

    // apply formulae
    float a = pow(sin(dLat / 2), 2) +
	       pow(sin(dLon / 2), 2) *
	       cos(lat1) * cos(lat2);
    float rad = 6371000;
    float c = 2 * asin(sqrt(a));
    
    printf("diff: %f\r\n", rad * c);
    
    return rad * c;
}
void searching(char ch)
{
    char upper, lower;

    upper = int2char((parser.checksum & 0xF0)>>4);
    lower = int2char((parser.checksum & 0x0F)>>0);

    if ((upper != parser.szChecksum[0]) && (lower != parser.szChecksum[1])) {
	return;
    }

    if (strcmp((char*)&parser.words[0], "$GPGGA") == 0) {
	parser.flagDetect = TRUE;
	// Check GPS Fix: 0=no fix, 1=GPS fix, 2=Dif. GPS fix
	if (parser.words[6][0] == '0') {
	    //_CLR_FLAG_(status, GPS_FIX_FLAG);
	    parser.flagTIMEDataReady = FALSE;
	    return;
	}
	else {
	    // parse latitude and longitude in NMEA format
	    parser.latitude = strtof((char*)&parser.words[2], NULL);
	    parser.longitude = strtof((char*)&parser.words[4], NULL);

	    // get decimal format
	    if (parser.words[3][0] == 'S') parser.latitude  *= -1.0;
	    if (parser.words[5][0] == 'W') parser.longitude *= -1.0;
	    float degrees = trunc(parser.latitude / 100.0f);
	    float minutes = parser.latitude - (degrees * 100.0f);
	    parser.latitude = degrees + minutes / 60.0f;

	    degrees = trunc(parser.longitude / 100.0f);
	    minutes = parser.longitude - (degrees * 100.0f);
	    parser.longitude = degrees + minutes / 60.0f;

	    // data ready
	    parser.flagTIMEDataReady = TRUE;
	    
	    //printf("lat = %f, lon = %f\r\n", parser.latitude, parser.longitude);
	    
	    //haversine(parser.latitude, parser.longitude, prvlat, prvlon);
	    //httppost(ch, parser.latitude, parser.longitude);
	    
	    //prvlat = parser.latitude;
	    //prvlon = parser.longitude;
	    
	}

	// parse time
	parser.UTCHour = charToInt(parser.words[1][0]) * 10 + charToInt(parser.words[1][1]);
	parser.UTCMin = charToInt(parser.words[1][2]) * 10 + charToInt(parser.words[1][3]);
	parser.UTCSec = charToInt(parser.words[1][4]) * 10 + charToInt(parser.words[1][5]);

	if(parser.flagTIMEDataReady == TRUE && parser.flagUTCDataReady == TRUE){
	    //_SET_FLAG_(status, GPS_FIX_FLAG);
	    //printf("%d:%d:%d\r\n", parser.UTCHour, parser.UTCMin, parser.UTCSec);
	}
    }//$GPGGA

    if (strcmp((char*)&parser.words[0], "$GPRMC") == 0) {
	parser.flagDetect = TRUE;

	// Check data status: A-ok, V-invalid
	if ( parser.words[2][0] == 'V') {
	     parser.flagUTCDataReady = FALSE;
	    return;
	}

	// parse UTC date
	 parser.UTCDay = charToInt( parser.words[9][0]) * 10 + charToInt( parser.words[9][1]);
	 parser.UTCMonth = charToInt( parser.words[9][2]) * 10 + charToInt( parser.words[9][3]);
	 parser.UTCYear = charToInt( parser.words[9][4]) * 10 + charToInt( parser.words[9][5]);

	// data ready
	 parser.flagUTCDataReady = TRUE;
	 //printf("%d-%d-%d\r\n", parser.UTCDay, parser.UTCMonth, parser.UTCYear);
    }//$GPRMC
}
void parsing(uint8_t ch, char channel)
{
    if (ch == '$'){
	parser.flagRead = TRUE;
	// init parser vars
	parser.flagComputedCks = FALSE;
	parser.checksum = 0;
	// after getting  * we start cuttings the received m_nChecksum
	parser.flagReceivedCks = FALSE;
	parser.index_received_checksum = 0;
	// word cutting variables
	parser.wordIdx = 0; parser.prevIdx = 0; parser.nowIdx = 0;
    }

    if (parser.flagRead) {
	// check ending
	if (ch == '\r' || ch == '\n') {
		// catch last ending item too
		parser.words[parser.wordIdx][parser.nowIdx - parser.prevIdx] = 0;
		parser.wordIdx++;
		// cut received m_nChecksum
		parser.szChecksum[parser.index_received_checksum] = 0;
		// sentence complete, read done
		parser.flagRead = FALSE;
		// parse
		//GPS_setDrvParam(&GPS, GPS_SET_PARSER, 0);
		searching(channel);
		
		
		
	} else {
		// computed m_nChecksum logic: count all chars between $ and * exclusively
		if (parser.flagComputedCks && ch == '*') parser.flagComputedCks = FALSE;
		if (parser.flagComputedCks) parser.checksum ^= ch;
		if (ch == '$') parser.flagComputedCks = TRUE;
		// received m_nChecksum
		if (parser.flagReceivedCks){
		    parser.szChecksum[parser.index_received_checksum] = ch;
		    parser.index_received_checksum++;
		}
		
		if (ch == '*'){
		    parser.flagReceivedCks = TRUE;
		}

		// build a word
		parser.words[parser.wordIdx][parser.nowIdx - parser.prevIdx] = ch;
		if (ch == ',') {
		    parser.words[parser.wordIdx][parser.nowIdx - parser.prevIdx] = 0;
		    parser.wordIdx++;
		    parser.prevIdx = parser.nowIdx;
		}
		else {
		    parser.nowIdx++;
		}
	}
    }    
}

void httpprint(char *str)
{
    printf("%s",str);
}

int httppost(char ch, float lat, float lon)
{
    CURL *curl;
    CURLcode res;
    char temp[64] = {0,};
#if 0
    httpprint("{\r\n    ");
    sprintf(temp, "\"lat2\":\"%f\",\r\n    ", lat);
    httpprint(temp);
    memset(temp, 0, 64);
    sprintf(temp, "\"lon2\":\"%f\"\r\n}", lon);
    httpprint(temp);
#else    
    curl = curl_easy_init();
    1
    if(curl) {
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	if(ch==1){
	    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.125:5000/pos1");
	}
	else{
	    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.125:5000/pos2");
	}
	
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	//const char *data = "{\r\n    \"lat2\":\"1234.5678\",\r\n    \"lon2\":\"1234.5678\"\r\n}";
	sprintf(temp, "{\r\n    \"lat\":\"%f\",\r\n    \"lon\":\"%f\"\r\n}", lat,lon);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, temp);
	res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);
#endif
}

struct memory {
   char *response;
   size_t size;
 };
 
static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
 {
   size_t realsize = size * nmemb;
   struct memory *mem = (struct memory *)userp;
 
   char *ptr = realloc(mem->response, mem->size + realsize + 1);
   if(ptr == NULL)
     return 0;  /* out of memory! */
 
   mem->response = ptr;
   memcpy(&(mem->response[mem->size]), data, realsize);
   mem->size += realsize;
   mem->response[mem->size] = 0;
 
   return realsize;
 }

int httpget(float *x1, float *y1, float *x2, float *y2)
{
    CURL *curl;
    CURLcode res;
    struct memory chunk = {0};
    int ret = 0;
    
    curl = curl_easy_init();

    if(curl) {
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
	
	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.125:5000/getpos");
	
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

	/* send all data to this function  */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
	 
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	
	struct curl_slist *headers = NULL;
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	res = curl_easy_perform(curl);
	
	if(CURLE_OK == res) {
	    //char *ct;
	    long response_code;
	    /* ask for the content-type */
	    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
 
	    if((CURLE_OK == res)){
		printf("[%ld]\r\n", response_code);
		printf("%s, %d \r\n", chunk.response, chunk.size);

		char *addr = NULL;
		char comma[4] = {0,}, buffer[16] = {0,};

		for(int i=0,j=0; i<chunk.size; i++){
		    if(chunk.response[i] == ','){
			comma[j] = i;
			j++;
		    }
		}
		addr = strstr(chunk.response, "@");
		if(addr != NULL){
		    memcpy(buffer, (addr+1), comma[0]-1);
		    //printf("%s,%f\r\n", buffer, atof(buffer));
		    *x1 = atof(buffer);
		    memset(buffer, 0, 16);
		    memcpy(buffer, (addr+comma[0]+1), comma[1]-comma[0]-1);
		    printf("%s,%f\r\n", buffer, atof(buffer));
		    *y1 = atof(buffer);
		    memset(buffer, 0, 16);
		    memcpy(buffer, (addr+comma[1]+1), comma[2]-comma[1]-1);
		    //printf("%s,%f\r\n", buffer, atof(buffer));
		    *x2 = atof(buffer);
		    memset(buffer, 0, 16);
		    memcpy(buffer, (addr+comma[2]+1), comma[3]-comma[2]-1);
		    //printf("%s,%f\r\n", buffer, atof(buffer));
		    *y2 = atof(buffer);
	    
		    ret = 1;
		    
		}
	    }
	}
    }
    curl_easy_cleanup(curl);

    return ret;
}

#define BUZZER 17
//#defien ABS(x) x<0?-(x):(x)

int main()
{
    float x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0, diff1 = 0.0, diff2 = 0.0;
    int result, ret = -1, i = 0;
    char toggle = 0;
    
    ret = init_uart(DEV_UART, 115200, & fd);

    printf("init uart [%d], [%d]\r\n", ret, fd);


    char buffer[MAX_BUF_SIZE], ch = 0;
    fd_set reads, temps;

    FD_ZERO( & reads);
    FD_SET(fd, & reads);

    if(wiringPiSetupGpio() <0){
	printf("Unabe to setup wiringPi\r\n");
	return 0;
    }
    
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, 0);
   /* 
    while(1)
    {
	digitalWrite(BUZZER, 1);
	printf("0\r\n");
	usleep(500000);
	digitalWrite(BUZZER, 0);
	printf("1\r\n");
	usleep(500000);
    }
    */
    
    while (1)
    {
        temps = reads;
        result = select(FD_SETSIZE, & temps, NULL, NULL, NULL);

        if (result < 0){
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(fd, & temps)){
            memset(buffer, 0, sizeof(buffer));
            
            if (read(fd, buffer, MAX_BUF_SIZE) == -1){
                continue;
            }
            
	    for(i = 0; i<strlen(buffer); i++){
		parsing(buffer[i], ch);
	    }
        }
	
	ret = httpget(&x1, &y1, &x2, &y2);
	
	if(parser.flagTIMEDataReady == TRUE && ret == TRUE){
	    printf("lat = %f, lon = %f\r\n", parser.latitude, parser.longitude);
	    printf("x1 = %f, y1 = %f, x2 = %f, y2 = %f\r\n", x1, y1, x2, y2);
	    
	    diff1 = haversine(parser.latitude, parser.longitude, x1, y1);
	    
	    diff2 = haversine(parser.latitude, parser.longitude, x2, y2);
	    
	    printf("diff1= %f, diff2=%f \r\n", diff1, diff2);
	    
	    
	    if(abs(diff1-diff2) < 100){
		digitalWrite(BUZZER, 1);
	    }
	    else if(diff1 < 500 || diff2 <500){
		toggle ^= 1;
		digitalWrite(BUZZER, toggle);
	    }
	    else{
		digitalWrite(BUZZER, 0);
	    }
	}
	
        sleep(1);
    }
}
