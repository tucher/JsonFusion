#pragma once
// #include "benchmarks_models.hpp"
#include "JsonFusion/error_formatting.hpp"
#include "JsonFusion/parser.hpp"

// using namespace json_fusion_benchmarks;


// JsonFusion::ParseResultT<EmbeddedConfigSmall::EmbeddedConfigStatic, char const*>
// JsonFusionParseEmbeddedConfigStatic(EmbeddedConfigSmall::EmbeddedConfigStatic& model, const char*begin,  const char* const& end);


struct JF {
    static constexpr std::string_view library_name = "JsonFusion";


    template <class Model>
    bool parse_validate_and_populate(Model& out, const std::string& data, bool insitu , std::string& remark) {
        auto res = JsonFusion::Parse(out, data.data(), data.data() + data.size());
        static_assert(std::is_same_v<decltype(data.data()),  char const *>);
        if (!res) {
            remark = JsonFusion::ParseResultToString<Model>(res, data.data(), data.data() + data.size());
            return false;
        }
        return true;
    }


    // bool parse_validate_and_populate(EmbeddedConfigSmall::EmbeddedConfigStatic& out, const std::string& data, bool insitu /*not needed*/, std::string& remark) {
    //     auto res = JsonFusionParseEmbeddedConfigStatic(out, data.data(), data.data() + data.size());
    //     static_assert(std::is_same_v<decltype(data.data()),  char const *>);
    //     if (!res) {
    //         remark = JsonFusion::ParseResultToString<EmbeddedConfigSmall::EmbeddedConfigStatic>(res, data.data(), data.data() + data.size());
    //         return false;
    //     }
    //     return true;
    // }
};



// template bool JF::parse_validate_and_populate<EmbeddedConfigSmall::EmbeddedConfigDynamic>(EmbeddedConfigSmall::EmbeddedConfigDynamic& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<TelemetrySample::SamplesDynamic>(TelemetrySample::SamplesDynamic& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<RPCCommand::TopLevel>(RPCCommand::TopLevel& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<LogEvent::Entry>>(vector<LogEvent::Entry>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<BusEvents_MessagePayloads::Event>>(vector<BusEvents_MessagePayloads::Event>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<MetricsTimeSeries::MetricSample>>(vector<MetricsTimeSeries::MetricSample>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);


