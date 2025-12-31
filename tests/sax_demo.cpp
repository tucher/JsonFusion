#include <string_view>
#include <cassert>
#include <iostream>
#include <format>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>


struct MyStruct {

};

template<> struct JsonFusion::Annotated<MyStruct> {
    using Options = OptionsPack<JsonFusion::options::exclude>;
};

using BadAnnotated = JsonFusion::Annotated<int>;
BadAnnotated t_;
auto a = t_.value;

JsonFusion::Annotated<bool> bv;
auto b = bv.value;


struct T2 {
    double f;
};

template<> struct JsonFusion::Annotated<T2> {
    using Options = OptionsPack<
        // JsonFusion::validators::range<0, M_PI>
        >;
};

T2 tt;
auto tv = tt.f;



using MyAnnotatedStruct = JsonFusion::Annotated<MyStruct, JsonFusion::options::exclude>;

static_assert(JsonFusion::options::detail::annotation_meta_getter<MyAnnotatedStruct>::options
              ::template has_option<JsonFusion::options::detail::exclude_tag>);
static_assert(JsonFusion::options::detail::annotation_meta_getter<MyStruct>::options
              ::template has_option<JsonFusion::options::detail::exclude_tag>);


struct Vec_WithExternalMeta{
    float x, y, z;
};


static_assert(JsonFusion::introspection::detail
              ::index_for_member_ptr<Vec_WithExternalMeta, &Vec_WithExternalMeta::y>() == 1);


template<> struct JsonFusion::Annotated<Vec_WithExternalMeta> {
    using Options = OptionsPack<
        JsonFusion::options::as_array
        >;
};


template<> struct JsonFusion::AnnotatedField<Vec_WithExternalMeta, 1> {
    using Options = OptionsPack<
        JsonFusion::options::exclude
        >;
};


static_assert(JsonFusion::options::detail::has_field_annotation_specialization_impl<Vec_WithExternalMeta, 1>::value);
using TstOpts = JsonFusion::options::detail::aggregate_field_opts_getter<Vec_WithExternalMeta, 1>;



static_assert(TstOpts::template has_option<JsonFusion::options::detail::exclude_tag>);

template <class ExternalVector = void>
struct Streamer {
    struct Vector{
        float x;
        // JsonFusion::Annotated<float, JsonFusion::options::exclude> y;
        float y;
        float z;
    };

    // Each element will be serialized as a JSON array [x,y,z]
    using VT =  std::conditional_t<
        std::is_same_v<ExternalVector, void>,
        Vector,
        Vec_WithExternalMeta
        >;
    using value_type = std::conditional_t<
        std::is_same_v<ExternalVector, void>,
        JsonFusion::Annotated<Vector, JsonFusion::options::as_array>,
        Vec_WithExternalMeta
        >;

    mutable std::size_t * count;
    mutable int counter = 0;

    // Called by JsonFusion to pull the next element.
    // Returns:
    //   value  – v has been filled, keep going
    //   end    – no more elements
    //   error  – abort serialization

    constexpr JsonFusion::stream_read_result read(VT & v) const {
        if (counter >= *count) {
            return JsonFusion::stream_read_result::end;
        }
        counter ++;
        v.x = 42 + counter;
        v.y = 43 + counter;
        v.z = 44 + counter;
        return JsonFusion::stream_read_result::value;
    }

    // Called at the start of the JSON array
    void reset() const {
        counter = 0;
    }
    void set_jsonfusion_context(std::size_t * c) const {
        count = c;
    }
};
static_assert(JsonFusion::ProducingStreamerLike<Streamer<>>, "Incompatible streamer");
void streaming_demo () {


    {
        struct TopLevel {
            Streamer<> points_xyz;
        };
        TopLevel a;

        std::size_t count =3;

        std::string out;
        JsonFusion::Serialize(a, out, &count);

        std::cout << out << std::endl;
    }
    {
        struct TopLevel {
            Streamer<Vec_WithExternalMeta> points_xyz;
        };
        TopLevel a;

        std::size_t count =3;

        std::string out;
        JsonFusion::Serialize(a, out, &count);

        std::cout << out << std::endl;
    }

    /* Output:

    {"points_xyz":[[43,44,45],[44,45,46],[45,46,47]]}

    */  
}

void sax_demo() {
    using JsonFusion::Annotated;
    using namespace JsonFusion::options;


    struct VectorWithTimestamp{
        struct Vector{
            float x, y, z;
        };
        Annotated<Vector, as_array> pos;
        Annotated<std::uint64_t> timestamp;
    };

    struct PointsConsumer {
        using value_type = Annotated<VectorWithTimestamp, as_array>;

        void  printPoint (const VectorWithTimestamp & point) {
            std::cout << std::format("Point received: t={}, pos=({},{},{})",
                                        point.timestamp.value,
                                        point.pos->x,
                                        point.pos->y,
                                        point.pos->z
                                        )
                        << std::endl;
        };
        // Called at the start of the JSON array
        void reset()  {
            std::cout << "Receiving points" << std::endl;
        }

        // Called for each element, with a fully-parsed value_type
        bool consume(const VectorWithTimestamp & point)  {
            printPoint(point);
            return true;
        }
        bool finalize(bool success)  {
            std::cout << "All points received" << std::endl;
            return true;
        }
    };
    static_assert(JsonFusion::ConsumingStreamerLike<PointsConsumer>, "Incompatible consumer");

    struct TagsConsumer {
        struct Tag {
            std::string id;
            std::string text;
        };
        using value_type = Tag;
        
       // Called at the start of the JSON array
        void reset()  {
            std::cout << "Receiving tags" << std::endl;
        }

        // Called for each element, with a fully-parsed value_type
        bool consume(const Tag & tag)  {
            std::cout << tag.id << " " << tag.text << std::endl;
            return true;
        }

        // called once at the end (with json success flag, if true, all data was consumed successfully)
        bool finalize(bool success)  {
            if (!success) {
                std::cout << "Tags stream aborted due to parse error\n";
                return false;
            }
            std::cout << "Tags received\n";
            return true;
        }
    };
    static_assert(JsonFusion::ConsumingStreamerLike<TagsConsumer>, "Incompatible consumer");

    struct TopLevel {
        double f;
        JsonFusion::Annotated<PointsConsumer, key<"timestamped_points">> positions;
        TagsConsumer tags;  // field name "tags" -> JSON key "tags"
    };

    TopLevel a;
    std::string_view in(R"JSON(
    {
        "tags": [
            {"id": "1", "text": "first tag"},
            {"id": "2", "text": "second tag"}
        ],
        "timestamped_points": [
            [[1,2,3], 2],
            [[4,5,6], 3],
            [[7,8,9], 8]
        ],
        "f": 3.18
    })JSON");

    JsonFusion::Parse(a, in);
   


    /*
    
    Receiving tags
    1 first tag
    2 second tag
    Tags received
    Receiving points
    Point received: t=2, pos=(1,2,3)
    Point received: t=3, pos=(4,5,6)
    Point received: t=8, pos=(7,8,9)
    All points received
    
    */
}


void nested_producers () {

    struct StreamerOuter {
        struct StreamerInner {
            using value_type = double;

            int count = 5;
            mutable int counter = 0;

            constexpr JsonFusion::stream_read_result read(double & v) const {
                if (counter >= count) {
                    return JsonFusion::stream_read_result::end;
                }
                counter ++;
                v = counter;
                (*ctx_int) --;
                return JsonFusion::stream_read_result::value;
            }

            void reset() const {
                counter = 0;
            }
            mutable int * ctx_int;
            void set_jsonfusion_context(int * ctx) const {
                ctx_int = ctx;
            }
        };

        using value_type = StreamerInner;

        int count {8};
        mutable int counter { 0};

        constexpr JsonFusion::stream_read_result read(StreamerInner & v) const {
            if (counter >= count) {
                return JsonFusion::stream_read_result::end;
            }
            counter ++;
            v.count = counter;
            (*ctx_int) --;
            return JsonFusion::stream_read_result::value;
        }

        void reset() const {
            counter = 0;
        }

        mutable int * ctx_int;
        void set_jsonfusion_context(int * ctx) const {
            ctx_int = ctx;
        }
    };

    std::string out;
    int ctx = 100;
    StreamerOuter s {};
    s.ctx_int = &ctx;
    JsonFusion::Serialize(s, out, &ctx);

    std::cout << out << std::endl;
    std::cout << ctx << std::endl;

    /*
    [[1],[1,2],[1,2,3],[1,2,3,4],[1,2,3,4,5],[1,2,3,4,5,6],[1,2,3,4,5,6,7],[1,2,3,4,5,6,7,8]]
    */
}
namespace fs = std::filesystem;

std::string read_file(const fs::path& filepath) {

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error(std::format("Failed to open file: {}", filepath.string()));
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(size, '\0');
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error(std::format("Failed to read file: {}", filepath.string()));
    }

    return buffer;
}




int main(int argc, char ** argv) {
    streaming_demo();
    sax_demo();
    nested_producers();
}
