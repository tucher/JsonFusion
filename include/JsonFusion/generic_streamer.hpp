#pragma once
#include <JsonFusion/static_schema.hpp>
namespace JsonFusion {
namespace streamers {
    
template <class ValueT>
struct CountingStreamer {
    using value_type = ValueT;

    std::size_t counter = 0;

    void reset()  {counter = 0;}

    // Called for each element, with a fully-parsed value_type
    bool consume(const ValueT & )  {
        counter ++;
        return true;
    }
    bool finalize(bool success)  { return true; }
};


}
}