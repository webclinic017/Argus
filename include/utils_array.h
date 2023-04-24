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
inline py::array_t<T> to_py_array(T const * data, long length, bool read_only)
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
bool array_eq(T const * a, T const * b, size_t length){
    for(size_t i = 0; i< length; i++){
        if(a[i] != b[i]){
            return false;
        }
    }
    return true;
}

template<class T>
tuple<T* , int> sorted_union(T const * a, T const* b, size_t n, size_t m) {
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

/**
 * Returns a sorted array of each each element's child array
 *
 * @param hash_map the container holding the elements to iterate over
 * @return pointer to dynamically allocated array
 *
 * @tparam Hashmap The type of the container. It must support iteration over its values.
 * @tparam IndexLoc function to call on container elements to get array location
 * @tparam IndexLen function to call on container elements to get array length
 */
template<typename Hashmap, typename IndexLoc, typename IndexLen>
tuple<long long* , int> inline container_sorted_union(
        Hashmap& hash_map,
        IndexLoc index_loc,
        IndexLen index_len) {
    //allocate location for new sorted array
    auto* sorted_array = new long long[0];
    size_t length = 0;

    for(const auto & it : hash_map) {
        auto element = it.second;

        if(length == index_len(element)){
            if(array_eq( sorted_array,index_loc(element),length)){
                continue;
            }
        }
        //get sorted union of the two datetime indecies
        auto sorted_index_tuple = sorted_union(
                sorted_array, index_loc(element),
                length,       index_len(element));

        auto sorted_index = get<0>(sorted_index_tuple);
        auto sorted_index_size = get<1>(sorted_index_tuple);

        //swap pointers between the new sorted union and the existing one
        std::swap(sorted_array , sorted_index);
        length = sorted_index_size;

        delete [] sorted_index;
    }
    return std::make_tuple(sorted_array, length);
}

#endif //ARGUS_UTILS_ARRAY_H
