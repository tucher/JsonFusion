
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <format>
#include <filesystem>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include "twitter_model_generic.hpp"
#include "rapidjson_populate.hpp"
#include "benchmark.hpp"

// For RapidJSON, we use the JsonFusion instantiation
using User = User_T<std::optional<bool>>;
using TwitterData = TwitterData_T<std::optional<bool>>;


using std::vector;

namespace RapidJSONPopulate {

using namespace rapidjson;

// Helper to safely get optional values
template<typename T, typename GetFunc>
optional<T> get_optional(const Value& obj, const char* key, GetFunc&& getter) {
    if (obj.HasMember(key) && !obj[key].IsNull()) {
        return getter(obj[key]);
    }
    return std::nullopt;
}

// Primitive getters
inline double get_double(const Value& v) { return v.GetDouble(); }
inline string get_string(const Value& v) { return string(v.GetString(), v.GetStringLength()); }
inline bool get_bool(const Value& v) { return v.GetBool(); }

// Populate Urls_item
inline void populate_urls_item(Urls_item<>& item, const Value& json) {
    if (json.HasMember("url") && json["url"].IsString()) {
        item.url = get_string(json["url"]);
    }
    if (json.HasMember("expanded_url") && json["expanded_url"].IsString()) {
        item.expanded_url = get_string(json["expanded_url"]);
    }
    if (json.HasMember("display_url") && json["display_url"].IsString()) {
        item.display_url = get_string(json["display_url"]);
    }
    if (json.HasMember("indices") && json["indices"].IsArray()) {
        for (auto& idx : json["indices"].GetArray()) {
            if (idx.IsNumber()) {
                item.indices.push_back(idx.GetDouble());
            }
        }
    }
}

// Populate Metadata
inline void populate_metadata(Metadata& metadata, const Value& json) {
    metadata.result_type = get_optional<string>(json, "result_type", get_string);
    metadata.iso_language_code = get_optional<string>(json, "iso_language_code", get_string);
}

// Populate Entities::Hashtags_item
inline void populate_hashtag(Entities<>::Hashtags_item& hashtag, const Value& json) {
    if (json.HasMember("text") && json["text"].IsString()) {
        hashtag.text = get_string(json["text"]);
    }
    if (json.HasMember("indices") && json["indices"].IsArray()) {
        for (auto& idx : json["indices"].GetArray()) {
            if (idx.IsNumber()) {
                hashtag.indices.push_back(idx.GetDouble());
            }
        }
    }
}

// Populate Entities::User_mentions_item
inline void populate_user_mention(Entities<>::User_mentions_item& mention, const Value& json) {
    if (json.HasMember("screen_name") && json["screen_name"].IsString()) {
        mention.screen_name = get_string(json["screen_name"]);
    }
    if (json.HasMember("name") && json["name"].IsString()) {
        mention.name = get_string(json["name"]);
    }
    if (json.HasMember("id") && json["id"].IsNumber()) {
        mention.id = json["id"].GetDouble();
    }
    if (json.HasMember("id_str") && json["id_str"].IsString()) {
        mention.id_str = get_string(json["id_str"]);
    }
    if (json.HasMember("indices") && json["indices"].IsArray()) {
        for (auto& idx : json["indices"].GetArray()) {
            if (idx.IsNumber()) {
                mention.indices.push_back(idx.GetDouble());
            }
        }
    }
}

// Populate Entities::Media_item::Sizes
inline void populate_media_size(optional<double>& w, optional<double>& h, optional<string>& resize, const Value& json) {
    w = get_optional<double>(json, "w", get_double);
    h = get_optional<double>(json, "h", get_double);
    resize = get_optional<string>(json, "resize", get_string);
}

inline void populate_media_sizes(Entities<>::Media_item::Sizes& sizes, const Value& json) {
    if (json.HasMember("medium") && json["medium"].IsObject()) {
        Entities<>::Media_item::Sizes::Medium medium;
        populate_media_size(medium.w, medium.h, medium.resize, json["medium"]);
        sizes.medium = std::move(medium);
    }
    if (json.HasMember("small") && json["small"].IsObject()) {
        Entities<>::Media_item::Sizes::Small small;
        populate_media_size(small.w, small.h, small.resize, json["small"]);
        sizes.small = std::move(small);
    }
    if (json.HasMember("thumb") && json["thumb"].IsObject()) {
        Entities<>::Media_item::Sizes::Thumb thumb;
        populate_media_size(thumb.w, thumb.h, thumb.resize, json["thumb"]);
        sizes.thumb = std::move(thumb);
    }
    if (json.HasMember("large") && json["large"].IsObject()) {
        Entities<>::Media_item::Sizes::Large large;
        populate_media_size(large.w, large.h, large.resize, json["large"]);
        sizes.large = std::move(large);
    }
}

// Populate Entities::Media_item
inline void populate_media_item(Entities<>::Media_item& media, const Value& json) {
    if (json.HasMember("id") && json["id"].IsNumber()) {
        media.id = json["id"].GetDouble();
    }
    if (json.HasMember("id_str") && json["id_str"].IsString()) {
        media.id_str = get_string(json["id_str"]);
    }
    if (json.HasMember("indices") && json["indices"].IsArray()) {
        for (auto& idx : json["indices"].GetArray()) {
            if (idx.IsNumber()) {
                media.indices.push_back(idx.GetDouble());
            }
        }
    }
    if (json.HasMember("media_url") && json["media_url"].IsString()) {
        media.media_url = get_string(json["media_url"]);
    }
    if (json.HasMember("media_url_https") && json["media_url_https"].IsString()) {
        media.media_url_https = get_string(json["media_url_https"]);
    }
    if (json.HasMember("url") && json["url"].IsString()) {
        media.url = get_string(json["url"]);
    }
    if (json.HasMember("display_url") && json["display_url"].IsString()) {
        media.display_url = get_string(json["display_url"]);
    }
    if (json.HasMember("expanded_url") && json["expanded_url"].IsString()) {
        media.expanded_url = get_string(json["expanded_url"]);
    }
    if (json.HasMember("type") && json["type"].IsString()) {
        media.type = get_string(json["type"]);
    }
    if (json.HasMember("sizes") && json["sizes"].IsObject()) {
        populate_media_sizes(media.sizes, json["sizes"]);
    }
    if (json.HasMember("source_status_id") && json["source_status_id"].IsNumber()) {
        media.source_status_id = json["source_status_id"].GetDouble();
    }
    if (json.HasMember("source_status_id_str") && json["source_status_id_str"].IsString()) {
        media.source_status_id_str = get_string(json["source_status_id_str"]);
    }
}

// Populate Entities
inline void populate_entities(Entities<>& entities, const Value& json) {
    if (json.HasMember("hashtags") && json["hashtags"].IsArray()) {
        vector<Entities<>::Hashtags_item> hashtags;
        for (auto& hashtag_json : json["hashtags"].GetArray()) {
            Entities<>::Hashtags_item item;
            populate_hashtag(item, hashtag_json);
            hashtags.push_back(std::move(item));
        }
        entities.hashtags = std::move(hashtags);
    }

    if (json.HasMember("symbols") && json["symbols"].IsArray()) {
        vector<string> symbols;
        for (auto& symbol_json : json["symbols"].GetArray()) {
            if (symbol_json.IsString()) {
                symbols.push_back(get_string(symbol_json));
            }
        }
        entities.symbols = std::move(symbols);
    }

    if (json.HasMember("urls") && json["urls"].IsArray()) {
        vector<Urls_item<>> urls;
        for (auto& url_json : json["urls"].GetArray()) {
            Urls_item item;
            populate_urls_item(item, url_json);
            urls.push_back(std::move(item));
        }
        entities.urls = std::move(urls);
    }

    if (json.HasMember("user_mentions") && json["user_mentions"].IsArray()) {
        vector<Entities<>::User_mentions_item> mentions;
        for (auto& mention_json : json["user_mentions"].GetArray()) {
            Entities<>::User_mentions_item item;
            populate_user_mention(item, mention_json);
            mentions.push_back(std::move(item));
        }
        entities.user_mentions = std::move(mentions);
    }

    if (json.HasMember("media") && json["media"].IsArray()) {
        vector<Entities<>::Media_item> media;
        for (auto& media_json : json["media"].GetArray()) {
            Entities<>::Media_item item;
            populate_media_item(item, media_json);
            media.push_back(std::move(item));
        }
        entities.media = std::move(media);
    }
}

// Populate UserEntities
inline void populate_user_entities(UserEntities<>& entities, const Value& json) {
    if (json.HasMember("description") && json["description"].IsObject()) {
        UserEntities<>::Description desc;
        if (json["description"].HasMember("urls") && json["description"]["urls"].IsArray()) {
            vector<Urls_item<>> urls;
            for (auto& url_json : json["description"]["urls"].GetArray()) {
                Urls_item<> item;
                populate_urls_item(item, url_json);
                urls.push_back(std::move(item));
            }
            desc.urls = std::move(urls);
        }
        entities.description = std::move(desc);
    }

    if (json.HasMember("url") && json["url"].IsObject()) {
        UserEntities<>::Url url;
        if (json["url"].HasMember("urls") && json["url"]["urls"].IsArray()) {
            vector<Urls_item<>> urls;
            for (auto& url_json : json["url"]["urls"].GetArray()) {
                Urls_item item;
                populate_urls_item(item, url_json);
                urls.push_back(std::move(item));
            }
            url.urls = std::move(urls);
        }
        entities.url = std::move(url);
    }
}


inline void populate_user(User& user, const Value& json) {

    user.id = get_optional<double>(json, "id", get_double);
    user.id_str = get_optional<string>(json, "id_str", get_string);
    user.name = get_optional<string>(json, "name", get_string);
    user.screen_name = get_optional<string>(json, "screen_name", get_string);
    user.location = get_optional<string>(json, "location", get_string);
    user.description = get_optional<string>(json, "description", get_string);
    user.url = get_optional<string>(json, "url", get_string);

    if (json.HasMember("entities") && json["entities"].IsObject()) {
        user.entities = std::make_unique<UserEntities<>>();
        populate_user_entities(*user.entities, json["entities"]);
    }

    user.protected_ = get_optional<bool>(json, "protected", get_bool);
    user.followers_count = get_optional<double>(json, "followers_count", get_double);
    user.friends_count = get_optional<double>(json, "friends_count", get_double);
    user.listed_count = get_optional<double>(json, "listed_count", get_double);
    user.created_at = get_optional<string>(json, "created_at", get_string);
    user.favourites_count = get_optional<double>(json, "favourites_count", get_double);
    user.utc_offset = get_optional<double>(json, "utc_offset", get_double);
    user.time_zone = get_optional<string>(json, "time_zone", get_string);
    user.geo_enabled = get_optional<bool>(json, "geo_enabled", get_bool);
    user.verified = get_optional<bool>(json, "verified", get_bool);
    user.statuses_count = get_optional<double>(json, "statuses_count", get_double);
    user.lang = get_optional<string>(json, "lang", get_string);
    user.contributors_enabled = get_optional<bool>(json, "contributors_enabled", get_bool);
    user.is_translator = get_optional<bool>(json, "is_translator", get_bool);
    user.is_translation_enabled = get_optional<bool>(json, "is_translation_enabled", get_bool);
    user.profile_background_color = get_optional<string>(json, "profile_background_color", get_string);
    user.profile_background_image_url = get_optional<string>(json, "profile_background_image_url", get_string);
    user.profile_background_image_url_https = get_optional<string>(json, "profile_background_image_url_https", get_string);
    user.profile_background_tile = get_optional<bool>(json, "profile_background_tile", get_bool);
    user.profile_image_url = get_optional<string>(json, "profile_image_url", get_string);
    user.profile_image_url_https = get_optional<string>(json, "profile_image_url_https", get_string);
    user.profile_banner_url = get_optional<string>(json, "profile_banner_url", get_string);
    user.profile_link_color = get_optional<string>(json, "profile_link_color", get_string);
    user.profile_sidebar_border_color = get_optional<string>(json, "profile_sidebar_border_color", get_string);
    user.profile_sidebar_fill_color = get_optional<string>(json, "profile_sidebar_fill_color", get_string);
    user.profile_text_color = get_optional<string>(json, "profile_text_color", get_string);
    user.profile_use_background_image = get_optional<bool>(json, "profile_use_background_image", get_bool);
    user.default_profile = get_optional<bool>(json, "default_profile", get_bool);
    user.default_profile_image = get_optional<bool>(json, "default_profile_image", get_bool);
    user.following = get_optional<bool>(json, "following", get_bool);
    user.follow_request_sent = get_optional<bool>(json, "follow_request_sent", get_bool);
    user.notifications = get_optional<bool>(json, "notifications", get_bool);
}



// Populate Retweeted_status
inline void populate_retweeted_status(TwitterData::Statuses_item::Retweeted_status& status, const Value& json) {
    if (json.HasMember("metadata") && json["metadata"].IsObject()) {
        status.metadata = std::make_unique<Metadata>();
        populate_metadata(*status.metadata, json["metadata"]);
    }

    status.created_at = get_optional<string>(json, "created_at", get_string);
    status.id = get_optional<double>(json, "id", get_double);
    status.id_str = get_optional<string>(json, "id_str", get_string);
    status.text = get_optional<string>(json, "text", get_string);
    status.source = get_optional<string>(json, "source", get_string);
    status.truncated = get_optional<bool>(json, "truncated", get_bool);
    status.in_reply_to_status_id = get_optional<double>(json, "in_reply_to_status_id", get_double);
    status.in_reply_to_status_id_str = get_optional<string>(json, "in_reply_to_status_id_str", get_string);
    status.in_reply_to_user_id = get_optional<double>(json, "in_reply_to_user_id", get_double);
    status.in_reply_to_user_id_str = get_optional<string>(json, "in_reply_to_user_id_str", get_string);
    status.in_reply_to_screen_name = get_optional<string>(json, "in_reply_to_screen_name", get_string);

    if (json.HasMember("user") && json["user"].IsObject()) {
        status.user = std::make_unique<User>();
        populate_user(*status.user, json["user"]);
    }

    status.geo = get_optional<bool>(json, "geo", get_bool);
    status.coordinates = get_optional<bool>(json, "coordinates", get_bool);
    status.place = get_optional<bool>(json, "place", get_bool);
    status.contributors = get_optional<bool>(json, "contributors", get_bool);
    status.retweet_count = get_optional<double>(json, "retweet_count", get_double);
    status.favorite_count = get_optional<double>(json, "favorite_count", get_double);

    if (json.HasMember("entities") && json["entities"].IsObject()) {
        status.entities = std::make_unique<Entities<>>();
        populate_entities(*status.entities, json["entities"]);
    }

    status.favorited = get_optional<bool>(json, "favorited", get_bool);
    status.retweeted = get_optional<bool>(json, "retweeted", get_bool);
    status.possibly_sensitive = get_optional<bool>(json, "possibly_sensitive", get_bool);
    status.lang = get_optional<string>(json, "lang", get_string);
}

// Populate Statuses_item
inline void populate_status(TwitterData::Statuses_item& status, const Value& json) {
    if (json.HasMember("metadata") && json["metadata"].IsObject()) {
        populate_metadata(status.metadata, json["metadata"]);
    }

    if (json.HasMember("created_at") && json["created_at"].IsString()) {
        status.created_at = get_string(json["created_at"]);
    }
    if (json.HasMember("id") && json["id"].IsNumber()) {
        status.id = json["id"].GetDouble();
    }
    if (json.HasMember("id_str") && json["id_str"].IsString()) {
        status.id_str = get_string(json["id_str"]);
    }
    if (json.HasMember("text") && json["text"].IsString()) {
        status.text = get_string(json["text"]);
    }
    if (json.HasMember("source") && json["source"].IsString()) {
        status.source = get_string(json["source"]);
    }
    if (json.HasMember("truncated") && json["truncated"].IsBool()) {
        status.truncated = json["truncated"].GetBool();
    }

    status.in_reply_to_status_id = get_optional<double>(json, "in_reply_to_status_id", get_double);
    status.in_reply_to_status_id_str = get_optional<string>(json, "in_reply_to_status_id_str", get_string);
    status.in_reply_to_user_id = get_optional<double>(json, "in_reply_to_user_id", get_double);
    status.in_reply_to_user_id_str = get_optional<string>(json, "in_reply_to_user_id_str", get_string);
    status.in_reply_to_screen_name = get_optional<string>(json, "in_reply_to_screen_name", get_string);

    if (json.HasMember("user") && json["user"].IsObject()) {
        populate_user(status.user, json["user"]);
    }

    status.geo = get_optional<bool>(json, "geo", get_bool);
    status.coordinates = get_optional<bool>(json, "coordinates", get_bool);
    status.place = get_optional<bool>(json, "place", get_bool);
    status.contributors = get_optional<bool>(json, "contributors", get_bool);

    if (json.HasMember("retweet_count") && json["retweet_count"].IsNumber()) {
        status.retweet_count = json["retweet_count"].GetDouble();
    }
    if (json.HasMember("favorite_count") && json["favorite_count"].IsNumber()) {
        status.favorite_count = json["favorite_count"].GetDouble();
    }

    if (json.HasMember("entities") && json["entities"].IsObject()) {
        populate_entities(status.entities, json["entities"]);
    }

    if (json.HasMember("favorited") && json["favorited"].IsBool()) {
        status.favorited = json["favorited"].GetBool();
    }
    if (json.HasMember("retweeted") && json["retweeted"].IsBool()) {
        status.retweeted = json["retweeted"].GetBool();
    }
    if (json.HasMember("lang") && json["lang"].IsString()) {
        status.lang = get_string(json["lang"]);
    }

    if (json.HasMember("retweeted_status") && json["retweeted_status"].IsObject()) {
        populate_retweeted_status(status.retweeted_status, json["retweeted_status"]);
    }

    if (json.HasMember("possibly_sensitive") && json["possibly_sensitive"].IsBool()) {
        status.possibly_sensitive = json["possibly_sensitive"].GetBool();
    }
}

// Populate Search_metadata
inline void populate_search_metadata(TwitterData::Search_metadata& metadata, const Value& json) {
    metadata.completed_in = get_optional<double>(json, "completed_in", get_double);
    metadata.max_id = get_optional<double>(json, "max_id", get_double);
    metadata.max_id_str = get_optional<string>(json, "max_id_str", get_string);
    metadata.next_results = get_optional<string>(json, "next_results", get_string);
    metadata.query = get_optional<string>(json, "query", get_string);
    metadata.refresh_url = get_optional<string>(json, "refresh_url", get_string);
    metadata.count = get_optional<double>(json, "count", get_double);
    metadata.since_id = get_optional<double>(json, "since_id", get_double);
    metadata.since_id_str = get_optional<string>(json, "since_id_str", get_string);
}

// Populate TwitterData
inline void populate_twitter_data(TwitterData& data, const Value& json) {
    if (json.HasMember("statuses") && json["statuses"].IsArray()) {
        vector<TwitterData::Statuses_item> statuses;
        for (auto& status_json : json["statuses"].GetArray()) {
            TwitterData::Statuses_item status;
            populate_status(status, status_json);
            statuses.push_back(std::move(status));
        }
        data.statuses = std::move(statuses);
    }

    if (json.HasMember("search_metadata") && json["search_metadata"].IsObject()) {
        TwitterData::Search_metadata metadata;
        populate_search_metadata(metadata, json["search_metadata"]);
        data.search_metadata = std::move(metadata);
    }
}

} // namespace RapidJSONPopulate



void rj_parse_only(int iterations, std::string json_data)
{
    rapidjson::Document doc;

    benchmark("RapidJSON DOM Parse ONLY", iterations, [&]() {
        std::string copy = json_data;
        doc.ParseInsitu(copy.data());
        if (doc.HasParseError()) {
            throw std::runtime_error(std::format("RapidJSON parse error: {}",
                                                 rapidjson::GetParseError_En(doc.GetParseError())));
        }
    });
}

void rj_parse_populate(int iterations, std::string json_data)
{
    TwitterData model;
    rapidjson::Document doc;

    benchmark("RapidJSON parsing + populating (manual)", iterations, [&]() {
        std::string copy = json_data;
        doc.ParseInsitu(copy.data());
        if (doc.HasParseError()) {
            throw std::runtime_error(std::format("RapidJSON parse error: {}",
                                                 rapidjson::GetParseError_En(doc.GetParseError())));
        }

        RapidJSONPopulate::populate_twitter_data(model, doc);
    });
}
