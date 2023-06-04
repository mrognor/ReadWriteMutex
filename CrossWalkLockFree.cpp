#include <thread>
#include <mutex>
#include <iostream>
#include <map>
#include <atomic>

#include <unistd.h>

class ThreadCrossWalk
{
private:
    std::mutex Mtx;
    std::atomic_int AtomicCounter;

public:
    ThreadCrossWalk() 
    {
        AtomicCounter.store(0);
    }

    void CarStartCrossRoad()
    {
        Mtx.lock();
        AtomicCounter.fetch_add(1);
        Mtx.unlock();
    }

    void CarStopCrossRoad()
    {
        AtomicCounter.fetch_sub(1);
    }

    void PedestrianStartCrossRoad()
    {
        Mtx.lock();
        while (AtomicCounter.load() != 0);
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
        usleep(5000000);
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
        usleep(1300000);
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
        usleep(2100000);
        std::cout << "Car 3 stop crossing road" << std::endl;
        Wk.CarStopCrossRoad();
    }
}

void Pedestrian()
{
    Wk.PedestrianStartCrossRoad();
    std::cout << "Pedestrian start crossing road" << std::endl;
    usleep(5000000);
    std::cout << "Pedestrian stop crossing road" << std::endl;
    Wk.PedestrianStopCrossRoad();
}


int main()
{
    for (int i = 0; i < 1; ++i)
    {
        std::thread th1(Road1);
        std::thread th2(Road2);
        std::thread th3(Road3);
        usleep(300);
        std::thread th4(Pedestrian);

        th1.join();
        th2.join();
        th3.join();
        th4.join();
        // std::cout << i << std::endl;
    }
}
