//
// Created by Nathan Tormaschy on 4/20/23.
//

#ifndef ARGUS_UTILS_ARRAY_H
#define ARGUS_UTILS_ARRAY_H
#include <queue>
#include <span>
#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

using namespace std;

namespace py = pybind11;

template<typename T, typename Func>
inline T unsorted_vector_remove(vector<T> &vec, Func func, unsigned int id){
    size_t index = 0;
    bool found_item = false;
    for(const auto& item : vec){
        if(func(item) == id){
            found_item = true;
            break;
        }
        else{
            index++;
        }
    }
    if(!found_item){
        throw std::runtime_error("failed to find item id in container");
    }
    else {
        std::swap(vec[index], vec.back());
    }
    auto order =  vec.back();
    vec.pop_back();
    return order;
}

template<typename T>
inline py::array_t<T> to_py_array(T* data, long length, bool read_only)
{
    auto capsule = py::capsule(data, [](void *data) {});
    auto array =  py::array_t<T> {
        length,
        data,
        capsule
    };
    if(read_only) {
        reinterpret_cast<py::detail::PyArray_Proxy *>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
    }
    return array;
}

template<class T>
bool array_eq(T* a, T* b, size_t length){
    for(size_t i = 0; i< length; i++){
        if(a[i] != b[i]){
            return false;
        }
    }
    return true;
}

template<class T>
tuple<T* , int> sorted_union(T* a, T* b, size_t n, size_t m) {
    int i = 0, j = 0, k = 0;
    auto* result = new T[n + m];
    priority_queue<T, vector<T>, greater<>> pq;

    while (i < n && j < m) {
        if (a[i] < b[j]) {
            pq.push(a[i]);
            i++;
        } else if (b[j] < a[i]) {
            pq.push(b[j]);
            j++;
        } else {
            pq.push(a[i]);
            i++;
            j++;
        }
    }

    while (i < n) {
        pq.push(a[i]);
        i++;
    }

    while (j < m) {
        pq.push(b[j]);
        j++;
    }

    while (!pq.empty()) {
        result[k] = pq.top();
        pq.pop();
        k++;
    }

    // Shrink the memory block to the actual size needed
    auto* temp = (T*)realloc(result, k * sizeof(T));
    if (temp != nullptr) {
        result = temp;
    } else{
        throw std::runtime_error("failed to realloc array");
    }
    return std::make_tuple(result, k);
}

#endif //ARGUS_UTILS_ARRAY_H
