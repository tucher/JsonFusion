// bench_matrix.hpp
#pragma once
#include <string>
#include <string_view>
#include <iostream>
#include <chrono>
#include <type_traits>

template<typename... T>
struct Libraries {};

template<typename... T>
struct Configs {};

inline bool time_op(
    const std::string& label,
    int iterations,
    auto&& op,
    double& avg_us_out)
{
    using clock = std::chrono::steady_clock;

    auto start = clock::now();
    for (int i = 0; i < iterations; ++i) {
        if (!op()) {
            avg_us_out = 0.0;
            return false;
        }
    }
    auto end = clock::now();
    auto total_us =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    avg_us_out = double(total_us) / iterations;
    return true;
}

namespace detail {

template<typename T, typename Out>
concept HasParse = requires(T t, Out& out, std::string& data, bool insitu, std::string& remark) {
    { t.parse_validate_and_populate(out, data, insitu, remark) } -> std::same_as<bool>;
};

template<typename T>
concept HasDynamic = requires {
    typename T::DynamicModel;
};

template<typename T>
concept HasStatic = requires {
    typename T::StaticModel;
};




template<typename  Config, typename ... Libs>
void run_for_cfg(Libs...);

template<typename... Libs, typename... Cfgs>
int run_impl(Libraries<Libs...>, Configs<Cfgs...>) {
    (run_for_cfg<Cfgs, Libs...>(Libs{}...), ...);
    return 0;
}

template<typename Cfg, typename ... Libs>
void run_for_cfg(Libs...libs) {
    std::cout << std::format("{:=^40} iterations: {}",  "Model " + std::string(Cfg::name), Cfg::iter_count) << std::endl;

    auto run_for_pair = [&]<typename Model, typename Tester>(std::string_view json_src) {

        Tester tester;
        Model    out{};
        std::string remark;



        // Prepare base JSON once
        std::string json_source(json_src);

        auto run = [&](bool insitu) {
            std::string_view lbl{insitu?"insitu":"non_insitu"};
            if constexpr (HasParse<Tester, Model>) {
                double avg_us{};

                bool ok = time_op(std::string(lbl), Cfg::iter_count, [&]() {
                    std::string buf = json_source; // fresh copy for insitu
                    return tester.parse_validate_and_populate(out, buf, insitu, remark);
                }, avg_us);
                std::string rem = remark.empty() ? "": "NOTE: "+remark;
                if (ok) {
                    std::cout << std::format("{:>30}  {:>10}      {:>6.2f} us/iter   {}", Tester::library_name, lbl, avg_us, rem) << std::endl;
                } else {
                    std::cout << std::format("{:>30}  {:>10}      {:>8}    {}", Tester::library_name, lbl, "FAILED", rem) << std::endl;
                }
            } else {
                std::cout << std::format("    {:<30}  {:>10}{:>6}", Tester::library_name, lbl, "N/A") << std::endl;
            }
        };
        run(true);
        run(false);


    };
    bool print_containers = HasStatic<Cfg> && HasDynamic<Cfg>;
    if constexpr(HasStatic<Cfg>) {
        if(print_containers)std::cout << std::format("{}", "  Static containers") << std::endl;
        (run_for_pair.template operator()<typename Cfg::StaticModel, Libs>(Cfg::json), ...);
    }
    if constexpr(HasDynamic<Cfg>) {
        if(print_containers)std::cout << std::format("{}", "  Dynamic containers") << std::endl;
        (run_for_pair.template operator()<typename Cfg::DynamicModel, Libs>(Cfg::json), ...);
    }

    std::cout << "\n";
}






} // namespace detail



template<typename LibList, typename ConfigList>
int run() {
    return detail::run_impl(LibList{}, ConfigList{});
}
