#include <iostream>
#include <functional>
#include <vector>

class Event
{
public:
    void Add(const std::function<void()>& callback)
    {
        listeners.push_back(callback);
    }

    void Invoke()
    {
        for (auto& callback : listeners)
            callback();
    }

private:
    std::vector<std::function<void()>> listeners;
};
