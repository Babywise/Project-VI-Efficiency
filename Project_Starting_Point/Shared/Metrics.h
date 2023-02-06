#pragma once


#include <Windows.h>
#include <chrono>
#include <vector>
#include "Logger.h"
#include <filesystem>


using namespace std::chrono;

#define METRICS // used to set metrics run or not

/// <summary>
/// Namespace is used for ease of use and quality of life calculations of code metrics
/// </summary>
namespace Metrics {

	const std::string metricsLogFile = "Metrics";
	Logger logger;

	/// <summary>
	/// used to simplify the calculation of code timings using the std::chrono library
	/// </summary>
	class Timer {
		steady_clock::time_point startTime;
	public:
		/// <summary>
		/// starts a timer
		/// </summary>
		bool start() {
			try {
				startTime = high_resolution_clock::now();
				return true;
			}
			catch (std::exception e) {
				return false;
			}
		}
		/// <summary>
		/// returns the time since the timer was started in milliseconds
		/// </summary>
		/// <returns></returns>
		float getTime() {
			steady_clock::time_point endTime = high_resolution_clock::now();

			return float((endTime - startTime).count()) / 1000000;

		}
	};

	/// <summary>
	/// helps to calculate averages of metrics quickly and easily
	/// </summary>
	class Calculations {
		float sum = 0;
		int count = 0;
	public:

		/// <summary>
		/// add a datapoint such as time at a given index t(n) add times such as 0.00734ms 
		/// </summary>
		/// <param name="point"></param>
		void addPoint(float point) {
			sum += point;
			count++;
		}

		/// <summary>
		/// get average for the points added such as times are 1s 3s, 7s therefor total averafe is 11/3
		/// </summary>
		/// <returns></returns>
		float getAverage() {
			return sum / count;
		}

		/// <summary>
		/// gets the sum for all the points added to this calculations object
		/// </summary>
		/// <returns></returns>
		float getSum() {
			return sum;
		}
	};


	/// <summary>
	/// Takes in a counter for times, a line counter, and a measure of timein milliseconds for getTime to run, specific to current implementation 01/26/2023
	/// </summary>
	/// <param name="calculations"></param>
	/// <param name="lineCounter"></param>
	/// <param name=""></param>
	/// <param name=""></param>
	void logIOMetrics(Metrics::Calculations calculations, Metrics::Calculations lineCounter, float timeToGetSize) {

		Timer timer;

		//format log file
		// write system information to lof before start of metrics logging
		logger.log("--- Start of Client IO Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);

		// log and archive system information

		system("wmic cpu get CurrentClockSpeed, MaxClockSpeed, Name, CurrentVoltage, DataWidth, ProcessorType | more >> \"%cd%/../Logs/Metrics.log\"");
		system("wmic cpu get CurrentClockSpeed, MaxClockSpeed, Name, CurrentVoltage, DataWidth, ProcessorType | more >> \"%cd%/../Archive/Metrics.archive\"");
		logger.emptyLine(metricsLogFile);
		system("wmic memorychip get FormFactor, Speed, Capacity, DataWidth, Manufacturer, name | more >> \"%cd%/../Logs/Metrics.log\"");
		system("wmic memorychip get FormFactor, Speed, Capacity, DataWidth, Manufacturer, name | more >> \"%cd%/../Archive/Metrics.archive\"");
		logger.emptyLine(metricsLogFile);
		system("wmic diskdrive get manufacturer, size,name, model, description | more >> \"%cd%/../Logs/Metrics.log\"");
		system("wmic diskdrive get manufacturer, size,name, model, description | more >> \"%cd%/../Archive/Metrics.archive\"");
		logger.emptyLine(metricsLogFile);
		// log information from counters and timers
		logger.log("Client - IO - Get File Size :" + std::to_string(timeToGetSize) + "ms", metricsLogFile);
		logger.log("Client - IO - Average time to get line from file : " + std::to_string(calculations.getAverage()) + "ms", metricsLogFile);
		logger.log("Client - IO - TotalTime reading files to get specific lines : " + std::to_string(calculations.getSum()) + "ms", metricsLogFile);
		logger.log("Client - IO - Total lines reading files ( not including get file length ) : " + std::to_string(int(lineCounter.getSum())), metricsLogFile);

		// get file counts, plus total bytes of data from all .txt files
		int fileCounter = 0; // to count number of txt files
		int byteCounter = 0; // to get total number of bytes
		for (const auto& entry : std::filesystem::directory_iterator("../Client/")) {
			if (entry.path().extension().string() == ".txt") {
				byteCounter += std::filesystem::file_size(entry.path());
				fileCounter++;
			}
		}
		// log data about file counts and bytes
		logger.log("Client - IO - Data File Count (.txt) is : " + std::to_string(fileCounter), metricsLogFile);
		logger.log("Client - IO - Total bytes in data files is : " + std::to_string(byteCounter), metricsLogFile);

		//log formatting
		logger.emptyLine(metricsLogFile);
		logger.log("--- End of Client IO Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
	}

	void logDataParsingMetricsClient(Calculations dataParsingTimeCalc, Calculations sizeOfDataParsedDataClientCalc, int numDataParsesClient) {
		//data parsing results
		logger.log("--- Start of Client DataParsing Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Client - DataParsing - Total Time = " + std::to_string(dataParsingTimeCalc.getSum()) + " ms", metricsLogFile);
		logger.log("Client - DataParsing - Average (Single Parse) = " + std::to_string(dataParsingTimeCalc.getAverage()) + " ms", metricsLogFile);
		logger.log("Client - DataParsing - # of Conversions = " + std::to_string(numDataParsesClient), metricsLogFile);
		logger.log("Client - DataParsing - Input Size of Parsed Data = " + std::to_string(std::filesystem::file_size("DataFile.txt")) + " Bytes", metricsLogFile);
		logger.log("Client - DataParsing - Total Size of Parsed Data = " + std::to_string((int)sizeOfDataParsedDataClientCalc.getSum()) + " Bytes", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("--- End of Client DataParsing Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
	}

	void logDataParsingMetricsServer(Calculations dataParsingTimeCalc, Calculations sizeOfDataParsedDataServerCalc, int numDataParsesServer) {
		//data parsing results
		logger.log("--- Start of Server DataParsing Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Server - DataParsing - Total Time = " + std::to_string(dataParsingTimeCalc.getSum()) + " ms", metricsLogFile);
		logger.log("Server - DataParsing - Average (Single Parse) = " + std::to_string(dataParsingTimeCalc.getAverage()) + " ms", metricsLogFile);
		logger.log("Server - DataParsing - # of Conversions = " + std::to_string(numDataParsesServer), metricsLogFile);
		logger.log("Server - DataParsing - Total Size of Parsed Data = " + std::to_string((int)sizeOfDataParsedDataServerCalc.getSum()) + " Bytes", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("--- End of Server DataParsing Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
	}

	void logCalcInfo(float calcTime, int numCalc) {
		//calculation results
		logger.log("--- Start of Server Calculation Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Server - Calculations - Average time used for a calculation: " + std::to_string((calcTime / numCalc) * 1000) + " �s", metricsLogFile);
		logger.log("Server - Calculations - Total time used for calculation: " + std::to_string(calcTime) + " ms", metricsLogFile);
		logger.log("Server - Calculations - Total number of calculations done: " + std::to_string(numCalc) + " ms", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("--- End of Server Calculation Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
	}

	void logMemoryMetricsServer(Calculations sizeOfMemoryServerCalc) {
		//memory results
		logger.log("--- Start of Server Memory Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Server - Memory - Total Memory Allocated: " + std::to_string(sizeOfMemoryServerCalc.getSum()) + " Bytes", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("--- End of Server Memory Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
	}

	void logNetworkMetricsClient(int numTransmissions, int avgHandshake, int handshakeTransmissionCount, std::string networkType) {
		// Client Network Results
		logger.log("--- Start of Client Network Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Client - Network - Network Type: " + networkType, metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Client - Network - Number of Transmissions: " + std::to_string(numTransmissions), metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Client - Network - Average Handshake Time: " + std::to_string(avgHandshake) + " �s", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Client - Network - Number of Transmissions in Handshake: " + std::to_string(handshakeTransmissionCount), metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("--- End of Client Network Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);

	}

	void logNetworkMetricsServer(int elapsedTimeMilSec, int numConnections) {
		// Server Network Results
		logger.log("--- Start of Server Network Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Server - Network - Server Uptime: " + std::to_string(elapsedTimeMilSec) + " ms", metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("Server - Network - Number of Connections: " + std::to_string(numConnections), metricsLogFile);
		logger.emptyLine(metricsLogFile);
		logger.log("--- End of Server Network Metrics ---", metricsLogFile);
		logger.emptyLine(metricsLogFile);
	}

	void addLogEndOfFileSpacing() {
		for (int i = 0; i < 3; i++) {
			logger.emptyLine(metricsLogFile);
		}
	}

}