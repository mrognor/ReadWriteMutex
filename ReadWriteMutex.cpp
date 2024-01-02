#include <thread>
#include <mutex>
#include <iostream>
#include <atomic>
#include <condition_variable>


/**
    \brief A class for synchronizing threads

    A class for thread management that allows you to lock sections of code for reading or writing.
    A write lock will ensure that there can only be one thread in the code section at a time.
    The read lock will ensure that no thread using the write lock gets into the code section until all threads using the read lock are unblocked.
    At the same time, after the write lock, no new threads with a read lock will enter the code section until all threads using the write lock are unblocked.
 */
class ReadWriteMutex
{
private:
    std::mutex WriteMutex, ReadMutex;
    std::atomic_int ReadCounter;
    std::atomic_bool IsCondVarWaiting;
    std::condition_variable Cv;
public:
    /// \brief Default constructor
    ReadWriteMutex() 
    { 
        ReadCounter.store(0);
        IsCondVarWaiting.store(false);
    }

    /**
        \brief A method for locking a section of code for reading

        Using this method, you can lock the code section for reading, which means that all threads using the read lock will have access to data inside the code section
        but threads using the write lock will wait until all read operations are completed.
    */
    void ReadLock()
    {
        WriteMutex.lock();
        ReadMutex.lock();
        ReadCounter.fetch_add(1);
        ReadMutex.unlock();
        WriteMutex.unlock();
    }

    /// \brief A method for unlocking a section of code for reading
    void ReadUnlock()
    {
        ReadMutex.lock();
        ReadCounter.fetch_sub(1);

        if (ReadCounter.load() == 0)
            while (IsCondVarWaiting.load()) Cv.notify_all();
        
        ReadMutex.unlock();
    }

    /**
        \brief A method for locking a section of code for writing

        This method provides exclusive access to a section of code for a single thread.
        All write operations will be performed sequentially.
        This method takes precedence over the read lock, which means that after calling this method, no new read operations will be started.
    */
    void WriteLock()
    {
        std::mutex mtx;
        std::unique_lock<std::mutex> lk(mtx);

        WriteMutex.lock();
        
        IsCondVarWaiting.store(true);

        if (ReadCounter > 0) Cv.wait(lk);
        
        IsCondVarWaiting.store(false);
    }


    /// \brief A method for unlocking a section of code for writing
    void WriteUnlock()
    {
        WriteMutex.unlock();
    }
};


// Demo
#include <vector>

ReadWriteMutex Rwx;
std::vector<int> Vec;
std::condition_variable Cv;
auto const timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);

void ReadingFunc()
{
    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    
    // Wait for start all threads at same time
    Cv.wait_until(lk, timeout);

    for (int i = 0; i < 1000; ++i)
    {
        int summ = 0;
        Rwx.ReadLock();
        for (const auto& it : Vec)
            summ += it;
        Rwx.ReadUnlock();
    }
}

void WritingFunc()
{
    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    
    // Wait for start all threads at same time
    Cv.wait_until(lk, timeout);

    for (int i = 0; i < 1000; ++i)
    {
        Rwx.WriteLock();
        Vec.emplace_back(i);
        Rwx.WriteUnlock();
    }
}

int main()
{


    std::thread th1(WritingFunc);
    std::thread th2(WritingFunc);
    std::thread th3(ReadingFunc);

    th1.join();
    th2.join();
    th3.join();


    int realSumm = 0;
    
    for (int i = 0; i < 1000; ++i)
        realSumm += i;
    
    for (int i = 0; i < 1000; ++i)
        realSumm += i;

    int summ = 0;
    for (const auto& it : Vec)
        summ += it;

    if (summ != realSumm)
        std::cout << "Error! Summ = " << summ << " Real summ: " << realSumm << std::endl;
    else
        std::cout << "Ok" << std::endl;
}