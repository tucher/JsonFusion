#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

using std::optional, std::vector, std::string, std::unique_ptr;

// Generic Twitter model that works with both JsonFusion and reflect-cpp
// Template parameter ProtectedFieldType allows using different wrapper types:
// - For JsonFusion: A<optional<bool>, key<"protected">>
// - For reflect-cpp: rfl::Rename<"protected", std::optional<bool>>

struct Urls_item {
    string url;
    string expanded_url;
    string display_url;
    vector<double> indices;
};

struct Entities {
    struct Hashtags_item {
        string text;
        vector<double> indices;
    };

    struct User_mentions_item {
        string screen_name;
        string name;
        double id;
        string id_str;
        vector<double> indices;
    };
    
    struct Media_item {
        struct Sizes {
            struct Medium {
                optional<double> w;
                optional<double> h;
                optional<string> resize;
            };
            struct Small {
                optional<double> w;
                optional<double> h;
                optional<string> resize;
            };
            struct Thumb {
                optional<double> w;
                optional<double> h;
                optional<string> resize;
            };
            struct Large {
                optional<double> w;
                optional<double> h;
                optional<string> resize;
            };

            optional<Medium> medium;
            optional<Small> small;
            optional<Thumb> thumb;
            optional<Large> large;
        };

        double id;
        string id_str;
        vector<double> indices;
        string media_url;
        string media_url_https;
        string url;
        string display_url;
        string expanded_url;
        string type;
        Sizes sizes;
        double source_status_id;
        string source_status_id_str;
    };

    optional<vector<Hashtags_item>> hashtags;
    optional<vector<string>> symbols;
    optional<vector<Urls_item>> urls;
    optional<vector<User_mentions_item>> user_mentions;
    optional<vector<Media_item>> media;
};

struct UserEntities {
    struct Description {
        optional<vector<Urls_item>> urls;
    };
    struct Url {
        optional<vector<Urls_item>> urls;
    };

    optional<Description> description;
    optional<Url> url;
};

template<typename ProtectedFieldType>
struct User_T {
    optional<double> id;
    optional<string> id_str;
    optional<string> name;
    optional<string> screen_name;
    optional<string> location;
    optional<string> description;
    optional<string> url;
    unique_ptr<UserEntities> entities;
    
    // Parameterized field wrapper for "protected" -> "protected_"
    ProtectedFieldType protected_;
    
    optional<double> followers_count;
    optional<double> friends_count;
    optional<double> listed_count;
    optional<string> created_at;
    optional<double> favourites_count;
    optional<double> utc_offset;
    optional<string> time_zone;
    optional<bool> geo_enabled;
    optional<bool> verified;
    optional<double> statuses_count;
    optional<string> lang;
    optional<bool> contributors_enabled;
    optional<bool> is_translator;
    optional<bool> is_translation_enabled;
    optional<string> profile_background_color;
    optional<string> profile_background_image_url;
    optional<string> profile_background_image_url_https;
    optional<bool> profile_background_tile;
    optional<string> profile_image_url;
    optional<string> profile_image_url_https;
    optional<string> profile_banner_url;
    optional<string> profile_link_color;
    optional<string> profile_sidebar_border_color;
    optional<string> profile_sidebar_fill_color;
    optional<string> profile_text_color;
    optional<bool> profile_use_background_image;
    optional<bool> default_profile;
    optional<bool> default_profile_image;
    optional<bool> following;
    optional<bool> follow_request_sent;
    optional<bool> notifications;
};

struct Metadata {
    optional<string> result_type;
    optional<string> iso_language_code;
};

template<typename ProtectedFieldType>
struct TwitterData_T {
    struct Statuses_item { 
        struct Retweeted_status {
            unique_ptr<Metadata> metadata;
            optional<string> created_at;
            optional<double> id;
            optional<string> id_str;
            optional<string> text;
            optional<string> source;
            optional<bool> truncated;
            optional<double> in_reply_to_status_id;
            optional<string> in_reply_to_status_id_str;
            optional<double> in_reply_to_user_id;
            optional<string> in_reply_to_user_id_str;
            optional<string> in_reply_to_screen_name;
            unique_ptr<User_T<ProtectedFieldType>> user;
            optional<bool> geo;
            optional<bool> coordinates;
            optional<bool> place;
            optional<bool> contributors;
            optional<double> retweet_count;
            optional<double> favorite_count;
            unique_ptr<Entities> entities;
            optional<bool> favorited;
            optional<bool> retweeted;
            optional<bool> possibly_sensitive;
            optional<string> lang;
        };

        Metadata metadata;
        string created_at;
        double id;
        string id_str;
        string text;
        string source;
        bool truncated;
        optional<double> in_reply_to_status_id;
        optional<string> in_reply_to_status_id_str;
        optional<double> in_reply_to_user_id;
        optional<string> in_reply_to_user_id_str;
        optional<string> in_reply_to_screen_name;
        User_T<ProtectedFieldType> user;
        optional<bool> geo;
        optional<bool> coordinates;
        optional<bool> place;
        optional<bool> contributors;

        double retweet_count;
        double favorite_count;
        Entities entities;
        bool favorited;
        bool retweeted;
        string lang;
        Retweeted_status retweeted_status;
        bool possibly_sensitive;
    };

    struct Search_metadata {
        optional<double> completed_in;
        optional<double> max_id;
        optional<string> max_id_str;
        optional<string> next_results;
        optional<string> query;
        optional<string> refresh_url;
        optional<double> count;
        optional<double> since_id;
        optional<string> since_id_str;
    };

    optional<vector<Statuses_item>> statuses;
    optional<Search_metadata> search_metadata;
};

