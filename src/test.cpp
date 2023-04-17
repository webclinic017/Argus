#include "../include/test.h"
#include <pybind11/pybind11.h>

namespace py = pybind11;

int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(Argus, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring
    m.def("add", static_cast<int (*)(int, int)>(add), "A function that adds two numbers", py::return_value_policy::reference);
}