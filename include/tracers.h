#include <algorithm>
#include <memory.h>


template <typename T, typename TT> 
shared_ptr<T> gen_get_tracer(vector<shared_ptr<T>> tracer_container, TT tracer_type){
    auto it = std::find_if(
        tracer_container.begin(),
        tracer_container.end(), 
            [tracer_type](auto tracer) { return tracer->tracer_type() == tracer_type; });
    
    if(it != tracer_container.end())
    {
        return *it;
    }
    else
    {
        return nullptr;
    }
};