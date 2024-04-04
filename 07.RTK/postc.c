#include <stdio.h>
#include <curl/curl.h>

int main()
{
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if(curl) {
	  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	  curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.115:5000/pos2");
	  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	  curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
	  struct curl_slist *headers = NULL;
	  headers = curl_slist_append(headers, "Content-Type: application/json");
	  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	  const char *data = "{\r\n    \"lat2\":\"1234.5678\",\r\n    \"lon2\":\"1234.5678\"\r\n}";
	  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
	  res = curl_easy_perform(curl);
	}
	curl_easy_cleanup(curl);
}
