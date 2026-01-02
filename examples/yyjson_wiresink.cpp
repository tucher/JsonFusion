#include <iostream>
#include <string>
#include <yyjson.h>
#include "JsonFusion/parser.hpp"
#include "JsonFusion/serializer.hpp"
#include "JsonFusion/yyjson.hpp"

using namespace JsonFusion;

// Simple test struct with WireSink field
struct TestData {
    std::string name;
    int value;
    WireSink<1024> metadata;  // Captures arbitrary JSON
    
    // Note: For yyjson (DOM-based reader), WireSink stores just a pointer
    // to the DOM node (8 bytes), not the serialized JSON!
    // This makes capture_to_sink O(1) instead of O(n) - the main advantage!
};

int main() {
    std::cout << "=== YyJSON WireSink Integration Test ===\n\n";
    
    // Test 1: Parse with WireSink capture
    std::cout << "Test 1: Parsing with WireSink\n";
    std::string input = R"({
        "name": "test",
        "value": 42,
        "metadata": {"key": "value", "nested": [1, 2, 3]}
    })";
    
    TestData data;
    {
        // RAII: YyjsonReader parses and owns the document
        YyjsonReader reader(input.data(), input.size());
        
        auto result = ParseWithReader(data, reader);
        
        if (!result) {
            std::cout << "  ✗ Parse failed\n";
            return 1;
        }
        
        std::cout << "  ✓ Parsed successfully\n";
        std::cout << "    name: " << data.name << "\n";
        std::cout << "    value: " << data.value << "\n";
        std::cout << "    metadata size: " << data.metadata.current_size() << " bytes";
        std::cout << " (just node pointer - O(1) capture!)\n";
        
        // Note: Don't try to interpret the data as JSON string - it's a DOM node pointer!
        std::cout << "    Captured as handle: " << (void*)data.metadata.data() << "\n";
        
        // IMPORTANT: The WireSink holds a pointer to the DOM, so the reader
        // (and its document) must remain alive while we use the WireSink!
        
        // Test 2: Serialize with YyjsonWriter (while reader is still alive!)
        std::cout << "\nTest 2: Serialization with WireSink\n";
        
        std::string output;
        {
            // RAII: YyjsonWriter creates owned document and auto-serializes to string
            YyjsonWriter writer(output);
            
            auto write_result = SerializeWithWriter(data, writer);
            
            if (!write_result) {
                std::cout << "  ✗ Serialization failed\n";
                return 1;
            }
            
            // writer.finish() is called by SerializeWithWriter
            // → serializes DOM to output string automatically
            
        } // writer destroyed here, mutable document freed automatically
        
        std::cout << "  ✓ Serialized successfully\n";
        std::cout << "    Output: " << output << "\n";
        
    } // reader destroyed here, document freed automatically
    
    // Test 3: Round-trip
    std::cout << "\nTest 3: Round-trip test\n";
    
    {
        // Parse again with RAII
        YyjsonReader reader2(input.data(), input.size());
        
        TestData data2;
        auto parse_result = ParseWithReader(data2, reader2);
        
        if (!parse_result) {
            std::cout << "  ✗ Round-trip parse failed\n";
            return 1;
        }
        
        // Serialize with YyjsonWriter
        std::string output;
        {
            YyjsonWriter writer2(output);
            auto write_result = SerializeWithWriter(data2, writer2);
            
            if (!write_result) {
                std::cout << "  ✗ Round-trip serialization failed\n";
                return 1;
            }
            
            // Auto-serializes to output in finish()
            
        } // writer2 destroyed here
        
        std::cout << "  ✓ Round-trip successful\n";
        std::cout << "    Final output: " << output << "\n";
        
    } // reader2 destroyed here, document freed automatically
    
    std::cout << "\n✅ All tests passed!\n";
    return 0;
}

