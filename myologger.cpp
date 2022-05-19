#define _USE_MATH_DEFINES
//#include "eyetracker.h"
#include "stdafx.h"
#include "myologger.h"
#include <fstream>
#include <sstream>

// �ϴ� �� ���丮�� �־�� ������ ������ �� �ִ�.
static std::ofstream outFile;

// Classes that inherit from myo::DeviceListener can be used to receive events from Myo devices. DeviceListener
// provides several virtual functions for handling different kinds of events. If you do not override an event, the
// default behavior is to do nothing.
class DataCollector : public myo::DeviceListener {
public:
	DataCollector()
		: onArm(false), isUnlocked(false), roll(0), pitch(0), yaw(0), accl_x(0), accl_y(0), accl_z(0), gyro_x(0), gyro_y(0), gyro_z(0), currentPose(), emgSamples()
	{
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost a Myo.
		// Let's clean up some leftover state.
		roll = 0; pitch = 0; yaw = 0;
		accl_x = 0; accl_y = 0; accl_z = 0;
		gyro_x = 0; gyro_y = 0; gyro_z = 0;
		onArm = false;
		isUnlocked = false;
		emgSamples.fill(0);
	}

	// onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
	void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
	{
		for (int i = 0; i < 8; i++) {
			emgSamples[i] = emg[i];
		}
	}

	// onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
	// as a unit quaternion.
	void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat)
	{
		using std::atan2;
		using std::asin;
		using std::sqrt;
		using std::max;
		using std::min;

		// Calculate Euler angles (roll, pitch, and yaw) from the unit quaternion.
		roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
			1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
		pitch = asin(max(-1.0f, min(1.0f, 2.0f * (quat.w() * quat.y() - quat.z() * quat.x()))));
		yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
			1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));

		/*
		// Convert the floating point angles in radians to a scale from 0 to 18.
		roll_w = static_cast<int>((roll + (float)M_PI) / (M_PI * 2.0f) * 18);
		pitch_w = static_cast<int>((pitch + (float)M_PI / 2.0f) / M_PI * 18);
		yaw_w = static_cast<int>((yaw + (float)M_PI) / (M_PI * 2.0f) * 18);
		*/
	}

	// onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
	// making a fist, or not making a fist anymore.
	void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
	{
		currentPose = pose;
		
		if (pose != myo::Pose::unknown && pose != myo::Pose::rest) {
			// Tell the Myo to stay unlocked until told otherwise. We do that here so you can hold the poses without the
			// Myo becoming locked.
			myo->unlock(myo::Myo::unlockHold);

			// Notify the Myo that the pose has resulted in an action, in this case changing
			// the text on the screen. The Myo will vibrate.
			myo->notifyUserAction();
		}
		else {
			// Tell the Myo to stay unlocked only for a short period. This allows the Myo to stay unlocked while poses
			// are being performed, but lock after inactivity.
			// myo->unlock(myo::Myo::unlockTimed);

			// this holds the Myo to stay unlocked one poses are being performed.
			myo->unlock(myo::Myo::unlockHold);
		}
	}

	// onArmSync() is called whenever Myo has recognized a Sync Gesture after someone has put it on their
	// arm. This lets Myo know which arm it's on and which way it's facing.
	void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,
		myo::WarmupState warmupState)
	{
		onArm = true;
		whichArm = arm;
	}

	// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
	// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
	// when Myo is moved around on the arm.
	void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
	{
		onArm = false;
	}

	// onUnlock() is called whenever Myo has become unlocked, and will start delivering pose events.
	void onUnlock(myo::Myo* myo, uint64_t timestamp)
	{
		isUnlocked = true;
	}

	// onLock() is called whenever Myo has become locked. No pose events will be sent until the Myo is unlocked again.
	void onLock(myo::Myo* myo, uint64_t timestamp)
	{
		isUnlocked = false;
	}

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	
	// onAccelerometerData is called whenever new acceleromenter data is provided
	// Be warned: This will not make any distiction between data from other Myo armbands
	void onAccelerometerData(myo::Myo *myo, uint64_t timestamp, const myo::Vector3< float > &accel) {
		accl_x = accel.x();
		accl_y = accel.y();
		accl_z = accel.z();

	}

	// onGyroscopeData is called whenever new gyroscope data is provided
	// Be warned: This will not make any distiction between data from other Myo armbands
	void onGyroscopeData(myo::Myo *myo, uint64_t timestamp, const myo::Vector3< float > &gyro) {
		gyro_x = gyro.x();
		gyro_y = gyro.y();
		gyro_z = gyro.z();
	}
	
	void log_data()
	{
		auto dt = 123; //tmr.elapsed(); //MilliSecFromEpoch();

		outFile << dt << ", ";

		// Data is valid only when onArm.
		outFile << onArm << ", " << isUnlocked << ", " << (whichArm == myo::armLeft ? 1 : 0) << ", ";
		outFile << roll << ", " << pitch << ", " << yaw << ", ";
		outFile << accl_x << ", " << accl_y << ", " << accl_z << ", ";
		outFile << gyro_x << ", " << gyro_y << ", " << gyro_z << ", ";

		// Print out the EMG data.
		for (size_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);
			std::string emgString = oss.str();
			if (i != 0)
				outFile << ", ";
			outFile << emgString;
		}
		outFile << std::flush;

		outFile << std::endl;
	}

	// We define this function to print the current values that were updated by the on...() functions above.
	void print()
	{
		auto dt = 123; //tmr.elapsed(); //MilliSecFromEpoch();

		std::cout << dt << ", ";

		// Data is valid only when onArm.
		std::cout << onArm << ", " << isUnlocked << ", " << (whichArm == myo::armLeft ? 1 : 0) << ", ";
		std::cout << roll << ", " << pitch << ", " << yaw << ", ";
		std::cout << accl_x << ", " << accl_y << ", " << accl_z << ", ";
		std::cout << gyro_x << ", " << gyro_y << ", " << gyro_z << ", ";

		// Print out the EMG data.
		for (size_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);
			std::string emgString = oss.str();
			if (i != 0)
				std::cout << ", ";
			std::cout << emgString;
		}
		std::cout << std::flush;

		std::cout << std::endl;
	}

	// These values are set by onArmSync() and onArmUnsync() above.
	bool onArm;
	myo::Arm whichArm;

	// This is set by onUnlocked() and onLocked() above.
	bool isUnlocked;

	// These values are set by onOrientationData() and onPose() above.
	float roll, pitch, yaw, accl_x, accl_y, accl_z, gyro_x, gyro_y, gyro_z;
	myo::Pose currentPose;

	// The values of this array is set by onEmgData() above.
	std::array<int8_t, 8> emgSamples;
};

int LogMyoArmband(std::string file_name)
{


	// We catch any exceptions that might occur below -- see the catch statement for more details.
	try {

		// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
		// publishing your application. The Hub provides access to one or more Myos.
		myo::Hub hub("com.kiml.myologger");


		

		std::cout << "MyoArmband : Finding a Myo..." << std::endl;

		// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
		// immediately.
		// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
		// if that fails, the function will return a null pointer.
		myo::Myo* myo = hub.waitForMyo(10000);

		// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
		if (!myo) {
			throw std::runtime_error("Unable to find a Myo!");
		}

		// We've found a Myo.
		std::cout << "MyoArmband : Connection Established with Myo armband" << std::endl;

		// Next we enable EMG streaming on the found Myo.
		myo->setStreamEmg(myo::Myo::streamEmgEnabled);

		// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
		DataCollector collector;

		// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
		// Hub::run() to send events to all registered device listeners.
		hub.addListener(&collector);
		

		// define an ofstream for log
		outFile = std::ofstream("rawdata/" + file_name + ".csv");
		//tmr.write_epoch_time(outFile);



		// mutex wait
		//mutex->lock(); // init lock
		/*if (UDP_DEFINED && !RECORDING) {
			std::cout << "MyoArmband : Waiting for LoggerSlate..." << std::endl;
			mutex->lock();
		}*/
		
		//if (UDP_DEFINED && !RECORDING)
		//	std::cout << "MyoArmband : Waiting for LoggerSlate..." << std::endl;

		bool recordingStarted = false;

		// Finally we enter our main loop.
		while (1) {
			// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
			// In this case, we wish to update our display 20 times a second, so we run for 1000/20 milliseconds. -> hub.run(1000 / 20);
			// EMG(5ms), IMU(20ms)�ε� �츮�� ���� ª�� interval�� 5ms�� �����ϵ��� �Ѵ�.
			// hub.run�� ��� iter���� ������ device or resource busy �߰� �α��� ���ϹǷ� ��¿������ spin�ϸ� ��� ���θ� �޾ƿ´�.
			hub.run(5); // hub.run(1000 / 20);

			// After processing events, we call the print() member function we defined above to print out the values we've
			// obtained from any events that have occurred.
			if (collector.onArm) {//(RECORDING) {
				if (!recordingStarted) {
					std::cout << "MyoArmband : Logging Start (saved at rawdata/" + file_name + ".csv)" << std::endl;
					recordingStarted = true;
				}
				collector.log_data();
				collector.print();
			}
			// if _sleep, kill thread and flush logFile
			else if (!collector.onArm) {//(UDP_DEFINED && recordingStarted) {
				//tmr.write_finish_time(outFile);
				outFile.close();
				std::cout << "MyoArmband : Finished by LoggerSlate" << std::endl;
				return 0;
			}
		}

		// If a standard exception occurred, we print out its message and exit.
	}
	catch (const std::exception& e) {
		//std::cout << "MyoArmband : Error! " << e.what() << std::endl;
		std::cerr << "MyoArmband : Error! " << e.what() << std::endl;
		std::cerr << "MyoArmband : Press any key to exit...";
		std::cin.ignore();
		return 1;
	}
}
