#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "Utilities/ThreadManager.h"
#include "Control/STM32F4.h"
#include "Utilities/XmlParser.h"

#include <vector>

class Motor
{
    public:  
        Motor(STM32F4 *stm32f4, std::string name, int id, int offset, int min, int max, int speed);
        ~Motor();
        void setPos(double pos);
        const double getPos();
        void setTorque(bool value);
        void Read();
        void Write();
    private:
        int Angle2Value(const double angle);
        double Value2Angle(const int value);
        
        STM32F4 *_stm32f4; 
        std::string _name;
        int _id;
        int _offset;
        int _min;
        int _max;
        int _speed;
        double _currentPos;
        double _nextPos;
};

class MotorControl
{
public:
   enum Config {
      ALL_MOTORS = 0,
      ALL_LEGS,
      RIGHT_LEG,
      LEFT_LEG,
      HEAD,
      NUM_TEST
   };

   MotorControl(ThreadManager *threadManager, const XmlParser &config);
   ~MotorControl();

   void Start();

   bool SetTorque(bool value, const Config config);
   bool SetTorque(bool value, const std::string name);
  
   bool InitPosition( const std::vector<double>& vPos,
                      const Config config,
                      const double msTotalTime = 10000.0,
                      const double msDt = 16);
   
   bool SetPosition(double pos, std::string name); 
   const double ReadPosition(std::string name);
   
   bool SetPositions(const std::vector<double>& pos, const Config config);
   bool ReadPositions(std::vector<double>& pos, const Config config);

private:
   void InitializeMotors(const XmlParser &config);
   void InitializeConfigurations(const XmlParser &config);

   void ReadAll();
   void WriteAll();

   STM32F4 *_stm32f4;
   ThreadManager *_threadManager;
    
   std::map<std::string, Motor*> _motors;
   std::map<Config, std::vector<Motor*>> _configurations;
};
#endif  //MOTOR_CONTROL_H