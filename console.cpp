#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <windows.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int hearbeat_interval = -1;
unsigned last_hearbeat_send = 0;
int sequence_number = -1;
bool hello_received = false;
bool identified = false;
const char* mybottoken="YOUR_TOKEN_HERE_";//about 66 length
std::string botId="";
void send_hearbeat(CURL *curl) {
    size_t sent;
    char buffer[512];
    sprintf(buffer, "{\"op\":%d, \"d\":%s}", 1, sequence_number == -1 ? "null" : std::to_string(sequence_number).c_str());
    CURLcode result = curl_ws_send(curl, (const void *)buffer, strlen(buffer), &sent, 0, CURLWS_TEXT);
    //printf("result %d, sent %d\n", result, sent);
    last_hearbeat_send = GetTickCount();
}

void send_identity(CURL *curl) {
    size_t sent;
    char buffer[4096];
    sprintf(buffer, "	\
    {	\
        \"op\":%d, \"d\": \
        {	\
            \"token\":\"%s\",	\
            \"intents\":%u,	\
            \"properties\":	\
            {	\
                \"os\": \"linux\",	\
                \"browser\": \"my_library\",	\
                \"device\": \"my_library\"	\
            }	\
        }	\
    }	\
    ", 2, mybottoken, 37377);
	//printf("sendbuffer is \n%s\n",buffer);
    //printf("sendbuffer len is %d\n", strlen(buffer));
    //CURLcode result = curl_ws_send(curl, (const void *)buffer, strlen(buffer), &sent, 0, CURLWS_TEXT);
    CURLcode result = curl_easy_send(curl, (const void *)buffer, strlen(buffer), &sent);
    printf("New identity interface. result is %d\n",result);
	//printf("Send identify. result %d, sent %d\n", result, sent);
}
void send_message(const std::string& channelID, const std::string& message) {
    size_t sent;
    char buffer[4096];
    sprintf(buffer,
    "	\
    {	\
        \"content\":\"%s\" \
    }	\
    ",
     message.c_str());
	CURL* easy_handle = curl_easy_init();
	
	if(easy_handle)
	{
	CURLM* multi_handle = curl_multi_init();
	if(multi_handle)
	{
		curl_multi_add_handle(multi_handle, easy_handle);
		std::string url = "https://discord.com/api/v10/channels/" + std::string(channelID) + "/messages";
		struct curl_slist* headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bot " + std::string(mybottoken)).c_str());
		curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());
		curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, buffer);
		curl_multi_remove_handle(multi_handle, easy_handle);
		//For time being
		curl_easy_perform(easy_handle);
	}
	//Clean the handles
	curl_easy_cleanup(easy_handle);
	curl_multi_cleanup(multi_handle);
	}
}


static int recv_any(CURL *curl) {
    size_t rlen;
    const struct curl_ws_frame *meta;
    char buffer[4096];
    CURLcode result = curl_ws_recv(curl, buffer, sizeof(buffer), &rlen, &meta);
	std::string data=std::string(buffer,rlen);
    if (result != CURLE_OK) {
        if (result != CURLE_AGAIN)
            printf("Error occurred in recv_any. Code: %d\n", result);
        return result;
    }

    if (meta->flags & CURLWS_TEXT) {
        //printf("received TEXT data of len %d\n",rlen);
		//printf("%s\n",data.c_str());
        try {
            json jsonData = json::parse(data);
			std::string jsonString = jsonData.dump(4);
			printf("%s\n",jsonString.c_str());
			if (jsonData.contains("op")) {
                int opcode = jsonData["op"];
				switch (opcode) {
                    case 10:
                        //printf("Received Hello (opcode 10)\n");
                        hello_received = true;

                        if (jsonData.contains("d") && jsonData["d"].contains("heartbeat_interval")) {
                            int heartbeat_interval = jsonData["d"]["heartbeat_interval"];
                           // printf("Heartbeat is %d\n", heartbeat_interval);

                            // Store heartbeat_interval in your global variable
                            hearbeat_interval = heartbeat_interval;
                            //printf("Heartbeat interval as an integer is %d\n", hearbeat_interval);
                            //printf("Sending heartbeat..\n");
                            send_hearbeat(curl);
                        }
                        break;
                    case 11:
                        //printf("Heartbeat ACK (opcode 11) received\n");
                        break;
                    case 1:
                        //printf("Heartbeat (opcode 1) received\n");
                        //printf("Sending heartbeat..\n");
                        send_hearbeat(curl);
                        break;
                    case 0:
                        // Handle Dispatch event (various event types)
                        if (jsonData.contains("t")) {
                            std::string eventType = jsonData["t"];

                            if (eventType == "MESSAGE_CREATE") {
                                // Handle the MESSAGE_CREATE event
                                if (jsonData.contains("d") && jsonData["d"].contains("content")) {
                                    std::string messageContent = jsonData["d"]["content"];
                                    std::string authorUsername = jsonData["d"]["author"]["username"];
                                    std::string authorid = jsonData["d"]["author"]["id"];
									if(authorid==botId)
										break;
									printf("Received a message from %s(%s): %s\n", authorUsername.c_str(), authorid.c_str(), messageContent.c_str());
									std::string channelID = jsonData["d"]["channel_id"];
                                    
                                    // Respond to the message
                                    std::string responseMessage = "Hello, I received your message!";
                                    send_message(channelID, responseMessage);
                                }
							}
                            else if(eventType == "READY"){
								//Handle the READY event
								if(jsonData.contains("d") && jsonData["d"].contains("user")){
									botId = jsonData["d"]["user"]["id"];
									printf("My ID is %s\n", botId.c_str());
								}
							}
							// Add more cases for other event types as needed.
						
                            
                        }
                        break;
                    default:
                        // Handle other opcodes or events
                        break;
                }
            }
        } catch (const json::parse_error& e) {
            fprintf(stderr, "JSON parsing error: %s\n", e.what());
            // Handle the parsing error as needed
        }
    } else if (meta->flags & CURLWS_CLOSE) {
        printf("Connection closed\n");
        exit(0);
    } else {
        printf("Other WebSocket flags or data\n");
    }

    return 0;
}

/* Close the connection */
static void websocket_close(CURL *curl) {
    size_t sent;
    (void)curl_ws_send(curl, "", 0, &sent, 0, CURLWS_CLOSE);
}

static void websocket(CURL *curl) {
    do {
        recv_any(curl);
        Sleep(2);
        if (hearbeat_interval != -1 && (GetTickCount() - last_hearbeat_send) > hearbeat_interval) {
            //printf("Heartbeat interval time reached. Sending heartbeat..\n");
            send_hearbeat(curl);
        }
        if (hello_received && !identified) {
            send_identity(curl);
            identified = true;
        }
    } while (true);

    websocket_close(curl);
}

int main(void) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "wss://gateway.discord.gg/?v=10&encoding=json");
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L); /* websocket style */

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        else {
            /* connected and ready */
            websocket(curl);
        }

        curl_easy_cleanup(curl);
    }

    return 0;
}