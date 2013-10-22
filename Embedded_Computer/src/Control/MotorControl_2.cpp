#include "Control/MotorControl_2.h"
#include "Utilities/logger.h"

#include <iostream>
#include <iterator>
#include <algorithm>
#include <functional>
#include <boost/algorithm/clamp.hpp>
#include <boost/asio.hpp>

using boost::filesystem::path;

//#define DEBUG_TEST_MOTION

using boost::algorithm::clamp;


#define DANGER_TEST_MOTION

namespace
{
	const double dAngleConvertion = 0.325;
	const double dInvAngleConvertion = 1/dAngleConvertion;
}

Motor::Motor(STM32F4 *stm32f4, std::string name, int id, int offset, int min, int max, int speed)
:
_stm32f4(stm32f4),
_name(name),
_id(id),
_offset(offset),
_min(min),
_max(max),
_speed(speed),
_currentPos(-1),
_nextPos(-1)
{    
}

Motor::~Motor()
{
}

void Motor::setPos(double pos)
{
    _nextPos = pos;
}

const double Motor::getPos()
{
    return _currentPos;
}

void Motor::setTorque(bool value)
{
    _stm32f4->setTorque(_id, value ? STM32F4::TorqueOn : STM32F4::TorqueOff);
}

int Motor::Angle2Value(const double angle)
{
	int value = (angle*dInvAngleConvertion) + _offset;
	return clamp(value, _min, _max);
}

double Motor::Value2Angle(const int value)
{
	int clampedValue = clamp(value, _min, _max);
	return double(clampedValue - _offset)*dAngleConvertion;
}

void Motor::Read()
{
    _currentPos = Value2Angle(_stm32f4->read(_id));
}

void Motor::Write()
{
    if (_currentPos != _nextPos)
        _stm32f4->setMotor(_id, Angle2Value(_nextPos));
}

MotorControl::MotorControl( ThreadManager *threadManager, const XmlParser &config ) :
 _threadManager(threadManager)
{
    try
    {
        // Init USB interface with STM32F4
        Logger::getInstance() << "Initializing USB interface..." << std::endl;
        boost::asio::io_service boost_io;
        std::string port_name = config.getStringValue(XmlPath::Root / "USB_Interface" / "TTY");
        _stm32f4 = new STM32F4(port_name, boost_io);
        threadManager->create(50, boost::bind(&boost::asio::io_service::run, &boost_io));
    }
    catch (std::exception& e)
    {
        Logger::getInstance(Logger::LogLvl::ERROR) << "Exception in MotorControl.cpp while initialising USB interface : " << e.what() << std::endl;
    }

    InitializeMotors(config);
    InitializeConfigurations(config);
}

MotorControl::~MotorControl()
{

}

void MotorControl::Start()
{
    // Main task reading and sending data
    ReadAll();
    _threadManager->resume(ThreadManager::Task::LEGS_CONTROL);
    _threadManager->wait();
    WriteAll();
    _threadManager->wait(); 
}

// Populate the motor list
void MotorControl::InitializeMotors(const XmlParser &config)
{
    std::map<std::string, path> paths;
    paths.insert(std::make_pair("R_HIP_YAW", XmlPath::LegsMotors / XmlPath::R_HIP_YAW));
	paths.insert(std::make_pair("L_HIP_YAW", XmlPath::LegsMotors / XmlPath::L_HIP_YAW));
	paths.insert(std::make_pair("R_HIP_ROLL", XmlPath::LegsMotors / XmlPath::R_HIP_ROLL));
	paths.insert(std::make_pair("L_HIP_ROLL", XmlPath::LegsMotors / XmlPath::L_HIP_ROLL));
	paths.insert(std::make_pair("R_HIP_PITCH", XmlPath::LegsMotors / XmlPath::R_HIP_PITCH));
	paths.insert(std::make_pair("L_HIP_PITCH", XmlPath::LegsMotors / XmlPath::L_HIP_PITCH));
	paths.insert(std::make_pair("R_KNEE", XmlPath::LegsMotors / XmlPath::R_KNEE));
	paths.insert(std::make_pair("L_KNEE", XmlPath::LegsMotors / XmlPath::L_KNEE));
	paths.insert(std::make_pair("R_ANKLE_PITCH", XmlPath::LegsMotors / XmlPath::R_ANKLE_PITCH));
	paths.insert(std::make_pair("L_ANKLE_PITCH", XmlPath::LegsMotors / XmlPath::L_ANKLE_PITCH));
	paths.insert(std::make_pair("R_ANKLE_ROLL", XmlPath::LegsMotors / XmlPath::R_ANKLE_ROLL));
	paths.insert(std::make_pair("L_ANKLE_ROLL", XmlPath::LegsMotors / XmlPath::L_ANKLE_ROLL));

	for (auto it = paths.begin(); it != paths.end(); ++it)
	{
	    int id     = config.getIntValue(it->second / XmlPath::MotorID);
		int offset = config.getIntValue(it->second / XmlPath::Offset);
		int min    = config.getIntValue(it->second / XmlPath::LimitMin);
		int max    = config.getIntValue(it->second / XmlPath::LimitMax);
        int speed  = config.getIntValue(it->second / XmlPath::Speed);    
        Motor *motor = new Motor(_stm32f4, it->first, id, offset, min, max, speed);
	    _motors.insert(std::make_pair(it->first, motor));
    }
}

// Populate the configuration list
void MotorControl::InitializeConfigurations(const XmlParser &config)
{
    std::map<Config, path> paths;
    paths.insert(std::make_pair(Config::ALL_MOTORS, XmlPath::Configurations / "ALL_MOTORS"));
    paths.insert(std::make_pair(Config::ALL_LEGS, XmlPath::Configurations / "ALL_LEGS"));
    paths.insert(std::make_pair(Config::RIGHT_LEG, XmlPath::Configurations / "RIGHT_LEG"));
    paths.insert(std::make_pair(Config::LEFT_LEG, XmlPath::Configurations / "LEFT_LEG"));
    paths.insert(std::make_pair(Config::HEAD, XmlPath::Configurations / "HEAD"));

    for (auto it = paths.begin(); it != paths.end(); ++it)
    {
        std::vector<Motor*> motors;
        std::vector<std::string> names = config.getChildrenStringValues(it->second);
        for (auto name = names.begin(); name != names.end(); ++name)
        {
            if (_motors.find(*name) != _motors.end())
                motors.push_back(_motors[*name]);
        } 
        _configurations.insert(std::make_pair(it->first, motors));
    }
}

bool MotorControl::SetTorque(bool value, const Config config)
{
   bool status = true;
   if (_configurations.find(config) != _configurations.end())
   {
       Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"SetTorque\" : Configuration is invalid." << std::endl;
       return false;
   }

   for (auto it = _configurations[config].begin(); it != _configurations[config].end() && status; ++it)
   {
       // TODO : Grab motor status
       (*it)->setTorque(value);
      //_cm730->WriteByte(*itr, MX28::P_P_GAIN, JointData::P_GAIN_DEFAULT, 0);
      //_cm730->WriteByte(*itr, MX28::P_I_GAIN, JointData::I_GAIN_DEFAULT, 0);
      //_cm730->WriteByte(*itr, MX28::P_D_GAIN, JointData::D_GAIN_DEFAULT, 0);
   }
   return status;
}

bool MotorControl::InitPosition(const std::vector<double>& desiredPos, const Config config,
				                const double msTotalTime /*= 10000.0*/,
				                const double msDt /*= 16*/ )
{
   if (_configurations.find(config) == _configurations.end())
   {
       Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"InitPosition\" : Configuration is invalid." << std::endl;
       return false;
   }
   
   if (desiredPos.size() != _configurations[config].size())
   {
#ifdef DEBUG_TEST_MOTION
       Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"InitPosition\" : Joint number is invalid." << std::endl;
#endif
       return false;
   }

   Logger::getInstance(Logger::LogLvl::INFO) << "Setting initial position" << std::endl;
#ifdef DEBUG_TEST_MOTION
   Logger::getInstance(Logger::LogLvl::DEBUG) << "";
   std::copy(desiredPos.begin(), desiredPos.end(), std::ostream_iterator<double>(std::cout, " "));
   Logger::getInstance(Logger::LogLvl::DEBUG) << std::endl;
#endif   

   ReadAll();

   std::vector<double> pos;
   if (!ReadPositions(pos, config))
   {
       Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"InitPosition\" : Error ready positions.";
       return false;
   }
   std::vector<double> posDt;

   auto itrDesiredPos = desiredPos.begin();
   auto endDesiredPos = desiredPos.end();
   auto itrPos = pos.begin();
   auto endPos = pos.end();

   // Calcul position dt for each motor
   for ( ; itrDesiredPos != endDesiredPos || itrPos != endPos; itrDesiredPos++, itrPos++ )
   {
      posDt.push_back( (*itrDesiredPos - *itrPos)/(msTotalTime/msDt) );
   }

   // Loop to add position dt to motor position and send command to motor
   for (double t = 0.0; t < msTotalTime; t+=msDt)
   {
      std::transform(pos.begin(), pos.end(), posDt.begin(), pos.begin(), std::plus<double>());

      SetPositions(pos, config);
      WriteAll();
      usleep(msDt*1000);
   }
   return true;
}

bool MotorControl::SetPosition(double pos, std::string name)
{  
    if (_motors.find(name) == _motors.end())
    {
        Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"SetPosition\" : Motor " << name << " invalid." << std::endl;
        return false;
    }

    _motors[name]->setPos(pos);
    return true;  
}

bool MotorControl::SetPositions(const std::vector<double>& pos, const Config config)
{
   if (_configurations.find(config) == _configurations.end())
   {
       Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"SetPositions\" : Configuration is invalid." << std::endl;
       return false;
   }
   
   if (pos.size() != _configurations[config].size())
   {
#ifdef DEBUG_TEST_MOTION
       Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"SetPositions\" : Joint number is invalid." << std::endl;
#endif
       return false;
   }

   bool status = true;

#ifdef DANGER_TEST_MOTION   
   auto itrJoint = _configurations[config].begin();
   const auto endJoint = _configurations[config].end();
   auto itrPos = pos.begin();
   const auto endPos = pos.end();

   for ( ; itrJoint != endJoint && itrPos != endPos && status ; itrJoint++, itrPos++ )
   {
       (*itrJoint)->setPos(*itrPos);
   }
#endif

#ifdef DEBUG_TEST_MOTION
      Logger::getInstance(Logger::LogLvl::DEBUG) << "";
      std::copy(pos.begin(), pos.end(), std::ostream_iterator<double>(std::cout, " "));
      Logger::getInstance(Logger::LogLvl::DEBUG) << std::endl;
#endif
   return status;
}

const double MotorControl::ReadPosition(std::string name)
{
    if (_motors.find(name) == _motors.end())
    {
        Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"ReadPosition\" : Motor " << name << " invalid." << std::endl;
        return -1;
    }
    return _motors[name]->getPos();
}

bool MotorControl::ReadPositions(std::vector<double>& pos, const Config config)
{
   if (_configurations.find(config) == _configurations.end())
   {
       Logger::getInstance(Logger::LogLvl::ERROR) << "In function \"ReadPositions\" : Configuration is invalid." << std::endl;
       return false;
   }

   bool status = true;
   auto itr = _configurations[config].begin();
   const auto end = _configurations[config].end();

   for ( ; itr != end && status ; itr++ )
   {
       pos.push_back((*itr)->getPos());
   }
   return status;
}

void MotorControl::ReadAll()
{
    for (auto it = _motors.begin(); it != _motors.end(); ++it)
    {
        it->second->Read();
    }
}

void MotorControl::WriteAll()
{
    for (auto it = _motors.begin(); it != _motors.end(); ++it)
    {
        it->second->Write();
    }
}