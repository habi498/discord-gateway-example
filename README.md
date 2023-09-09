# discord-gateway-example
How to connect to discord gateway and receive messages in C++/C using Curl websockets
# How to compile in Windows with MSVC
1. Open X64 Native Tools Command Prompt
2. cd to the directory containing console.cpp
3. cl -DCURL_STATICLIB  console.cpp libcurl-x64.lib   -I . ws2_32.lib crypt32.lib wldap32.lib advapi32.lib /MD
4. This will create console.exe
# What this example do?
1. This example console.exe connects to discord-gateway ( You can see the bot-becoming online ) and listen to user messages. If somebody types something in a channel in which the bot belongs, it will respond by Hello, I recieved your message.

I originally made for a plugin for game server, but since example in c++ to connect to discord gateway is rare online, decided to upload it.
