using System;
using System.Diagnostics;
using System.IO;
using System.Text.Json;

namespace TwitterBenchmark;

class Program
{
    static void Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine("Usage: TwitterBenchmark <path-to-twitter.json>");
            return;
        }

        string jsonPath = args[0];
        
        if (!File.Exists(jsonPath))
        {
            Console.WriteLine($"Error: File not found: {jsonPath}");
            return;
        }

        Console.WriteLine($"Reading file: {jsonPath}");
        string jsonData = File.ReadAllText(jsonPath);
        double fileSizeMB = jsonData.Length / (1024.0 * 1024.0);
        Console.WriteLine($"File size: {fileSizeMB:F2} MB ({jsonData.Length} bytes)\n");

        const int iterations = 10000;
        
        Console.WriteLine("=== C# System.Text.Json Benchmark ===\n");
        
        // Configure JsonSerializer options
        var options = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = false,
            NumberHandling = System.Text.Json.Serialization.JsonNumberHandling.AllowReadingFromString
        };

        // Warmup phase - critical for JIT compilation
        Console.WriteLine("Warming up JIT (1000 iterations)...");
        for (int i = 0; i < 1000; i++)
        {
            var warmup = JsonSerializer.Deserialize<TwitterData>(jsonData, options);
            if (warmup == null)
            {
                Console.WriteLine("Warmup deserialization failed!");
                return;
            }
        }
        Console.WriteLine("Warmup complete.\n");

        // Actual benchmark
        TwitterData? model = null;
        var stopwatch = Stopwatch.StartNew();
        
        for (int i = 0; i < iterations; i++)
        {
            model = JsonSerializer.Deserialize<TwitterData>(jsonData, options);
            if (model == null)
            {
                Console.WriteLine($"Deserialization failed at iteration {i}");
                return;
            }
        }
        
        stopwatch.Stop();
        
        double avgMicroseconds = (stopwatch.ElapsedMilliseconds * 1000.0) / iterations;
        
        Console.WriteLine($"System.Text.Json parsing + populating                            {avgMicroseconds:F2} µs/iter  ({iterations} iterations)");
        
        // Verify we got valid data
        if (model?.Statuses != null)
        {
            Console.WriteLine($"\nVerification: Parsed {model.Statuses.Count} statuses");
            if (model.Statuses.Count > 0)
            {
                Console.WriteLine($"First status text: {model.Statuses[0].Text.Substring(0, Math.Min(50, model.Statuses[0].Text.Length))}...");
            }
        }
        
        Console.WriteLine("\nBenchmark complete.");
    }
}

