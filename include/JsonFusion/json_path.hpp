#pragma once

#include "static_schema.hpp"
#include "struct_introspection.hpp"
#include "options.hpp"

namespace JsonFusion {
namespace json_path {

static constexpr std::size_t DefaultInlineKeyCapacity = 256;

constexpr bool allowed_std_string_allocation() {
#ifdef JSONFUSION_ALLOW_JSON_PATH_STRING_ALLOCATION_FOR_MAP_ACCESS
    return true;
#else
    return false;
#endif
}

using namespace static_schema;

template<std::size_t InlineKeyCapacity>
struct PathElement {
    std::size_t     array_index = std::numeric_limits<std::size_t>::max();   // all array-like
    std::string_view field_name;  // consteval name for structs, buffer content for maps
    bool is_static = true;
    char        buf[InlineKeyCapacity > 0 ? InlineKeyCapacity : 1] = {};


    PathElement() = default;

    // For `{index}`
    constexpr PathElement(std::size_t index)
        : array_index(index)
        , field_name{}
        , is_static(true)
    {}

    // For `{max, key, is_static}`
    constexpr PathElement(std::size_t index, std::string_view key, bool is_static_ = true)
        : array_index(index)
        , field_name(key)
        , is_static(is_static_)
    {
        if(!is_static_) {
            constexpr std::size_t Cap = InlineKeyCapacity > 0 ? InlineKeyCapacity : 1;
            const std::size_t len = std::min<std::size_t>(key.size(), Cap);
            for(std::size_t l = 0; l < len; l ++) buf[l] = key[l];
            field_name = std::string_view(buf, len);
        }
    }


    // Copy ctor
    constexpr PathElement(const PathElement& other)
        : array_index(other.array_index),
        is_static(other.is_static)
    {
        if (is_static) {
            // Static case: just copy the view (points to static storage)
            field_name = other.field_name;
        } else {
            // Dynamic (map key) case: copy from other's buf into ours
            constexpr std::size_t Cap = InlineKeyCapacity > 0 ? InlineKeyCapacity : 1;
            const std::size_t len = std::min<std::size_t>(other.field_name.size(), Cap);
            for(std::size_t l = 0; l < len; l ++) buf[l] = other.buf[l];
            field_name = std::string_view(buf, len);
        }
    }

    // Copy assignment
    constexpr PathElement& operator=(const PathElement& other) {
        if (this == &other) return *this;

        array_index = other.array_index;
        is_static   = other.is_static;

        if (is_static) {
            field_name = other.field_name;
        } else {
            constexpr std::size_t Cap = InlineKeyCapacity > 0 ? InlineKeyCapacity : 1;
            const std::size_t len = std::min<std::size_t>(other.field_name.size(), Cap);
            for(std::size_t l = 0; l < len; l ++) buf[l] = other.buf[l];
            field_name = std::string_view(buf, len);
        }
        return *this;
    }

    // Move = same as copy (buf is inline anyway)
    constexpr PathElement(PathElement&& other) noexcept
        : PathElement(static_cast<const PathElement&>(other)) {}

    constexpr PathElement& operator=(PathElement&& other) noexcept {
        return *this = static_cast<const PathElement&>(other);
    }
};

template<std::size_t SchemaDepth, bool schemaHasMaps> // schema depth INCLUDING root
struct JsonPath {
    static constexpr std::size_t InlineKeyCapacity = schemaHasMaps ? DefaultInlineKeyCapacity: 0;

    using PathElementT = PathElement<InlineKeyCapacity>;
    using StorageT = std::conditional_t<SchemaDepth != schema_analyzis::SCHEMA_UNBOUNDED,
                                     std::array<PathElementT, SchemaDepth-1>,
                                    std::vector<PathElementT>
                                     >;

    static constexpr bool unbounded =  SchemaDepth == schema_analyzis::SCHEMA_UNBOUNDED;
    std::size_t currentLength = 0;

    StorageT storage;
    constexpr JsonPath () {
        currentLength = 0;
    }


    constexpr void push_child(PathElementT && el) {
        if constexpr(!unbounded) {
            storage[currentLength] = el;
        } else {
            storage.emplace_back(std::forward<PathElementT>(el));
        }
        currentLength ++;
    }
    constexpr void pop() {
        if constexpr (unbounded) {
            storage.pop_back();
        }
        currentLength --;
    }

    template <class ... PathElems>
    constexpr JsonPath(PathElems ... args) {
        auto toPathElement = []<class ArgT>(ArgT arg) {
            if constexpr (std::is_convertible_v<ArgT, std::string_view>) {
                return PathElementT {
                    std::numeric_limits<std::size_t>::max(),
                        std::string_view(arg)
                };
            } else if constexpr (std::is_convertible_v<ArgT, std::size_t>){
                return PathElementT {
                    static_cast<std::size_t>(arg),
                    std::string_view()
                };
            } else {
                static_assert(!sizeof(arg), "Use integer integers or str-compatible segments in JsonPath contruction");
            }
        };
        storage = StorageT {toPathElement(args)...};
        currentLength = sizeof...(args);
    }


    template<class T, class Visitor>
    constexpr bool visit(T & obj, Visitor && v, std::size_t offs = 0) const {
        if(offs == currentLength) {
            using Opts    = options::detail::annotation_meta_getter<T>::options;
            v(static_cast<AnnotatedValue<T> & >(obj), Opts{});
            return true;
        } else {
            if(storage[offs].array_index == std::numeric_limits<std::size_t>::max()) {// struct or map
                if constexpr (static_schema::JsonParsableMap<T>) {
                    if constexpr (requires {obj.contains( storage[offs].field_name);}) {
                        if(obj.contains(storage[offs].field_name))
                            return visit(obj[storage[offs].field_name], std::forward<Visitor>(v), offs + 1);
                        else
                            return false;
                    } else {
                        if constexpr(allowed_std_string_allocation()) {
#if __has_include(<string>)
                            if(obj.contains(std::string(storage[offs].field_name)))
                                return visit(obj[std::string(storage[offs].field_name)], std::forward<Visitor>(v), offs + 1);
                            else
#endif
                                return false;
                        } else {
                            static_assert(!sizeof(T), "Either enable JSONFUSION_ALLOW_JSON_PATH_STRING_ALLOCATION_FOR_MAP_ACCESS macro or use map-like, which supports std::string_view");
                            return false;
                        }
                    }
                } else if constexpr (static_schema::JsonObject<T>) {

                    auto try_one = [&](auto ic) {
                        constexpr std::size_t StructIndex = decltype(ic)::value;

                        using Field   = introspection::structureElementTypeByIndex<StructIndex, T>;
                        using Opts    = options::detail::annotation_meta_getter<Field>::options;

                        std::string_view sv = "";
                        if constexpr (Opts::template has_option<options::detail::key_tag>) {
                            using KeyOpt = typename Opts::template get_option<options::detail::key_tag>;
                            sv = KeyOpt::desc.toStringView();
                        } else {
                            sv = introspection::structureElementNameByIndex<StructIndex, T>;
                        }

                        if(storage[offs].field_name == sv) {
                            return visit(introspection::getStructElementByIndex<StructIndex, T>(obj), std::forward<Visitor>(v), offs + 1);
                        } else {
                            return false;
                        }
                    };
                    bool field_visit_result = [&]<std::size_t... I>(std::index_sequence<I...>) {
                        bool ok = false;
                        (
                            (ok = ok || try_one(std::integral_constant<std::size_t, I>{}), 0),
                            ...
                            );
                        return ok;
                    } (std::make_index_sequence<introspection::structureElementsCount<T>>{});
                    return field_visit_result;
                } else {
                    return false;
                }
            } else {  // array
                if  constexpr (!static_schema::JsonParsableArray<T>) {

                    return false;
                } else {
                    if(storage[offs].array_index >= obj.size()) {
                        return false;
                    } else {
                        return visit(obj[storage[offs].array_index], std::forward<Visitor>(v), offs + 1);
                    }
                }
            }
        }
    }

    template<class T, class Visitor, class Opts>
    constexpr bool visit_options(Visitor && v, Opts o, std::size_t offs = 0) const {
        if(offs == currentLength) {
            v(o);
            return true;
        } else {
            if(storage[offs].array_index == std::numeric_limits<std::size_t>::max()) {// struct or map
                if constexpr (static_schema::JsonParsableMap<T>) {
                    using MappedItemOpts    = options::detail::annotation_meta_getter<typename map_write_cursor<AnnotatedValue<T>>::mapped_type>::options;

                    return visit_options<
                        AnnotatedValue<typename map_write_cursor<AnnotatedValue<T>>::mapped_type>
                        >(std::forward<Visitor>(v), MappedItemOpts{}, offs + 1);
                } else if constexpr (static_schema::JsonObject<T>) {

                    auto try_one = [&](auto ic) {
                        constexpr std::size_t StructIndex = decltype(ic)::value;

                        using Field   = introspection::structureElementTypeByIndex<StructIndex, T>;
                        using FieldOpts    = options::detail::annotation_meta_getter<Field>::options;

                        std::string_view sv = "";
                        if constexpr (Opts::template has_option<options::detail::key_tag>) {
                            using KeyOpt = typename Opts::template get_option<options::detail::key_tag>;
                            sv = KeyOpt::desc.toStringView();
                        } else {
                            sv = introspection::structureElementNameByIndex<StructIndex, T>;
                        }

                        if(storage[offs].field_name == sv) {
                            return visit_options<AnnotatedValue<Field>>(std::forward<Visitor>(v), FieldOpts{}, offs + 1);
                        } else {
                            return false;
                        }
                    };
                    bool field_visit_result = [&]<std::size_t... I>(std::index_sequence<I...>) {
                        bool ok = false;
                        (
                            (ok = ok || try_one(std::integral_constant<std::size_t, I>{}), 0),
                            ...
                            );
                        return ok;
                    } (std::make_index_sequence<introspection::structureElementsCount<T>>{});
                    return field_visit_result;
                } else {
                    return false;
                }
            } else {  // array
                if  constexpr (!static_schema::JsonParsableArray<T>) {

                    return false;
                } else {
                    using ElemOpts    = options::detail::annotation_meta_getter<typename array_write_cursor<AnnotatedValue<T>>::element_type>::options;
                    return visit_options<
                        AnnotatedValue<typename array_write_cursor<AnnotatedValue<T>>::element_type>
                        >(std::forward<Visitor>(v), ElemOpts{}, offs + 1);
                }
            }
        }
    }
};



template<class C, class Visitor, class JsonPath>
constexpr bool visit_by_path(C & obj, Visitor && v, const JsonPath & path) {
    return path.visit(obj, std::forward<Visitor>(v));
}




}
}
