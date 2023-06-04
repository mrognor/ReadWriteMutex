#include <thread>
#include <mutex>
#include <iostream>
#include <atomic>
#include <condition_variable>

#if defined WIN32 || defined _WIN64
#include <Windows.h>
#define Sleep(sec) Sleep(sec * 1000)
#else
#include <unistd.h>
#define Sleep(sec) sleep(sec)
#endif



/**
    \brief A class for synchronizing threads

    The work of this class can be represented using a pedestrian crossing. 
    For simplicity, you can imagine the work of this class on a two-lane road, 
    but it can work with any number of roads. 
    Cars can drive on two lanes of the same road completely independently of each other.  
    At the same time, there is a pedestrian crossing on the road, and when a pedestrian approaches it, 
    cars will no longer be able to enter the road, 
    the pedestrian will wait until all the cars that were on the road when he came will finish their movement. 
    After that, he will cross the road and cars will be able to move on the road again. 
    Now let's move on to threads. You have a lot of threads that can run in parallel, 
    i.e. they don't have to be synchronized with each other. 
    And there is one thread that should only work when all other threads are not working. 
    For example, when working with some container. For simplicity, 
    let's imagine that we are working with a vector in which there are 2 elements. 
    2 threads independently work with different elements of the vector. 
    At the same time, you want to change the size of the vector. This class can help you with this. 
    Working with reading and writing to vector cells is machines, and resizing a vector is a pedestrian
 */
class ThreadCrossWalk
{
private:
    std::mutex Mtx, RoadMtx;
    std::atomic_int RoadCounter;
    std::atomic_bool IsCondVarWaiting;
    std::condition_variable Cv;
public:
    ThreadCrossWalk() 
    { 
        RoadCounter.store(0);
        IsCondVarWaiting.store(false);
    }

    /// If the PedestrianStartCrossRoad method was called before, 
    /// then this method will wait until the PedestrianStopCrossRoad method is called
    void CarStartCrossRoad()
    {
        Mtx.lock();
        RoadMtx.lock();
        RoadCounter.fetch_add(1);
        RoadMtx.unlock();
        Mtx.unlock();
    }

    void CarStopCrossRoad()
    {
        RoadMtx.lock();
        RoadCounter.fetch_sub(1);

        if (RoadCounter.load() == 0)
            while (IsCondVarWaiting.load()) Cv.notify_all();
        
        RoadMtx.unlock();
    }

    /// If the CarStartCrossRoad methods were called before, 
    /// then this method will wait until the last of the CarStopCrossRoad methods is called
    void PedestrianStartCrossRoad()
    {
        std::mutex mtx;
        std::unique_lock<std::mutex> lk(mtx);

        Mtx.lock();
        
        IsCondVarWaiting.store(true);

        if (RoadCounter > 0) Cv.wait(lk);
        
        IsCondVarWaiting.store(false);
    }

    void PedestrianStopCrossRoad()
    {
        Mtx.unlock();
    }
};

ThreadCrossWalk Wk;

void Road1()
{
    for (int i = 0; i < 10; ++i)
    {
        Wk.CarStartCrossRoad();
        std::cout << "Car 1 start crossing road" << std::endl;
        Sleep(5);
        std::cout << "Car 1 stop crossing road" << std::endl;
        Wk.CarStopCrossRoad();
    }
}

void Road2()
{
    for (int i = 0; i < 10; ++i)
    {
        Wk.CarStartCrossRoad();
        std::cout << "Car 2 start crossing road" << std::endl;
        Sleep(3);
        std::cout << "Car 2 stop crossing road" << std::endl;
        Wk.CarStopCrossRoad();
    }
}

void Road3()
{
    for (int i = 0; i < 10; ++i)
    {
        Wk.CarStartCrossRoad();
        std::cout << "Car 3 start crossing road" << std::endl;
        Sleep(4);
        std::cout << "Car 3 stop crossing road" << std::endl;
        Wk.CarStopCrossRoad();
    }
}

void Pedestrian()
{
    Wk.PedestrianStartCrossRoad();
    std::cout << "Pedestrian start crossing road" << std::endl;
    Sleep(10);
    std::cout << "Pedestrian stop crossing road" << std::endl;
    Wk.PedestrianStopCrossRoad();
}

int main()
{
    std::thread th1(Road1);
    std::thread th2(Road2);
    std::thread th3(Road3);
    std::thread th4(Pedestrian);

    Sleep(12);
    std::thread th5(Pedestrian);
    std::thread th6(Pedestrian);

    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();
    th6.join();
}