#include <string_view>
#include <cassert>
#include <iostream>
#include <format>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

void streaming_demo () {

    struct Streamer {
        struct Vector{
            float x, y, z;
        };

        // Each element will be serialized as a JSON array [x,y,z]
        using value_type = JsonFusion::Annotated<Vector, JsonFusion::options::as_array>;

        int count;
        mutable int counter = 0;

        // Called by JsonFusion to pull the next element.
        // Returns:
        //   value  – v has been filled, keep going
        //   end    – no more elements
        //   error  – abort serialization

        constexpr JsonFusion::stream_read_result read(Vector & v) const {
            if (counter >= count) {
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
    };

    static_assert(JsonFusion::ProducingStreamerLike<Streamer>, "Incompatible streamer");

    struct TopLevel {
        Streamer points_xyz;
    };
    TopLevel a;

    a.points_xyz.count=3;

    std::string out;
    JsonFusion::Serialize(a, out);

    std::cout << out << std::endl;

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
        std::uint64_t timestamp;
    };

    struct PointsConsumer {
        using value_type = Annotated<VectorWithTimestamp, as_array>;

        void  printPoint (const VectorWithTimestamp & point) {
            std::cout << std::format("Point received: t={}, pos=({},{},{})",
                                        point.timestamp,
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
            void set_json_fusion_context(int * ctx) const {
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
        void set_json_fusion_context(int * ctx) const {
            ctx_int = ctx;
        }
    };

    std::string out;
    int ctx = 100;
    StreamerOuter s {};
    s.ctx_int = &ctx;
    JsonFusion::SerializeWithContext(s, out, &ctx);

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


void geojson_reader(int argc, char ** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-canada.json>\n";
        std::cerr << "Download from: https://github.com/miloyip/nativejson-benchmark/blob/master/data/canada.json\n";
        return;
    }

    fs::path json_path = argv[1];
    std::cout << "Reading file: " << json_path << "\n";
    std::string json_data = read_file(json_path);
    std::cout << std::format("File size: {:.2f} MB ({} bytes)\n\n",
                             json_data.size() / (1024.0 * 1024.0),
                             json_data.size());

    using namespace JsonFusion;
    using namespace JsonFusion::options;
    using namespace JsonFusion::validators;





    struct Stats {
        std::size_t totalPoints = 0;
        std::size_t totalRings = 0;
        std::size_t totalFeatures = 0;
    };






    // static_assert(static_schema::is_json_object<Feature>::value);


    struct CanadaStatsCounter {
        A<std::string, key<"type">, string_constant<"FeatureCollection"> > _;

        struct FeatureConsumer {
            struct Feature {
                A<std::string, key<"type">, string_constant<"Feature"> > _;

                std::map<std::string, std::string> properties;


                struct RingsConsumer {

                    struct RingConsumer {

                        struct Point {
                            A<float, skip_json<>> x;
                            A<float, skip_json<>> y;
                        };

                        using PointsAsArray = Annotated<Point, as_array>;

                        using value_type = PointsAsArray;

                        bool finalize(bool success)  { return true; }

                        Stats * stats = nullptr;
                        void reset()  {}
                        bool consume(const Point & r)  {
                            stats->totalPoints ++;
                            return true;
                        }
                        void set_json_fusion_context(Stats * ctx) {
                            stats = ctx;
                        }
                    };

                    using value_type = RingConsumer;

                    bool finalize(bool success)  { return true; }

                    Stats * stats = nullptr;
                    void reset()  {}

                    bool consume(const RingConsumer & ringConsumer)  {
                        stats->totalRings ++;
                        return true;
                    }

                    void set_json_fusion_context(Stats * ctx) {
                        stats = ctx;
                    }
                };

                struct PolygonGeometry {
                    A<std::string, key<"type">, string_constant<"Polygon"> > _;
                    A<RingsConsumer, key<"coordinates">> rings;
                } geometry;
            };

            using value_type = Feature;

            bool finalize(bool success)  { return true; }

            Stats * stats = nullptr;
            void reset() { }
            bool consume(const Feature & f)  {
                stats->totalFeatures ++;
                return true;
            }
            void set_json_fusion_context(Stats * ctx) {
                stats = ctx;
            }
        } features;

    };
    Stats stats;
    CanadaStatsCounter canada;
    canada.features.set_json_fusion_context(&stats);

    auto r = Parse(canada, json_data, &stats);
    assert(r);

    std::cout << std::format("Features: {}, rings: {}, points: {}", stats.totalFeatures, stats.totalRings, stats.totalPoints) << std::endl;


    // static_assert(ConsumingStreamerLike<FeatureConsumer>, "Incompatible consumer");
}

int main(int argc, char ** argv) {
    streaming_demo();
    sax_demo();
    nested_producers();
    geojson_reader(argc, argv);
}
