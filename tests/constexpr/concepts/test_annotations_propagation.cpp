#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/validators.hpp>
using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;


static_assert(std::is_same_v<
    options::detail::annotation_meta_getter<
        Annotated<int, key<"">, exclude>
    >::OptionsP, 
      OptionsPack<key<"">, exclude>
>);


struct TestStruct {
    Annotated<int, key<"">, exclude> field1;
};
static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        TestStruct, 0
    >, 
    options::detail::field_options<OptionsPack<key<"">, exclude>>
>);


struct Vec {
    float x = 1, y = 2, z = 3;
};
template<> struct JsonFusion::Annotated<Vec> {
    using Options = OptionsPack<
            key<"">, exclude
        >;
};

template<> struct JsonFusion::AnnotatedField<Vec, 1> {
    using Options = OptionsPack<
        key<"">,exclude
        >;
};


static_assert(std::is_same_v<
    options::detail::annotation_meta_getter<
        Vec
    >::OptionsP, 
      OptionsPack<key<"">, exclude>
>);
static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        Vec, 0
    >, 
    options::detail::field_options<OptionsPack<>>
>);
static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        Vec, 1
    >, 
    options::detail::field_options<OptionsPack<key<"">, exclude>>
>);
static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        Vec, 2
    >, 
    options::detail::field_options<OptionsPack<>>
>);


struct Motor {
    double position[3];
    bool active;
    char name[20];
};

template<>
struct JsonFusion::StructMeta<Motor> {
    using Fields = StructFields<
        Field<&Motor::position, "position", min_items<3>>,         // double position[3];
        Field<&Motor::active, "active">,                      //without annotations!  // bool active;
        Field<&Motor::name, "name",  min_length<3>>     // char name[20];
    >;
};
template<> struct JsonFusion::Annotated<Motor> {
    using Options = OptionsPack<
            key<"">, exclude
        >;
};

static_assert(introspection::detail::has_struct_meta_specialization<Motor>);
static_assert(introspection::structureElementsCount<Motor> == 3);

static_assert(static_schema::ParsableArrayLike<introspection::structureElementTypeByIndex<0, Motor>>);
static_assert(static_schema::BoolLike<introspection::structureElementTypeByIndex<1, Motor>>);
static_assert(static_schema::StringLike<introspection::structureElementTypeByIndex<2, Motor>>);


static_assert(introspection::structureElementNameByIndex<0, Motor> == "position");
static_assert(introspection::structureElementNameByIndex<1, Motor> == "active");
static_assert(introspection::structureElementNameByIndex<2, Motor> == "name");

static_assert(std::is_same_v<
    options::detail::annotation_meta_getter<
        Motor
    >::OptionsP, 
      OptionsPack<key<"">, exclude>
>);

static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        Motor, 0
    >, 
    options::detail::field_options<OptionsPack<min_items<3>>>
>);

static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        Motor, 1
    >, 
    options::detail::field_options<OptionsPack<>>
>);

static_assert(std::is_same_v<
    options::detail::aggregate_field_opts_getter<
        Motor, 2
    >, 
    options::detail::field_options<OptionsPack<min_length<3>>>
>);