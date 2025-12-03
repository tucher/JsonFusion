#include "benchmarks_models.hpp"
#include "JsonFusion/parser.hpp"


using namespace json_fusion_benchmarks;




JsonFusion::ParseResultT<EmbeddedConfigSmall::EmbeddedConfigStatic, char const*>
JsonFusionParseEmbeddedConfigStatic(EmbeddedConfigSmall::EmbeddedConfigStatic& model, const char*begin,  const char* const& end) {
    return JsonFusion::Parse(model, begin, end);
}





// template bool JF::parse_validate_and_populate<EmbeddedConfigSmall::EmbeddedConfigDynamic>(EmbeddedConfigSmall::EmbeddedConfigDynamic& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<TelemetrySample::SamplesDynamic>(TelemetrySample::SamplesDynamic& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<RPCCommand::TopLevel>(RPCCommand::TopLevel& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<LogEvent::Entry>>(vector<LogEvent::Entry>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<BusEvents_MessagePayloads::Event>>(vector<BusEvents_MessagePayloads::Event>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<MetricsTimeSeries::MetricSample>>(vector<MetricsTimeSeries::MetricSample>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);




// template bool JF::parse_validate_and_populate<EmbeddedConfigSmall::EmbeddedConfigDynamic>(EmbeddedConfigSmall::EmbeddedConfigDynamic& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<TelemetrySample::SamplesDynamic>(TelemetrySample::SamplesDynamic& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<RPCCommand::TopLevel>(RPCCommand::TopLevel& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<LogEvent::Entry>>(vector<LogEvent::Entry>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<BusEvents_MessagePayloads::Event>>(vector<BusEvents_MessagePayloads::Event>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);
// template bool JF::parse_validate_and_populate<vector<MetricsTimeSeries::MetricSample>>(vector<MetricsTimeSeries::MetricSample>& out, const std::string& data, bool insitu /*not needed*/, std::string& remark);


