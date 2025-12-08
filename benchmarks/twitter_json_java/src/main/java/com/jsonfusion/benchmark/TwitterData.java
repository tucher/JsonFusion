package com.jsonfusion.benchmark;

import com.dslplatform.json.CompiledJson;
import com.dslplatform.json.JsonAttribute;
import java.util.List;

// Java equivalent to twitter_model_generic.hpp
// Mapping:
//   std::optional<T>  -> T (nullable in Java)
//   std::vector<T>    -> List<T>
//   std::string       -> String
//   std::unique_ptr<T> -> T (Java has GC)

@CompiledJson
class UrlsItem {
    @JsonAttribute(name = "url")
    public String url = "";
    
    @JsonAttribute(name = "expanded_url")
    public String expandedUrl = "";
    
    @JsonAttribute(name = "display_url")
    public String displayUrl = "";
    
    @JsonAttribute(name = "indices")
    public List<Double> indices = List.of();
}

@CompiledJson
class Metadata {
    @JsonAttribute(name = "result_type")
    public String resultType;
    
    @JsonAttribute(name = "iso_language_code")
    public String isoLanguageCode;
}

@CompiledJson
class HashtagsItem {
    @JsonAttribute(name = "text")
    public String text = "";
    
    @JsonAttribute(name = "indices")
    public List<Double> indices = List.of();
}

@CompiledJson
class UserMentionsItem {
    @JsonAttribute(name = "screen_name")
    public String screenName = "";
    
    @JsonAttribute(name = "name")
    public String name = "";
    
    @JsonAttribute(name = "id")
    public double id;
    
    @JsonAttribute(name = "id_str")
    public String idStr = "";
    
    @JsonAttribute(name = "indices")
    public List<Double> indices = List.of();
}

@CompiledJson
class SizeClass {
    @JsonAttribute(name = "w")
    public Double w;
    
    @JsonAttribute(name = "h")
    public Double h;
    
    @JsonAttribute(name = "resize")
    public String resize;
}

@CompiledJson
class SizesClass {
    @JsonAttribute(name = "medium")
    public SizeClass medium;
    
    @JsonAttribute(name = "small")
    public SizeClass small;
    
    @JsonAttribute(name = "thumb")
    public SizeClass thumb;
    
    @JsonAttribute(name = "large")
    public SizeClass large;
}

@CompiledJson
class MediaItem {
    @JsonAttribute(name = "id")
    public double id;
    
    @JsonAttribute(name = "id_str")
    public String idStr = "";
    
    @JsonAttribute(name = "indices")
    public List<Double> indices = List.of();
    
    @JsonAttribute(name = "media_url")
    public String mediaUrl = "";
    
    @JsonAttribute(name = "media_url_https")
    public String mediaUrlHttps = "";
    
    @JsonAttribute(name = "url")
    public String url = "";
    
    @JsonAttribute(name = "display_url")
    public String displayUrl = "";
    
    @JsonAttribute(name = "expanded_url")
    public String expandedUrl = "";
    
    @JsonAttribute(name = "type")
    public String type = "";
    
    @JsonAttribute(name = "sizes")
    public SizesClass sizes = new SizesClass();
    
    @JsonAttribute(name = "source_status_id")
    public double sourceStatusId;
    
    @JsonAttribute(name = "source_status_id_str")
    public String sourceStatusIdStr = "";
}

@CompiledJson
class Entities {
    @JsonAttribute(name = "hashtags")
    public List<HashtagsItem> hashtags;
    
    @JsonAttribute(name = "symbols")
    public List<String> symbols;
    
    @JsonAttribute(name = "urls")
    public List<UrlsItem> urls;
    
    @JsonAttribute(name = "user_mentions")
    public List<UserMentionsItem> userMentions;
    
    @JsonAttribute(name = "media")
    public List<MediaItem> media;
}

@CompiledJson
class DescriptionClass {
    @JsonAttribute(name = "urls")
    public List<UrlsItem> urls;
}

@CompiledJson
class UrlClass {
    @JsonAttribute(name = "urls")
    public List<UrlsItem> urls;
}

@CompiledJson
class UserEntities {
    @JsonAttribute(name = "description")
    public DescriptionClass description;
    
    @JsonAttribute(name = "url")
    public UrlClass url;
}

@CompiledJson
class User {
    @JsonAttribute(name = "id")
    public Double id;
    
    @JsonAttribute(name = "id_str")
    public String idStr;
    
    @JsonAttribute(name = "name")
    public String name;
    
    @JsonAttribute(name = "screen_name")
    public String screenName;
    
    @JsonAttribute(name = "location")
    public String location;
    
    @JsonAttribute(name = "description")
    public String description;
    
    @JsonAttribute(name = "url")
    public String url;
    
    @JsonAttribute(name = "entities")
    public UserEntities entities;
    
    @JsonAttribute(name = "protected")
    public Boolean protectedField;
    
    @JsonAttribute(name = "followers_count")
    public Double followersCount;
    
    @JsonAttribute(name = "friends_count")
    public Double friendsCount;
    
    @JsonAttribute(name = "listed_count")
    public Double listedCount;
    
    @JsonAttribute(name = "created_at")
    public String createdAt;
    
    @JsonAttribute(name = "favourites_count")
    public Double favouritesCount;
    
    @JsonAttribute(name = "utc_offset")
    public Double utcOffset;
    
    @JsonAttribute(name = "time_zone")
    public String timeZone;
    
    @JsonAttribute(name = "geo_enabled")
    public Boolean geoEnabled;
    
    @JsonAttribute(name = "verified")
    public Boolean verified;
    
    @JsonAttribute(name = "statuses_count")
    public Double statusesCount;
    
    @JsonAttribute(name = "lang")
    public String lang;
    
    @JsonAttribute(name = "contributors_enabled")
    public Boolean contributorsEnabled;
    
    @JsonAttribute(name = "is_translator")
    public Boolean isTranslator;
    
    @JsonAttribute(name = "is_translation_enabled")
    public Boolean isTranslationEnabled;
    
    @JsonAttribute(name = "profile_background_color")
    public String profileBackgroundColor;
    
    @JsonAttribute(name = "profile_background_image_url")
    public String profileBackgroundImageUrl;
    
    @JsonAttribute(name = "profile_background_image_url_https")
    public String profileBackgroundImageUrlHttps;
    
    @JsonAttribute(name = "profile_background_tile")
    public Boolean profileBackgroundTile;
    
    @JsonAttribute(name = "profile_image_url")
    public String profileImageUrl;
    
    @JsonAttribute(name = "profile_image_url_https")
    public String profileImageUrlHttps;
    
    @JsonAttribute(name = "profile_banner_url")
    public String profileBannerUrl;
    
    @JsonAttribute(name = "profile_link_color")
    public String profileLinkColor;
    
    @JsonAttribute(name = "profile_sidebar_border_color")
    public String profileSidebarBorderColor;
    
    @JsonAttribute(name = "profile_sidebar_fill_color")
    public String profileSidebarFillColor;
    
    @JsonAttribute(name = "profile_text_color")
    public String profileTextColor;
    
    @JsonAttribute(name = "profile_use_background_image")
    public Boolean profileUseBackgroundImage;
    
    @JsonAttribute(name = "default_profile")
    public Boolean defaultProfile;
    
    @JsonAttribute(name = "default_profile_image")
    public Boolean defaultProfileImage;
    
    @JsonAttribute(name = "following")
    public Boolean following;
    
    @JsonAttribute(name = "follow_request_sent")
    public Boolean followRequestSent;
    
    @JsonAttribute(name = "notifications")
    public Boolean notifications;
}

@CompiledJson
class RetweetedStatusClass {
    @JsonAttribute(name = "metadata")
    public Metadata metadata;
    
    @JsonAttribute(name = "created_at")
    public String createdAt;
    
    @JsonAttribute(name = "id")
    public Double id;
    
    @JsonAttribute(name = "id_str")
    public String idStr;
    
    @JsonAttribute(name = "text")
    public String text;
    
    @JsonAttribute(name = "source")
    public String source;
    
    @JsonAttribute(name = "truncated")
    public Boolean truncated;
    
    @JsonAttribute(name = "in_reply_to_status_id")
    public Double inReplyToStatusId;
    
    @JsonAttribute(name = "in_reply_to_status_id_str")
    public String inReplyToStatusIdStr;
    
    @JsonAttribute(name = "in_reply_to_user_id")
    public Double inReplyToUserId;
    
    @JsonAttribute(name = "in_reply_to_user_id_str")
    public String inReplyToUserIdStr;
    
    @JsonAttribute(name = "in_reply_to_screen_name")
    public String inReplyToScreenName;
    
    @JsonAttribute(name = "user")
    public User user;
    
    @JsonAttribute(name = "geo")
    public Boolean geo;
    
    @JsonAttribute(name = "coordinates")
    public Boolean coordinates;
    
    @JsonAttribute(name = "place")
    public Boolean place;
    
    @JsonAttribute(name = "contributors")
    public Boolean contributors;
    
    @JsonAttribute(name = "retweet_count")
    public Double retweetCount;
    
    @JsonAttribute(name = "favorite_count")
    public Double favoriteCount;
    
    @JsonAttribute(name = "entities")
    public Entities entities;
    
    @JsonAttribute(name = "favorited")
    public Boolean favorited;
    
    @JsonAttribute(name = "retweeted")
    public Boolean retweeted;
    
    @JsonAttribute(name = "possibly_sensitive")
    public Boolean possiblySensitive;
    
    @JsonAttribute(name = "lang")
    public String lang;
}

@CompiledJson
class Status {
    @JsonAttribute(name = "metadata")
    public Metadata metadata = new Metadata();
    
    @JsonAttribute(name = "created_at")
    public String createdAt = "";
    
    @JsonAttribute(name = "id")
    public double id;
    
    @JsonAttribute(name = "id_str")
    public String idStr = "";
    
    @JsonAttribute(name = "text")
    public String text = "";
    
    @JsonAttribute(name = "source")
    public String source = "";
    
    @JsonAttribute(name = "truncated")
    public boolean truncated;
    
    @JsonAttribute(name = "in_reply_to_status_id")
    public Double inReplyToStatusId;
    
    @JsonAttribute(name = "in_reply_to_status_id_str")
    public String inReplyToStatusIdStr;
    
    @JsonAttribute(name = "in_reply_to_user_id")
    public Double inReplyToUserId;
    
    @JsonAttribute(name = "in_reply_to_user_id_str")
    public String inReplyToUserIdStr;
    
    @JsonAttribute(name = "in_reply_to_screen_name")
    public String inReplyToScreenName;
    
    @JsonAttribute(name = "user")
    public User user = new User();
    
    @JsonAttribute(name = "geo")
    public Boolean geo;
    
    @JsonAttribute(name = "coordinates")
    public Boolean coordinates;
    
    @JsonAttribute(name = "place")
    public Boolean place;
    
    @JsonAttribute(name = "contributors")
    public Boolean contributors;
    
    @JsonAttribute(name = "retweet_count")
    public double retweetCount;
    
    @JsonAttribute(name = "favorite_count")
    public double favoriteCount;
    
    @JsonAttribute(name = "entities")
    public Entities entities = new Entities();
    
    @JsonAttribute(name = "favorited")
    public boolean favorited;
    
    @JsonAttribute(name = "retweeted")
    public boolean retweeted;
    
    @JsonAttribute(name = "lang")
    public String lang = "";
    
    @JsonAttribute(name = "retweeted_status")
    public RetweetedStatusClass retweetedStatus = new RetweetedStatusClass();
    
    @JsonAttribute(name = "possibly_sensitive")
    public boolean possiblySensitive;
}

@CompiledJson
class SearchMetadata {
    @JsonAttribute(name = "completed_in")
    public Double completedIn;
    
    @JsonAttribute(name = "max_id")
    public Double maxId;
    
    @JsonAttribute(name = "max_id_str")
    public String maxIdStr;
    
    @JsonAttribute(name = "next_results")
    public String nextResults;
    
    @JsonAttribute(name = "query")
    public String query;
    
    @JsonAttribute(name = "refresh_url")
    public String refreshUrl;
    
    @JsonAttribute(name = "count")
    public Double count;
    
    @JsonAttribute(name = "since_id")
    public Double sinceId;
    
    @JsonAttribute(name = "since_id_str")
    public String sinceIdStr;
}

@CompiledJson
public class TwitterData {
    @JsonAttribute(name = "statuses")
    public List<Status> statuses;
    
    @JsonAttribute(name = "search_metadata")
    public SearchMetadata searchMetadata;
}

