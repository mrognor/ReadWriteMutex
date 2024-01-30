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

        if (ReadCounter.load() > 0) Cv.wait(lk);
        
        IsCondVarWaiting.store(false);
    }

    /// \brief A method for unlocking a section of code for writing
    void WriteUnlock()
    {
        WriteMutex.unlock();
    }
};

thread_local std::size_t LocalThreadReadLockCounter = 0;
thread_local std::size_t LocalThreadWriteLockCounter = 0;

/**
    \brief A class for synchronizing threads

    A class for thread management that allows you to lock sections of code for reading or writing.
    A write lock will ensure that there can only be one thread in the code section at a time.
    The read lock will ensure that no thread using the write lock gets into the code section until all threads using the read lock are unblocked.
    At the same time, after the write lock, no new threads with a read lock will enter the code section until all threads using the write lock are unblocked.
    Recursiveness allows you to call blocking methods in the same thread multiple times without self-locking.
*/
class RecursiveReadWriteMutex
{
private:
    ReadWriteMutex Rwmx;

public:
    
    /**
        \brief A method for locking a section of code for reading

        Using this method, you can lock the code section for reading, which means that all threads using the read lock will have access to data inside the code section
        but threads using the write lock will wait until all read operations are completed.
        Note that, in fact, blocking for reading inside writing does not make sense, 
        since the code section is already locked and therefore nothing will happen inside the function in such a situation.
    */
    void ReadLock()
    {
        if (LocalThreadWriteLockCounter == 0)
        {
            if (LocalThreadReadLockCounter == 0)
                Rwmx.ReadLock();

            ++LocalThreadReadLockCounter;
        }
    }

    /// \brief A method for unlocking a section of code for reading
    void ReadUnlock()
    {
        if (LocalThreadWriteLockCounter == 0)
        {
            if (LocalThreadReadLockCounter == 1)
                Rwmx.ReadUnlock();
            
            --LocalThreadReadLockCounter;
        }
    }

    /**
        \brief A method for locking a section of code for writing

        This method provides exclusive access to a section of code for a single thread.
        All write operations will be performed sequentially.
        This method takes precedence over the read lock, which means that after calling this method, no new read operations will be started.
        Note that if the write lock is called inside the read lock, then this will be equivalent to unlocking for reading and then locking for writing.
    */
    void WriteLock()
    {
        if (LocalThreadWriteLockCounter == 0)
        {
            if (LocalThreadReadLockCounter > 0)
                Rwmx.ReadUnlock();
            
            Rwmx.WriteLock();
        }

        ++LocalThreadWriteLockCounter;
    }

    /// \brief A method for unlocking a section of code for writing
    /// Note that if the write unlock is called inside the read lock, then this will be equivalent to unlocking for writing and then locking for reading.
    void WriteUnlock()
    {
        if (LocalThreadWriteLockCounter == 1)
        {
            Rwmx.WriteUnlock();
            if (LocalThreadReadLockCounter > 0)
                Rwmx.ReadLock();
        }
        --LocalThreadWriteLockCounter;
    }
};

// Demo
#include <vector>

RecursiveReadWriteMutex Rrwmx;
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
        Rrwmx.ReadLock();
        for (const auto& it : Vec)
            summ += it;
        Rrwmx.ReadUnlock();
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
        Rrwmx.ReadLock();
        Rrwmx.WriteLock();
        Vec.emplace_back(i);
        Rrwmx.WriteUnlock();
        Rrwmx.ReadUnlock();
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

    RecursiveReadWriteMutex rrwx;
    rrwx.ReadLock();
    rrwx.ReadLock();
    rrwx.ReadUnlock();
    rrwx.ReadUnlock();

    rrwx.WriteLock();
    rrwx.WriteLock();
    rrwx.WriteUnlock();
    rrwx.WriteUnlock();
    
    rrwx.WriteLock();
    rrwx.ReadLock();
    rrwx.ReadUnlock();
    rrwx.WriteUnlock();   

    rrwx.ReadLock();
    rrwx.WriteLock();
    rrwx.WriteUnlock();
    rrwx.ReadUnlock();

    std::thread th4([&]()
        {
            rrwx.ReadLock();

            std::cout << "Th1 sleep1 started" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(20000));
            std::cout << "Th1 sleep1 ended" << std::endl;

            rrwx.WriteLock();

            std::cout << "Th1 sleep2 started" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(20000));
            std::cout << "Th1 sleep2 ended" << std::endl;

            rrwx.WriteUnlock();
            rrwx.ReadUnlock();
        });

    std::thread th5([&]()
        {
            std::cout << "Th2 sleep1 started" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
            std::cout << "Th2 sleep1 ended" << std::endl;

            rrwx.WriteLock();

            std::cout << "Th2 sleep2 started" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(20000));
            std::cout << "Th2 sleep2 ended" << std::endl;
            
            rrwx.WriteUnlock();
        });

    th4.join();
    th5.join();
}