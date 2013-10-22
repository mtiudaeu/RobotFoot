/////////////////////////////////////////////////////////////////////
//
// Class ThreadManager
// Author : Mickael Paradis
// Date : October 11
// Description : Class managing threads and their priorities.
//
/////////////////////////////////////////////////////////////////////
#ifndef THREADMANAGER_H
#define THREADMANAGER_H
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>
#include <string>

class ThreadManager
{
    public:
        ThreadManager(boost::asio::io_service &boost_io);
        ~ThreadManager();

        enum Task
        {
            UNKNOW = 0,
            LEGS_CONTROL,
            HEAD_CONTROL,
            MOTOR_CONTROL
        };

        boost::thread::id create(unsigned int priority, const boost::function0<void>& thread_func, Task task = Task::UNKNOW);
        
        void stop(boost::thread::id thread_id);
        void stop(Task task);

        void attach(boost::thread::id thread_id);
        void attach(Task task);

        void resume(boost::thread::id thread_id);
        void resume(Task task);

        void wait();
        int calculate_the_answer_to_life_the_universe_and_everything();     
    private:
        void timer();
        boost::asio::deadline_timer _timer;

        std::map<boost::thread::id, boost::thread*> _threads;
        std::map<boost::thread::id, boost::condition_variable*> _cond_variables;
        std::map<Task, boost::thread::id> _tasks; 
};
#endif