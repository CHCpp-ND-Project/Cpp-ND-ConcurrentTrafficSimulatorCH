#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> uLock(_mtxMQ);
    _condMQ.wait(uLock, [this] {return !_queue.empty();});
    T msg = std::move(_queue.back());
    _queue.clear();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> uLock(_mtxMQ);
    _queue.emplace_back(std::move(msg));
    _condMQ.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (_queue.receive() == TrafficLightPhase::red)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public 
    // method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : 
    
    // print id of the current thread
    std::unique_lock<std::mutex> lck(_mtx);
    std::cout << "Traffic Light #" << _id << "::cycleThroughPhases: thread id = " << std::this_thread::get_id() << std::endl;
    lck.unlock();

    // initalize variables
    // Setup cycle duration uniformly distributed real number between 4 and 6 seconds
    // The cycle duration should be a random value between 4 and 6 seconds. 
    // Currently fixed on instantiation, evaluate modifying each iteration of while loop

    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> dis(4000, 6000);
    double cycleDuration = dis(eng);    // duration of a single simulation cycle in ms

    // init stop watch
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
    lastUpdate = std::chrono::system_clock::now();

    // Implement the function with an infinite loop that measures the time between two loop cycles 
    while (true)
    {
        // use std::this_thread::sleep_for to wait 1ms between two cycles. 
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        // toggles the current phase of the traffic light between red and green
        if (timeSinceLastUpdate >= cycleDuration){
            if (_currentPhase == red){
                _currentPhase = green;
            } else {
                _currentPhase = red;
            }
        // sends an update method to the message queue using move semantics. 
        lck.lock();
        std::cout << "   Traffic Light #" << _id << " status changed to " << _currentPhase << std::endl;
        lck.unlock();

        _queue.send(std::move(_currentPhase));
        _condition.notify_one(); // notify client after pushing new Vehicle into vector
        lastUpdate = std::chrono::system_clock::now();
        }
    }
    
}