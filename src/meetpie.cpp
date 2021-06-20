//         While the server is running, you will likely need to update the data being served. This is done by calling
//         `ggkNofifyUpdatedCharacteristic()` or `ggkNofifyUpdatedDescriptor()` with the full path to the characteristic or delegate
//         whose data has been updated. This will trigger your server's `onUpdatedValue()` method, which can perform whatever
//         actions are needed such as sending out a change notification (or in BlueZ parlance, a "PropertiesChanged" signal.)
//
// * A stand-alone application SHOULD:
//
//     * Shutdown the server before termination
//
//         Triggering the server to begin shutting down is done via a call to `ggkTriggerShutdown()`. This is a non-blocking method
//         that begins the asynchronous shutdown process.
//
//         Before your application terminates, it should wait for the server to be completely stopped. This is done via a call to
//         `ggkWait()`. If the server has not yet reached the `EStopped` state when `ggkWait()` is called, it will block until the
//         server has done so.
//
//         To avoid the blocking behavior of `ggkWait()`, ensure that the server has stopped before calling it. This can be done
//         by ensuring `ggkGetServerRunState() == EStopped`. Even if the server has stopped, it is recommended to call `ggkWait()`
//         to ensure the server has cleaned up all threads and other internals.
//
//         If you want to keep things simple, there is a method `ggkShutdownAndWait()` which will trigger the shutdown and then
//         block until the server has stopped.
//
//     * Implement signal handling to provide a clean shut-down
//
//         This is done by calling `ggkTriggerShutdown()` from any signal received that can terminate your application. For an
//         example of this, search for all occurrences of the string "signalHandler" in the code below.
//
//     * Register a custom logging mechanism with the server
//
//         This is done by calling each of the log registeration methods:
//
//             `ggkLogRegisterDebug()`
//             `ggkLogRegisterInfo()`
//             `ggkLogRegisterStatus()`
//             `ggkLogRegisterWarn()`
//             `ggkLogRegisterError()`
//             `ggkLogRegisterFatal()`
//             `ggkLogRegisterAlways()`
//             `ggkLogRegisterTrace()`
//
//         Each registration method manages a different log level. For a full description of these levels, see the header comment
//         in Logger.cpp.
//
//         The code below includes a simple logging mechanism that logs to stdout and filters logs based on a few command-line
//         options to specify the level of verbosity.
//

#include <signal.h>
#include <iostream>
#include <thread>
#include <sstream>
#include <mutex>
#include <fstream>

#include "../include/ggk.h"
// meetpie specific
#include "../include/meetpie.h"
#include "../include/json_parsing.h"


// Maximum time to wait for any single async process to timeout during initialization
static const int kMaxAsyncInitTimeoutMS = 30 * 1000;

//mutex so that we can multi-thread
std::mutex mutex_buffer;

// The battery level ("battery/level") reported by the server (see Server.cpp)
static uint8_t serverDataBatteryLevel = 100;

// The text string ("text/string") used by our custom text string service (see Server.cpp)
static std::string serverDataTextString;

//
// Logging
//

enum LogLevel
{
	Debug,
	Verbose,
	Normal,
	ErrorsOnly
};

// Our log level - defaulted to 'Normal' but can be modified
LogLevel logLevel = Normal;

// Our full set of logging methods (we just log to stdout)
//
// NOTE: Some methods will only log if the appropriate `logLevel` is set
void LogDebug(const char *pText)
{
	if (logLevel <= Debug)
	{
		std::cout << "  DEBUG: " << pText << std::endl;
	}
}
void LogInfo(const char *pText)
{
	if (logLevel <= Verbose)
	{
		std::cout << "   INFO: " << pText << std::endl;
	}
}
void LogStatus(const char *pText)
{
	if (logLevel <= Normal)
	{
		std::cout << " STATUS: " << pText << std::endl;
	}
}
void LogWarn(const char *pText) { std::cout << "WARNING: " << pText << std::endl; }
void LogError(const char *pText) { std::cout << "!!ERROR: " << pText << std::endl; }
void LogFatal(const char *pText) { std::cout << "**FATAL: " << pText << std::endl; }
void LogAlways(const char *pText) { std::cout << "..Log..: " << pText << std::endl; }
void LogTrace(const char *pText) { std::cout << "-Trace-: " << pText << std::endl; }

//
// Signal handling
//

// We setup a couple Unix signals to perform graceful shutdown in the case of SIGTERM or get an SIGING (CTRL-C)
void signalHandler(int signum)
{
	switch (signum)
	{
	case SIGINT:
		LogStatus("SIGINT recieved, shutting down");
		// sd need to put code in here to free up any memeory and also close file socket
		ggkTriggerShutdown();
		break;
	case SIGTERM:
		LogStatus("SIGTERM recieved, shutting down");
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
		LogError("NULL name sent to server data getter");
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

//		serverOutputString = serverDataTextString;

//		mutex_buffer.unlock();

		return serverDataTextString.c_str();
	}

	LogWarn((std::string("Unknown name for server data getter request: '") + pName + "'").c_str());
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
		LogError("NULL name sent to server data setter");
		return 0;
	}
	if (nullptr == pData)
	{
		LogError("NULL pData sent to server data setter");
		return 0;
	}

	std::string strName = pName;

	if (strName == "battery/level")
	{
		serverDataBatteryLevel = *static_cast<const uint8_t *>(pData);
		LogDebug((std::string("Server data: battery level set to ") + std::to_string(serverDataBatteryLevel)).c_str());
		return 1;
	}
	else if (strName == "text/string")
	{
		//		need to change this below as will no lnger work
		//		serverDataTextString = static_cast<const char *>(pData);
		LogDebug((std::string("Server data: text string set to '") + serverDataTextString + "'").c_str());
		return 1;
	}

	LogWarn((std::string("Unknown name for server data setter request: '") + pName + "'").c_str());

	return 0;
}

//  the following three functions are called whn we have UDP data
// I will build them into seprate files eventually

void process_sound_data(meeting *meeting_data, participant_data *participant_data_array, odas_data *odas_data_array)

{
	int target_angle;
	int iChannel, iAngle;

	meeting_data->total_meeting_time++;

	for (iChannel = 0; iChannel < NUM_CHANNELS; iChannel++)
	{
		//  dont use energy to check if track is active otherwise you miss the ending of the speech and
		//  participant talking is never set to false
		if (odas_data_array[iChannel].x != 0.0 && odas_data_array[iChannel].y != 0.0)
		{
			meeting_data->total_silence = 0;
			target_angle = 180 - (atan2(odas_data_array[iChannel].x, odas_data_array[iChannel].y) * 57.3);

			printf("target angle %d\n", target_angle);

			// angle_array holds a int for every angle position.  Once an angle is set to true a person is registered there
			// so if tracked source is picked up we check to see if it is coming from a known participant
			// if it is not yet known then we also check that we havent reached max particpants before trying to add a new one

			//max_num_participants -1 so that we dont go out of bounds - means 0 is never used so will need to optimise

			if (meeting_data->angle_array[target_angle] == 0x00 && meeting_data->num_participants < (MAXPART - 1))
			{
				meeting_data->num_participants++;

				meeting_data->angle_array[target_angle] = meeting_data->num_participants;
				//                printf ("new person %d\n", num_participants);
				participant_data_array[meeting_data->num_participants].participant_angle = target_angle;

				// set intial frequency high
				participant_data_array[meeting_data->num_participants].participant_frequency = 200.0;
				// write a buffer around them

				for (iAngle = 1; iAngle < ANGLE_SPREAD; iAngle++)
				{
					if (target_angle + iAngle < 360)
					{
						// could check if already set here - but for now will just overwrite
						// 360 is for going round the clock face

						meeting_data->angle_array[target_angle + iAngle] = meeting_data->num_participants;
					}
					else
					{
						meeting_data->angle_array[iAngle - 1] = meeting_data->num_participants;
					}
					if (target_angle - iAngle >= 0)
					{
						// could check if already set here - but for now will just overwrite
						// 360 is for going round the clock face
						meeting_data->angle_array[target_angle - iAngle] = meeting_data->num_participants;
					}
					else
					{
						meeting_data->angle_array[361 - iAngle] = meeting_data->num_participants;
					}
				}

				participant_data_array[meeting_data->num_participants].participant_is_talking = iChannel;
			}
			else // its an existing talker we're hearing
			{
				// could put logic in here to count turns
				participant_data_array[meeting_data->angle_array[target_angle]].participant_is_talking = 10 * odas_data_array[iChannel].activity;
				participant_data_array[meeting_data->angle_array[target_angle]].participant_total_talk_time++;

				if (odas_data_array[iChannel].frequency > 0.0)
				{
					participant_data_array[meeting_data->angle_array[target_angle]].participant_frequency = (0.9 * participant_data_array[meeting_data->angle_array[target_angle]].participant_frequency) + (0.1 * odas_data_array[iChannel].frequency);
				}
			}
		}
		else
		{
			meeting_data->total_silence++;
		}
	}
}

void initialise_meeting_data(meeting *meeting_data, participant_data *participant_data_array, odas_data *odas_data_array)
{

	int i;

	// initialise meeting array
	for (i = 0; i < MAXPART; i++)
	{
		participant_data_array[i].participant_angle = 0;
		participant_data_array[i].participant_is_talking = 0;
		participant_data_array[i].participant_silent_time = 0;
		participant_data_array[i].participant_total_talk_time = 0;
		participant_data_array[i].participant_num_turns = 0;
		participant_data_array[i].participant_frequency = 150.0;
	}

	for (i = 0; i < NUM_CHANNELS; i++)
	{
		odas_data_array[i].x = 0.0;
		odas_data_array[i].y = 0.0;
		odas_data_array[i].activity = 0.0;
		odas_data_array[i].frequency = 0.0;
	}

	for (i = 0; i < 360; i++)
	{
		meeting_data->angle_array[i] = 0;
	}

	meeting_data->total_silence = 0;
	meeting_data->total_meeting_time = 0;
	meeting_data->num_participants = 0;
}

void write_to_file(std::string buffer)
{

	struct tm *timenow;
	std::string filename = "MP_";

	time_t now = time(NULL);
	timenow = gmtime(&now);
	filename += std::to_string(now);

	std::ofstream file(filename);
//	std::cin >> buffer;
	file << buffer;
	file.close();

}

//
// Entry point
//

int main(int argc, char **ppArgv)
{

	// A basic command-line parser
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = ppArgv[i];
		if (arg == "-q")
		{
			logLevel = ErrorsOnly;
		}
		else if (arg == "-v")
		{
			logLevel = Verbose;
		}
		else if (arg == "-d")
		{
			logLevel = Debug;
		}
		else
		{
			LogFatal((std::string("Unknown parameter: '") + arg + "'").c_str());
			LogFatal("");
			LogFatal("Usage: standalone [-q | -v | -d]");
			return -1;
		}
	}

	// Setup our signal handlers
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	// Register our loggers
	ggkLogRegisterDebug(LogDebug);
	ggkLogRegisterInfo(LogInfo);
	ggkLogRegisterStatus(LogStatus);
	ggkLogRegisterWarn(LogWarn);
	ggkLogRegisterError(LogError);
	ggkLogRegisterFatal(LogFatal);
	ggkLogRegisterAlways(LogAlways);
	ggkLogRegisterTrace(LogTrace);

	//SD reserve space for json string to improve performance
	serverDataTextString.reserve(MAXLINE);

	//SD
	//Create and bind UDP socket to get data from odas server

	// first declare UDP variables
	int in_sockfd;
	int bytes_returned;
	struct sockaddr_in in_addr;
	char input_buffer[MAXLINE];

	// Create socket file descriptor for server
	if ((in_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		LogFatal("Error creating socket");
		return -1;
	}
	// Populate socket structure information
	in_addr.sin_family = AF_INET; // IPv4
	in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	in_addr.sin_port = htons(INPORT);

	// Bind the input socket for target with the server address
	if (bind(in_sockfd, (const struct sockaddr *)&in_addr,
			 sizeof(in_addr)) < 0)
	{
		LogFatal("socket binding failed");
		return -1;
	}

	socklen_t len;
	len = sizeof(in_addr); //length data is neeeded for receive call

	// initialise all meeting data variables
	// initialise arrays for input and output data

	meeting meeting_data;	   // "meeting" is a  custom type
	participant_data *participant_data_array = (participant_data *)malloc(MAXPART * sizeof(participant_data)); // "participant data" is a struct
	odas_data *odas_data_array = (odas_data *)malloc(NUM_CHANNELS * sizeof(odas_data));					   // "odas data" is a struct
	initialise_meeting_data(&meeting_data, participant_data_array, odas_data_array);					   // set everything to zero

	// need to change the main to poll gpio to test for reset

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
		return -1;
	}

	// Wait for the server to start the shutdown process
	//
	// While we wait, every 15 ticks, drop the battery level by one percent until we reach 0
	while (ggkGetServerRunState() < EStopping)
	{

		//this is main polling loop for getting data from odas via UDP receive
		//it processes this data and then updates the bluetooth characteristic with new data

		bytes_returned = recvfrom(in_sockfd, (char *)input_buffer, MAXLINE,
								  MSG_DONTWAIT, (struct sockaddr *)&in_addr,
								  &len);

		if (bytes_returned > 0)
		{
//			printf("got %d bytes\n", bytes_returned);
			input_buffer[bytes_returned] = 0x00; // sets end for json parser


//			printf("going in to parsing");

			json_parse(input_buffer, odas_data_array);
		
//			printf("got past parsing");
			process_sound_data(&meeting_data, participant_data_array, odas_data_array);

			// build the string for the server
			// load data into shared buffer space for the data getter
			// first lock the mutex

			mutex_buffer.lock();

		// is this needed ?
			serverDataTextString = "{\"tMT\": ";
			serverDataTextString += std::to_string(meeting_data.total_meeting_time);
			serverDataTextString += ",\n\"m\": [\n";

			int i;
			for (i = 1; i < MAXPART; i++)
			{


				serverDataTextString += "[";
				serverDataTextString += std::to_string(participant_data_array[i].participant_angle);
				serverDataTextString += ",";
				serverDataTextString += std::to_string(participant_data_array[i].participant_is_talking);
				serverDataTextString += ",";
				serverDataTextString += std::to_string(participant_data_array[i].participant_num_turns);
				serverDataTextString += ",";
				serverDataTextString += std::to_string(participant_data_array[i].participant_total_talk_time);
				serverDataTextString += "]";


// new logic by sd to calc turns using energy 

				if (participant_data_array[i].participant_is_talking == 0) 
				{
					participant_data_array[i].participant_silent_time++;
				}
				else 
				{
					if (participant_data_array[i].participant_silent_time > MINTURNSILENCE) participant_data_array[i].participant_num_turns++;
					participant_data_array[i].participant_silent_time = 0;
				}




					

				
// end of new turn logic

				if (i < MAXPART-1)
				{
					serverDataTextString += ",";
				}
			}

			serverDataTextString += "]}\n";



/*			if (participant_data_array[i].participant_is_talking > 0)
			{
				// not sure this logic is necessary - prob all targets go to zero before dropping out
				participant_data_array[i].participant_is_talking = 0x00;
				if (participant_data_array[i].participant_silent_time > MINTURNSILENCE)
				{
					participant_data_array[i].participant_num_turns++;
					participant_data_array[i].participant_silent_time = 0;
				}
				else
				{
					participant_data_array[i].participant_silent_time++;
				}

// i think this logic was from when i only sent data about talking people
//				if (i != (meeting_data.num_participants))
//				{
//					serverDataTextString += ",";
//				}
			}
*/



			mutex_buffer.unlock();

		    	printf ("%s\n",serverDataTextString.c_str());

		// now the output string is ready and we should call notify
			ggkNofifyUpdatedCharacteristic("/com/gobbledegook/text/string");

			if (meeting_data.total_silence > MAXSILENCE)
			{
				// reset all the meeting stuff and write to file
				if (meeting_data.num_participants > 0)
				{
					mutex_buffer.lock();
					write_to_file(serverDataTextString);
					mutex_buffer.unlock();

				// reset data for next meeting
					initialise_meeting_data(&meeting_data, participant_data_array, odas_data_array);
				}
			}
		//		std::this_thread::sleep_for(std::chrono::seconds(2));

		//		sd need to change the battery level to be real - from PiJuice
		//		serverDataBatteryLevel = std::max(serverDataBatteryLevel - 1, 0);
		//		ggkNofifyUpdatedCharacteristic("/com/gobbledegook/battery/level");
		}
	}

	// Wait for the server to come to a complete stop (CTRL-C from the command line)
	if (!ggkWait())
	{
		return -1;
	}

	// Return the final server health status as a success (0) or error (-1)
	return ggkGetServerHealth() == EOk ? 0 : 1;
}
