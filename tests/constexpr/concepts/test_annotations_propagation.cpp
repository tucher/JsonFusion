#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/options.hpp>
using namespace JsonFusion;
using namespace JsonFusion::options;


static_assert(std::is_same_v<
    options::detail::annotation_meta_getter<
        Annotated<int, key<"">, not_json>
    >::OptionsP, 
      OptionsPack<key<"">, not_json>
>);


struct TestStruct {
    Annotated<int, key<"">, not_json> field1;
};
static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        TestStruct, 0
    >, 
    options::detail::field_options<OptionsPack<key<"">, not_json>>
>);



struct Vec {
    float x = 1, y = 2, z = 3;
};
template<> struct JsonFusion::Annotated<Vec> {
    using Options = OptionsPack<
            key<"">, not_json
        >;
};

template<> struct JsonFusion::AnnotatedField<Vec, 1> {
    using Options = OptionsPack<
        key<"">,not_json
        >;
};


static_assert(std::is_same_v<
    options::detail::annotation_meta_getter<
        Vec
    >::OptionsP, 
      OptionsPack<key<"">, not_json>
>);

static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        Vec, 1
    >, 
    options::detail::field_options<OptionsPack<key<"">, not_json>>
>);



