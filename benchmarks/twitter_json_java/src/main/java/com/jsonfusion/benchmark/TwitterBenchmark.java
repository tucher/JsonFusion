package com.jsonfusion.benchmark;

import com.dslplatform.json.DslJson;
import com.dslplatform.json.runtime.Settings;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

/**
 * Cross-language benchmark for twitter.json parsing using DSL-JSON.
 * Comparable to C++ JsonFusion, C# System.Text.Json benchmarks.
 */
public class TwitterBenchmark {
    
    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java -jar twitter-benchmark.jar <path-to-twitter.json>");
            System.err.println("Example: java -jar twitter-benchmark.jar ../../twitter.json");
            System.exit(1);
        }
        
        String jsonPath = args[0];
        System.out.println("=== Twitter.json Parsing Benchmark - Java DSL-JSON ===\n");
        System.out.println("Reading file: " + jsonPath);
        
        // Read JSON file
        byte[] jsonBytes;
        try {
            jsonBytes = Files.readAllBytes(Paths.get(jsonPath));
            System.out.println("File size: " + jsonBytes.length + " bytes\n");
        } catch (IOException e) {
            System.err.println("Error reading file: " + e.getMessage());
            System.exit(1);
            return;
        }
        
        // Create DSL-JSON instance with compile-time generated deserializers
        DslJson<Object> dslJson = new DslJson<>(Settings.withRuntime().includeServiceLoader());
        
        // Warmup phase - important for JIT compilation
        System.out.println("Warming up JIT (1000 iterations)...");
        try {
            for (int i = 0; i < 1000; i++) {
                TwitterData warmup = dslJson.deserialize(TwitterData.class, jsonBytes, jsonBytes.length);
                if (warmup == null) {
                    System.err.println("Warmup deserialization failed!");
                    System.exit(1);
                }
            }
        } catch (IOException e) {
            System.err.println("Warmup error: " + e.getMessage());
            System.exit(1);
        }
        System.out.println("Warmup complete.\n");
        
        // Actual benchmark
        int iterations = 10000;
        System.out.println("Running benchmark (" + iterations + " iterations)...");
        
        TwitterData model = null;
        long startTime = System.nanoTime();
        
        try {
            for (int i = 0; i < iterations; i++) {
                model = dslJson.deserialize(TwitterData.class, jsonBytes, jsonBytes.length);
                if (model == null) {
                    System.err.println("Deserialization failed at iteration " + i);
                    System.exit(1);
                }
            }
        } catch (IOException e) {
            System.err.println("Benchmark error: " + e.getMessage());
            System.exit(1);
        }
        
        long endTime = System.nanoTime();
        double elapsedTimeMs = (endTime - startTime) / 1_000_000.0;
        double avgTimeUs = (elapsedTimeMs * 1000.0) / iterations;
        
        System.out.println("Benchmark complete.\n");
        System.out.println("=== Results ===");
        System.out.printf("DSL-JSON parsing + populating                                   %.2f µs/iter%n", avgTimeUs);
        System.out.printf("%nTotal time: %.2f ms for %d iterations%n", elapsedTimeMs, iterations);
        
        // Sanity check - print some data to ensure parsing worked
        if (model != null && model.statuses != null && !model.statuses.isEmpty()) {
            System.out.printf("%nSanity check: Parsed %d statuses%n", model.statuses.size());
            Status firstStatus = model.statuses.get(0);
            if (firstStatus != null && firstStatus.user != null) {
                System.out.printf("First status by: %s%n", firstStatus.user.name != null ? firstStatus.user.name : "unknown");
            }
        }
    }
}

