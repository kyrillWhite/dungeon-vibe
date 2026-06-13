#include <iostream>
#include <exception>

#include "Application.h"

int main()
{
    try
    {
        Application app;

        if (!app.init())
        {
            std::cerr << "Engine initialization failed." << std::endl;
            return -1;
        }

        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal exception caught: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cerr << "An unknown fatal error occurred." << std::endl;
        return -1;
    }

    return 0;
}
