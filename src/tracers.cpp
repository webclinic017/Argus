#include <memory>
#include <optional>
#include <stdexcept>

#include "portfolio.h"

void PortfolioHistory::add_tracer(PortfolioTracerType tracer_type)
{
    // test to see if tracer exists
    auto tracer_exists = this->get_tracer(tracer_type);
    if(tracer_exists.has_value())
    {
        throw std::runtime_error("tracer already exists");
    }

    switch (tracer_type) 
    {
        case PortfolioTracerType::Value:
            this->tracers.push_back(std::make_shared<ValueTracer>(this->parent_portfolio));
        case PortfolioTracerType::Event:
            this->tracers.push_back(std::make_shared<EventTracer>(this->parent_portfolio));
    }
}


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

void PortfolioHistory::reset(bool clear_history)
{
    for(auto& tracer : this->tracers)
    {
        if(tracer->tracer_type() == PortfolioTracerType::Event)
        {
            if(clear_history)
            {
                tracer.reset();
            }
        }
        else
        {
            tracer->reset();
        }
    }
}

void PortfolioHistory::update(){
    for(auto& tracer : this->tracers)
    {
        tracer->step();
    }
}