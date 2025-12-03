#include "bench_matrix.hpp"
#include "benchmarks_models.hpp"
#include "benchmarks_rapidjson_tu.hpp"
#include "benchmarks_jsonfusion_tu.hpp"

using namespace json_fusion_benchmarks;


using LibsToTest    = Libraries<
    JF
    , RapidJSON
>;
using ConfigsToTest = Configs<
    EmbeddedConfigSmall
    ,TelemetrySample
    ,RPCCommand
    ,LogEvent
    ,BusEvents_MessagePayloads
    ,MetricsTimeSeries
>;

int main() {
    return run<LibsToTest, ConfigsToTest>();
}


/*

Results for Apple M1 Max:

=======Model EmbeddedConfig/small======= iterations: 1000000
  Static containers
                    JsonFusion      insitu        1.01 us/iter   
                    JsonFusion  non_insitu        1.00 us/iter   
                     RapidJSON      insitu        1.36 us/iter   
                     RapidJSON  non_insitu        1.12 us/iter   
  Dynamic containers
                    JsonFusion      insitu        1.24 us/iter   
                    JsonFusion  non_insitu        1.23 us/iter   
                     RapidJSON      insitu        2.10 us/iter   
                     RapidJSON  non_insitu        2.13 us/iter   

=========Model TelemetrySample========== iterations: 1000000
                    JsonFusion      insitu        4.89 us/iter   
                    JsonFusion  non_insitu        5.09 us/iter   
                     RapidJSON      insitu        7.79 us/iter   
                     RapidJSON  non_insitu        7.13 us/iter   

===========Model RPC Command============ iterations: 1000000
                    JsonFusion      insitu        2.35 us/iter   
                    JsonFusion  non_insitu        2.35 us/iter   
                     RapidJSON      insitu        3.89 us/iter   
                     RapidJSON  non_insitu        4.57 us/iter   

============Model Log events============ iterations: 1000000
                    JsonFusion      insitu        3.39 us/iter   
                    JsonFusion  non_insitu        3.34 us/iter   
                     RapidJSON      insitu        4.83 us/iter   
                     RapidJSON  non_insitu        5.09 us/iter   

==Model Bus Events / Message Payloads=== iterations: 1000000
                    JsonFusion      insitu        4.92 us/iter   
                    JsonFusion  non_insitu        4.87 us/iter   
                     RapidJSON      insitu        7.38 us/iter   
                     RapidJSON  non_insitu        8.37 us/iter   

==Model Metrics / Time-Series Samples=== iterations: 1000000
                    JsonFusion      insitu        3.98 us/iter   
                    JsonFusion  non_insitu        3.97 us/iter   
                     RapidJSON      insitu        5.78 us/iter   
                     RapidJSON  non_insitu        5.60 us/iter  

*/
