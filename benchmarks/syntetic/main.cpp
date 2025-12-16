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


