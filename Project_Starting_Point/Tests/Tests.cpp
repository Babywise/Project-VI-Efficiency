#include "pch.h"
#include "CppUnitTest.h"
#include "../Shared/Metrics.h"
#include "../Shared/configManager.h"
#include "../Client/Client.cpp"
#ifdef _WIN32
#endif

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Unit_Tests
{

	TEST_CLASS(Metrics_Tests)
	{
	public:
		
		TEST_METHOD(Calculation_getSum)
		{
			// setup
			Metrics::Calculations calc;
			int numbers[4] = { 1,3,6,4 };
			float expectedSum = 14;
			float answer;

			//action
			for (int i = 0; i < sizeof(numbers)/sizeof(int); i++) {
				calc.addPoint(numbers[i]);
			}
			answer = calc.getSum();

			//assert
			Assert::AreEqual(expectedSum, answer);
		}

		TEST_METHOD(Calculation_getAverage)
		{
			// setup
			Metrics::Calculations calc;
			int numbers[4] = { 1,3,6,4 };
			float expected = 3.5;
			float answer;

			//action
			for (int i = 0; i < sizeof(numbers) / sizeof(int); i++) {
				calc.addPoint(numbers[i]);
			}
			answer = calc.getAverage();

			//assert
			Assert::AreEqual(expected, answer);
		}

		TEST_METHOD(Calculation_addPoint)
		{
			// setup
			Metrics::Calculations calc;
			float expected = 1;
			float answer;

			//action
			
			calc.addPoint(1);
			
			answer = calc.getSum();

			//assert
			Assert::AreEqual(expected, answer);
		}
		TEST_METHOD(Timer_Start)
		{
			// setup
			Metrics::Timer timer;
			bool expected = true;
			bool answer;

			//action
			answer = timer.start();
		
			//assert
			Assert::AreEqual(expected, answer);
		}

		#ifdef _WIN32
		TEST_METHOD(getTime_1_Second)
		{
			
			
			// setup
			Metrics::Timer timer;
			float startRange = 1000;
			float endRange = 1200;
			float answer;

			//action
			timer.start();
			Sleep(1000);
			answer = timer.getTime();
			
		
			//assert
			if (answer >= startRange && answer <= endRange) {
				Assert::AreEqual(0, 0);
			}
			else {
				Assert::Fail();
			}
		}
		TEST_METHOD(getTime_3_Second)
		{

			
			// setup
			Metrics::Timer timer;
			float startRange = 3000;
			float endRange = 3200;
			float answer;

			//action
			timer.start();
			Sleep(3000);
			answer = timer.getTime();

			
			//assert
			if (answer >= startRange && answer <= endRange) {
				Assert::AreEqual(0, 0);
			}
			else {
				Assert::Fail();
			}
		}
		#endif
	};
	TEST_CLASS(config_Manager_Tests) {

	public:
		TEST_METHOD(getConfig_exists_int) {
			
			std::string expected = "5";
			configuration::configManager manager("../../Tests/TestConfig.conf");
			Assert::AreEqual(atoi(expected.c_str()), atoi(manager.getConfig("test").c_str()));
		}
		TEST_METHOD(getConfig_exists_float) {

			std::string expected = "5.22";
			configuration::configManager manager("../../Tests/TestConfig.conf");
			Assert::AreEqual(atof(expected.c_str()), atof(manager.getConfig("testfloat").c_str()));
		}
		TEST_METHOD(getConfig_exists_string) {

			std::string expected = "works";
			configuration::configManager manager("../../Tests/TestConfig.conf");
			Assert::AreEqual(expected, manager.getConfig("filetest"));
		}
		TEST_METHOD(getConfig_not_exists_int) {
			std::string answer;
			
			configuration::configManager manager("");
			answer = manager.getConfig("not this one");
			Assert::AreEqual(0, (int)answer.size());
		}

	};
	
}

namespace metrics_Testing
{
	TEST_CLASS(IO)
	{
	public:
		/// <summary>
		/// This test method is used to ensure PERF_REQ_IO_001 is met
		/// </summary>
		TEST_METHOD(getFileSize)
		{
			//setup
			configuration::configManager manager("../../Tests/TestConfig.conf");
		
			int maxTime = 2; //2 milliseconds
			int time;
			Metrics::Timer timer;

			//act
			timer.start();

			FILE* f;
			int counter = 0;
			fopen_s(&f, manager.getConfigChar("fileLocation"), "rb");

			fseek(f, 0, SEEK_END);
			long fsize = ftell(f);
			fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

			char* data = (char*)malloc(fsize + 1);
			fread(data, fsize, 1, f);
			fclose(f);

			data[fsize] = 0;

			for (int i = 0; i < fsize + 1; i++) {

				if (data[i] == '\n') {
					counter++; // add each new line character as a count
				}
			}
			counter++; // incremement 1 since the last line wont have a new line character
			Sleep(0.1);
			time = timer.getTime();

			//assert
			if (time < maxTime) {
				Assert::AreEqual(1, 1);
			}
			else {
				Assert::Fail();
			}
		}
		
	};
}
