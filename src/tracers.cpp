#include <memory>
#include <optional>

#include "portfolio.h"


optional<shared_ptr<PortfolioTracer>> PortfolioHistory::get_tracer(PortfolioTracerType tracer_type){
    auto it = std::find_if(
        this->tracers.begin(),
        this->tracers.end(), 
            [tracer_type](auto tracer) { return tracer->tracer_type() == tracer_type; });
    if(it == this->tracers.end())
    {
        return nullptr;
    }
    else{
        return *it;
    }
};

void PortfolioHistory::build(size_t portfolio_eval_length)
{
    for(auto& tracer : this->tracers)
    {
        tracer->build(portfolio_eval_length);
    }
}

void PortfolioHistory::reset()
{
    for(auto& tracer : this->tracers)
    {
        tracer->reset();
    }
}

void PortfolioHistory::update(){
    for(auto& tracer : this->tracers)
    {
        tracer->step();
    }
}