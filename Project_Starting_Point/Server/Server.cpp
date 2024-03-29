#include <windows.networking.sockets.h>

#include <iostream>

#include <string>
#include <thread>
#include <vector>

#include "../Shared/Packet.h"
#include "../Shared/load_packet.h"
#include "../Shared/Logger.h"
#include "../Shared/configManager.h"

configuration::configManager configurations("../Shared/config.conf");

#define METRICS
#ifdef METRICS

#include "../Shared/Metrics.h"

int numDataParsesServer = 0;
float maxSizeRxData = 0;

Metrics::Timer timer;
Metrics::Timer calcTimer;
Metrics::Calculations dataParsingTimeCalc;
Metrics::Calculations sizeOfDataParsedDataServerCalc;
Metrics::Calculations sizeOfMemoryServerCalc;

// Network
int numTotalConnections;
int numCompletedConnections;
int numCurrentConnections;
int numFailedConnections;
CRITICAL_SECTION critical;
CRITICAL_SECTION loggingCritical;
CRITICAL_SECTION parsingCritical;

std::chrono::time_point<std::chrono::system_clock> serverStartTime;

#endif // METRICS

#pragma comment (lib, "Ws2_32.lib")

const int numColumns = 1;

struct StorageTypes
{
	std::string startTime = "";
	std::string currentTime = "";
	float startingFuel = 0;
	float sumFuel = 0;
	int size = 0;
};

void UpdateData(StorageTypes* plane, float currentFuelPoint, std::string timeStamp);
float CalcAvg(StorageTypes* plane);
float CalcFuelConsumption(StorageTypes* plane, float currentFuel);

static int numCalc = 0;		//number of calculations
static float calcTime = 0;


void clientHandler(SOCKET clientSocket)
{
#ifdef METRICS
	float localDataParsingTimeCalc = 0;
	int localSizeOfDataParsedDataServerCalc = 0;
	int localNumDataParsesServer = 0;
	// This is a critical section to avoid a deadlock while updating the global variables.
	EnterCriticalSection(&critical);
	numTotalConnections++;
	numCurrentConnections++;
	LeaveCriticalSection(&critical);

	// Initialize the endtime Variable. We don't want this to be global.
	std::chrono::time_point<std::chrono::system_clock> connectionEndTime;
#endif // METRICS

	std::thread::id threadID = std::this_thread::get_id();
	unsigned int planeID = *static_cast<unsigned int*>(static_cast<void*>(&threadID));

	std::cout << "Connection Established with Plane: " << planeID << std::endl;
#ifdef METRICS
	timer.start();
#endif
	Packet p(planeID);
#ifdef METRICS
	localDataParsingTimeCalc += timer.getTime();
	localNumDataParsesServer += 3;
#endif
	send(clientSocket, p.serialize(), Packet::getPacketSize(), 0);
	p.freeBuffer();
#ifdef METRICS
	localNumDataParsesServer += 3;
	localSizeOfDataParsedDataServerCalc += Packet::getPacketSize();
#endif
	StorageTypes plane;
	char* RxBuffer = (char*)malloc(Packet::getPacketSize());
	memset(RxBuffer, NULL, Packet::getPacketSize());
	float fValue;
	std::string timestamp;

	int loopCounter = 0;
	bool exit = false;

	bool failedConn = false;
	std::string errMessage;

	while (!exit)
	{
		memset(RxBuffer, NULL, Packet::getPacketSize());
	
		size_t result = recv(clientSocket, RxBuffer, Packet::getPacketSize(), 0);

		if (result == SOCKET_ERROR || result == 0) {
			failedConn = true;

			DWORD err = GetLastError();
			LPSTR messageBuffer = nullptr;

			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

			std::string message(messageBuffer, size);
			errMessage = message;

			break;
		}

#ifdef METRICS
		timer.start();
#endif

		p = Packet(RxBuffer);

#ifdef METRICS
		localDataParsingTimeCalc += timer.getTime();
		localNumDataParsesServer += 3;
		localSizeOfDataParsedDataServerCalc += Packet::getPacketSize();
#endif

		if (strcmp(p.getTimestamp().c_str(), "*") == 0) 
		{ 
			p.setCurrentFuelConsumption(CalcFuelConsumption(&plane, fValue));
			p.setAverageFuelConsumption(CalcAvg(&plane));
			p.swapIP();

			std::tm tm1 = {};
			std::istringstream ss1(plane.startTime);
			ss1 >> std::get_time(&tm1, "%d_%m_%Y %H:%M:%S");
			auto timeStart = std::chrono::system_clock::from_time_t(std::mktime(&tm1));

			std::tm tm2 = {};
			std::istringstream ss2(plane.currentTime);
			ss2 >> std::get_time(&tm2, "%d_%m_%Y %H:%M:%S");
			auto timeEnd = std::chrono::system_clock::from_time_t(std::mktime(&tm2));

			auto diff = std::chrono::duration_cast<std::chrono::seconds>(timeEnd - timeStart).count();

			p.setTimeStamp(std::to_string(diff));

			send(clientSocket, p.serialize(), Packet::getPacketSize(), 0); //send final stats back 
			p.freeBuffer();
#ifdef METRICS
			localNumDataParsesServer += 3;
			localSizeOfDataParsedDataServerCalc += Packet::getPacketSize();
#endif
			exit = true; 
		} else {

			if (strcmp(p.getParamName().c_str(), configurations.getConfigChar("columnOne")) == 0) {

				fValue = p.getFuelTotalQuantity();
				timestamp = p.getTimestamp();

				if (loopCounter == 0) {
					plane.startingFuel = fValue;
					plane.startTime = timestamp;
				}

				UpdateData(&plane, fValue, timestamp);

				p.setCurrentFuelConsumption(CalcFuelConsumption(&plane, fValue));
				p.setAverageFuelConsumption(CalcAvg(&plane));
				p.swapIP();

				send(clientSocket, p.serialize(), Packet::getPacketSize(), 0);//send average back 
				p.freeBuffer();

#ifdef METRICS
				localNumDataParsesServer += 3;
				localSizeOfDataParsedDataServerCalc += Packet::getPacketSize();
#endif
			} else {

				std::string invalidMessage = "Invalid Parameter Name, Closing Connection.";
				// Kill connection if not one of the correct param names
				send(clientSocket, invalidMessage.c_str(), invalidMessage.length(), 0);//send average back 
				exit = true;

			}
		}
		loopCounter++;
	}

	std::cout << "Closing connection to Plane: " << planeID << std::endl;
	closesocket(clientSocket);
#ifdef METRICS
	// Do the Network Logging for each client connection.
	connectionEndTime = std::chrono::system_clock::now();
	auto currentUptime = std::chrono::duration_cast<std::chrono::milliseconds>(connectionEndTime - serverStartTime);

	EnterCriticalSection(&loggingCritical);

	std::cout << "Writing report to log for plane: " << planeID << std::endl;
	numDataParsesServer += localNumDataParsesServer;
	dataParsingTimeCalc.addPoint(localDataParsingTimeCalc);
	sizeOfDataParsedDataServerCalc.addPoint(localSizeOfDataParsedDataServerCalc);

	// This is a critical section to avoid a deadlock while updating the global variables.
	//EnterCriticalSection(&critical);
	numCurrentConnections--; // socket closed so decrement the current connections.
	// if the failed boolean is false we have a completed connection
	if (failedConn == false) {
		numCompletedConnections++;
	} else {
		numFailedConnections++;
	}
	//LeaveCriticalSection(&critical);

	Metrics::logFlightStatisticsServer(p, plane.startingFuel);
	Metrics::logNetworkMetricsServer(planeID, currentUptime, numTotalConnections, numCurrentConnections, numCompletedConnections, numFailedConnections, errMessage);
	Metrics::logCalcInfo(calcTime, numCalc);
	Metrics::logDataParsingMetricsServer(dataParsingTimeCalc, sizeOfDataParsedDataServerCalc, numDataParsesServer);
	std::cout << "Finished writing report to log for plane: " << planeID << std::endl;
	LeaveCriticalSection(&loggingCritical);
#endif
	free(RxBuffer);
}
void setup();
void tearDown();
int main()
{
	setup();




#ifdef METRICS
	InitializeCriticalSection(&critical);
	InitializeCriticalSection(&loggingCritical);
	InitializeCriticalSection(&parsingCritical);
#endif // METRICS


	// Initialize Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed.\n";
		return 1;
	}

	// Create a socket for listening
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "socket failed with error: " << WSAGetLastError() << '\n';
		WSACleanup();
		return 1;
	} else {
#ifdef METRICS
		Metrics::setServerLogName(configurations.getConfigChar("serverMetricsLogFileName"));
		Metrics::logStartOfServer();
		Metrics::logSystemStatsMetrics(false);
		serverStartTime = std::chrono::system_clock::now();
#endif // METRICS
		//}

		// Bind the socket to an address and port
		sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(atoi(configurations.getConfigChar("port"))); // Magic Number
		if ( bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR ) {
			std::cerr << "bind failed with error: " << WSAGetLastError() << '\n';
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		// Start listening for incoming connections
		if ( listen(listenSocket, SOMAXCONN) == SOCKET_ERROR ) {
			std::cerr << "listen failed with error: " << WSAGetLastError() << '\n';
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		std::vector<std::thread> threads;
		DWORD timeout = atoi(configurations.getConfigChar("sockettimeoutseconds")) * 1000;

		// Accept incoming connections and spawn a thread to handle each client
		while ( true ) {
			SOCKET clientSocket = accept(listenSocket, NULL, NULL);
			if ( clientSocket == INVALID_SOCKET ) {
				std::cerr << "accept failed with error: " << WSAGetLastError() << '\n';
				closesocket(listenSocket);
				WSACleanup();
				return 1;
			}

			setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

			threads.emplace_back(clientHandler, clientSocket);
		}
	}
	// Close the listening socket and cleanup Winsock
	closesocket(listenSocket);
	WSACleanup();

#ifdef METRICS
	DeleteCriticalSection(&critical);
	DeleteCriticalSection(&loggingCritical);
	DeleteCriticalSection(&parsingCritical);
#endif // METRICS
	tearDown();
	return 0;
}

void UpdateData(StorageTypes* plane, float currentFuelPoint, std::string timeStamp)
{
#ifdef METRICS
	calcTimer.start();
#endif
	plane->size++;
	plane->sumFuel += currentFuelPoint;
	plane->currentTime = timeStamp;
#ifdef METRICS
	calcTime += calcTimer.getTime();
	numCalc += 2;
#endif
}

float CalcAvg(StorageTypes* plane)
{
#ifdef METRICS
	calcTimer.start();
#endif
	float result = plane->sumFuel / plane->size;
#ifdef METRICS
	calcTime += calcTimer.getTime();
	numCalc++;
#endif
	return result;
}

float CalcFuelConsumption(StorageTypes* plane, float currentFuel) {
#ifdef METRICS
	calcTimer.start();
#endif
	float result = plane->startingFuel - currentFuel;
#ifdef METRICS
	calcTime += calcTimer.getTime();
	numCalc++;
#endif
	return result;
}



/// <summary>
/// add server to load balancer
/// </summary>
void setup() {

	//setup
	WSADATA wsaData;
	SOCKET ClientSocket;
	sockaddr_in SvrAddr;
	char* Rx = (char*)malloc(4);



	std::cout << "SETUP -> ";
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SvrAddr.sin_family = AF_INET;
	int port = atoi(configurations.getConfigChar("balancer_port"));
	SvrAddr.sin_port = htons(port);

	SvrAddr.sin_addr.s_addr = inet_addr(configurations.getConfigChar("balancer_IP"));
	std::cout << "Connecting -> ";
	int v =connect(ClientSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)); // connect
	recv(ClientSocket, Rx, 3, 0);
	Rx[3] = '\0';
	if (strcmp(Rx, "ACK") == 0) {
		std::cout << "ACK -> ";
		load_packet pack("server",configurations.getConfigChar("lanAddr"), configurations.getConfigChar("port"));
		send(ClientSocket, pack.serialize(), load_packet::getPacketSize(),0);
	}
	std::cout << "Closing" << std::endl;
	closesocket(ClientSocket);
}
/// <summary>
/// remove server from load balancer
/// </summary>
void tearDown() {
	//setup
	WSADATA wsaData;
	SOCKET ClientSocket;
	sockaddr_in SvrAddr;
	char* Rx = (char*)malloc(4);




	WSAStartup(MAKEWORD(2, 2), &wsaData);
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SvrAddr.sin_family = AF_INET;
	int port = atoi(configurations.getConfigChar("balancer_port"));
	SvrAddr.sin_port = htons(port);

	SvrAddr.sin_addr.s_addr = inet_addr(configurations.getConfigChar("balancer_IP"));

	int v = connect(ClientSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)); // connect
	recv(ClientSocket, Rx, 3, 0);
	Rx[3] = '\0';
	if (strcmp(Rx, "ACK") == 0) {

		load_packet pack("server", configurations.getConfigChar("lanAddr"), configurations.getConfigChar("port"));
		pack.setTerminate(true);
		send(ClientSocket, pack.serialize(), load_packet::getPacketSize(), 0);
	}
	closesocket(ClientSocket);
}