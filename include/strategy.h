
#include <functional>
#include <string>

#include "pybind11/gil.h"

namespace py = pybind11;


class Strategy
{
public:
    std::function<void()> python_handler_on_open;
    std::function<void()> cxx_handler_on_open;

    std::function<void()> python_handler_on_close;
    std::function<void()> cxx_handler_on_close;

    Strategy()
    {
        cxx_handler_on_open = [this](void) { return python_handler_on_open(); };
        cxx_handler_on_close = [this](void) {
            // call python object's on_close() method to generate orders
            return python_handler_on_close(); 

            //release gil 
            py::gil_scoped_release release;
        };
    }
};