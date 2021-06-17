
#include <signal.h>
#include <iostream>
#include <thread>
#include <sstream>
#include <mutex>
#include <fstream>

#include "../include/ggk.h"
// meetpie specific
#include "../include/meetpie.h"


// Maximum time to wait for any single async process to timeout during initialization
static const int kMaxAsyncInitTimeoutMS = 30 * 1000;
//mutex so that we can multi-thread
std::mutex mutex_buffer;
// The battery level ("battery/level") reported by the server (see Server.cpp)
static uint8_t serverDataBatteryLevel = 100;
// The text string ("text/string") used by our custom text string service (see Server.cpp)
static std::string serverDataTextString;

//
// Logging removed by sd
//

//
// Signal handling
//

// We setup a couple Unix signals to perform graceful shutdown in the case of SIGTERM or get an SIGING (CTRL-C)
void signalHandler(int signum)
{
	switch (signum)
	{
	case SIGINT:
		// sd need to put code in here to free up any memeory and also close file socket
		ggkTriggerShutdown();
		break;
	case SIGTERM:
		ggkTriggerShutdown();
		break;
	}
}

//
// Server data management
//

// Called by the server when it wants to retrieve a named value
//
// This method conforms to `GGKServerDataGetter` and is passed to the server via our call to `ggkStart()`.
//
// The server calls this method from its own thread, so we must ensure our implementation is thread-safe. In our case, we're simply
// sending over stored values, so we don't need to take any additional steps to ensure thread-safety.
const void *dataGetter(const char *pName)
{

	if (nullptr == pName)
	{
		return nullptr;
	}

	std::string strName = pName;

	if (strName == "battery/level")
	{
		return &serverDataBatteryLevel;
	}
	else if (strName == "text/string")
	{
//		std::string serverOutputString;

//		mutex_buffer.lock();
		printf ("received read\n");
//		serverOutputString = serverDataTextString;

//		mutex_buffer.unlock();

//		return serverOutputString.c_str();
		return serverDataTextString.c_str();
	}

	return nullptr;
}

// Called by the server when it wants to update a named value
//
// This method conforms to `GGKServerDataSetter` and is passed to the server via our call to `ggkStart()`.
//
// The server calls this method from its own thread, so we must ensure our implementation is thread-safe. In our case, we're simply
// sending over stored values, so we don't need to take any additional steps to ensure thread-safety.
int dataSetter(const char *pName, const void *pData)
{

	if (nullptr == pName)
	{
		return 0;
	}
	if (nullptr == pData)
	{
		return 0;
	}

	std::string strName = pName;

	if (strName == "battery/level")
	{
		serverDataBatteryLevel = *static_cast<const uint8_t *>(pData);
		return 1;
	}
	else if (strName == "text/string")
	{
		printf("called write\n");
		//		need to change this below as will no lnger work
		//		serverDataTextString = static_cast<const char *>(pData);
		return 1;
	}


	return 0;
}

//
// Entry point
//

int main(int argc, char **ppArgv)
{

	// A basic command-line parser

	// Setup our signal handlers
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	// Register our loggers

	//SD reserve space for json string to improve performance
	serverDataTextString.reserve(MAXLINE);
	
	//SD
	//Create and bind UDP socket to get data from odas server


	// Start the server's ascync processing
	//
	// This starts the server on a thread and begins the initialization process
	//
	// !!!IMPORTANT!!!
	//
	//     This first parameter (the service name) must match tha name configured in the D-Bus permissions. See the Readme.md file
	//     for more information.
	//

	if (!ggkStart("gobbledegook", "Gobbledegook", "Gobbledegook", dataGetter, dataSetter, kMaxAsyncInitTimeoutMS))
	{
	printf ("error starting ggk\n");
	return -1;
	}

	// Wait for the server to start the shutdown process
	//
	// While we wait, every 15 ticks, drop the battery level by one percent until we reach 0

	int j = 1;

	while (ggkGetServerRunState() < EStopping)
	{

		//this is main polling loop for getting data from odas via UDP receive
		//it processes this data and then updates the bluetooth characteristic with new data
			sleep (8);
			printf ("woken up\n");
			mutex_buffer.lock();

		// is this needed ?
			serverDataTextString = "{\"tMT\": ";
			serverDataTextString += std::to_string(j++);
			serverDataTextString += ",\n\"m\": [\n";


			for (int i = 1; i <= j && i < 7; i++)
			{
				serverDataTextString += "{\"mN\": ";
				serverDataTextString += std::to_string(i);
				serverDataTextString += ",\"a\": ";
				serverDataTextString += std::to_string(i);
				serverDataTextString += ",\"t\": ";
				serverDataTextString += std::to_string(i);
				serverDataTextString += ",\"nT\": ";
				serverDataTextString += std::to_string(i);
				serverDataTextString += ",\"f\": ";
				serverDataTextString += std::to_string(i);
				serverDataTextString += ",\"tT\": ";
				serverDataTextString += std::to_string(i);
				serverDataTextString += "}";


				if (i < j && i < 7)
				{
					serverDataTextString += ",";
				}
			}

			serverDataTextString += "]}\n";

			mutex_buffer.unlock();

		    	printf ("%s\n",serverDataTextString.c_str());

		// now the output string is ready and we should call notify
			ggkNofifyUpdatedCharacteristic("/com/gobbledegook/text/string");

		
	}

	// Wait for the server to come to a complete stop (CTRL-C from the command line)
	if (!ggkWait())
	{
		
		return -1;
	}

	// Return the final server health status as a success (0) or error (-1)
	return ggkGetServerHealth() == EOk ? 0 : 1;
}
