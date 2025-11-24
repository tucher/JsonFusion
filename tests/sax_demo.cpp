#include <string_view>
#include <array>
#include <optional>
#include <list>
#include <cassert>
#include <iostream>

#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>


void streaming_demo () {
    
    struct Streamer {
        struct Vector{
            float x, y, z;
        };
        using value_type = JsonFusion::Annotated<Vector, JsonFusion::options::as_array>;
        int count;
        mutable int counter = 0;
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
    };
    struct TopLevel {
        Streamer points_xyz;
    };
    TopLevel a;

    a.points_xyz.count=3;

    std::string out;
    JsonFusion::Serialize(a, out);

    std::cout << out << std::endl;

    /* Output:

    {"f":-2,"points_xyz":[[43,44,45],[44,45,46],[45,46,47]],"v":[1,23]}

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

        void reset()  {
            std::cout << "Receiving points" << std::endl;
        }

        bool consume(const VectorWithTimestamp & point)  {
            printPoint(point);
            return true;
        }

        bool finalize(bool success)  {
            std::cout << "All points received" << std::endl;
            return true;
        }
    };

    struct TagsConsumer {
        struct Tag {
            std::string id;
            std::string text;
        };
        using value_type = Tag;
        
        // called at array start
        void reset()  {
            std::cout << "Receiving tags" << std::endl;
        }

        // called for each element
        bool consume(const Tag & tag)  {
            std::cout << tag.id << " " << tag.text << std::endl;
            return true;
        }

        // called once at the end (with json domain success flag)
        bool finalize(bool success)  {
            std::cout << "Tags received" << std::endl; return true;
        }
    };

    struct TopLevel {
        double f;
        JsonFusion::Annotated<PointsConsumer, key<"timestamped_points">> positions;
        TagsConsumer tags;
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

int main() {
    streaming_demo();
    sax_demo();
}