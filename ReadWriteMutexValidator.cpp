#include <iostream>
#include <mutex>


class MutexValidator
{
private:
    
    std::recursive_mutex* Mtx = nullptr;
    std::size_t* Counter = nullptr;
    bool* IsValid = nullptr;
    bool IsOriginal;

public:
    MutexValidator()
    {
        Mtx = new std::recursive_mutex;
        Counter = new std::size_t(1);
        IsValid = new bool(true);

        IsOriginal = true;
    }

    MutexValidator(const MutexValidator& other)
    {
        other.Mtx->lock();

        Mtx = other.Mtx;
        Counter = other.Counter;
        IsValid = other.IsValid;

        ++*Counter;
        IsOriginal = false;

        other.Mtx->unlock();
    }

    MutexValidator& operator=(const MutexValidator& other)
    {
        if (this != &other)
        {
            // Clear old
            Mtx->lock();


            --*Counter;
            if (*Counter == 0)
            {
                delete Mtx;
                delete Counter;
                delete IsValid;
            }
        
            Mtx->unlock();

            // Set new
            other.Mtx->lock();

            Mtx = other.Mtx;
            Counter = other.Counter;
            IsValid = other.IsValid;

            ++*Counter;
            IsOriginal = false;

            other.Mtx->unlock();
        }

        return *this;
    }

    // Must be inside Lock and Unlock
    bool GetIsValid()
    {
        return *IsValid;
    }

    void Lock()
    {
        Mtx->lock();
    }

    bool TryLock()
    {
        return Mtx->try_lock();
    }

    void Unlock()
    {
        Mtx->unlock();
    }

    ~MutexValidator()
    {
        Mtx->lock();

        if(IsOriginal)
            *IsValid = false;
        
        --*Counter;
        
        if (*Counter == 0)
        {
            delete Mtx;
            delete Counter;
            delete IsValid;
        }

        Mtx->unlock();
    }
};

int main()
{
    MutexValidator* mwvp = new MutexValidator;
    MutexValidator mwv(*mwvp);

    mwv.Lock();
    std::cout << mwv.GetIsValid() << std::endl;
    mwv.Unlock();

    delete mwvp;

    mwv.Lock();
    std::cout << mwv.GetIsValid() << std::endl;
    mwv.Unlock();
}