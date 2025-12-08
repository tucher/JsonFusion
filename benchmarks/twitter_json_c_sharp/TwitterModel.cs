using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace TwitterBenchmark;

// Equivalent C# model to twitter_model_generic.hpp
// Mapping:
//   std::optional<T>  -> T?
//   std::vector<T>    -> List<T>
//   std::string       -> string
//   std::unique_ptr<T> -> T? (C# has GC, nullable reference types)

public class UrlsItem
{
    [JsonPropertyName("url")]
    public string Url { get; set; } = "";
    
    [JsonPropertyName("expanded_url")]
    public string ExpandedUrl { get; set; } = "";
    
    [JsonPropertyName("display_url")]
    public string DisplayUrl { get; set; } = "";
    
    [JsonPropertyName("indices")]
    public List<double> Indices { get; set; } = new();
}

public class Metadata
{
    [JsonPropertyName("result_type")]
    public string? ResultType { get; set; }
    
    [JsonPropertyName("iso_language_code")]
    public string? IsoLanguageCode { get; set; }
}

public class Entities
{
    public class HashtagsItem
    {
        [JsonPropertyName("text")]
        public string Text { get; set; } = "";
        
        [JsonPropertyName("indices")]
        public List<double> Indices { get; set; } = new();
    }

    public class UserMentionsItem
    {
        [JsonPropertyName("screen_name")]
        public string ScreenName { get; set; } = "";
        
        [JsonPropertyName("name")]
        public string Name { get; set; } = "";
        
        [JsonPropertyName("id")]
        public double Id { get; set; }
        
        [JsonPropertyName("id_str")]
        public string IdStr { get; set; } = "";
        
        [JsonPropertyName("indices")]
        public List<double> Indices { get; set; } = new();
    }
    
    public class MediaItem
    {
        public class SizesClass
        {
            public class MediumClass
            {
                [JsonPropertyName("w")]
                public double? W { get; set; }
                
                [JsonPropertyName("h")]
                public double? H { get; set; }
                
                [JsonPropertyName("resize")]
                public string? Resize { get; set; }
            }
            
            public class SmallClass
            {
                [JsonPropertyName("w")]
                public double? W { get; set; }
                
                [JsonPropertyName("h")]
                public double? H { get; set; }
                
                [JsonPropertyName("resize")]
                public string? Resize { get; set; }
            }
            
            public class ThumbClass
            {
                [JsonPropertyName("w")]
                public double? W { get; set; }
                
                [JsonPropertyName("h")]
                public double? H { get; set; }
                
                [JsonPropertyName("resize")]
                public string? Resize { get; set; }
            }
            
            public class LargeClass
            {
                [JsonPropertyName("w")]
                public double? W { get; set; }
                
                [JsonPropertyName("h")]
                public double? H { get; set; }
                
                [JsonPropertyName("resize")]
                public string? Resize { get; set; }
            }

            [JsonPropertyName("medium")]
            public MediumClass? Medium { get; set; }
            
            [JsonPropertyName("small")]
            public SmallClass? Small { get; set; }
            
            [JsonPropertyName("thumb")]
            public ThumbClass? Thumb { get; set; }
            
            [JsonPropertyName("large")]
            public LargeClass? Large { get; set; }
        }

        [JsonPropertyName("id")]
        public double Id { get; set; }
        
        [JsonPropertyName("id_str")]
        public string IdStr { get; set; } = "";
        
        [JsonPropertyName("indices")]
        public List<double> Indices { get; set; } = new();
        
        [JsonPropertyName("media_url")]
        public string MediaUrl { get; set; } = "";
        
        [JsonPropertyName("media_url_https")]
        public string MediaUrlHttps { get; set; } = "";
        
        [JsonPropertyName("url")]
        public string Url { get; set; } = "";
        
        [JsonPropertyName("display_url")]
        public string DisplayUrl { get; set; } = "";
        
        [JsonPropertyName("expanded_url")]
        public string ExpandedUrl { get; set; } = "";
        
        [JsonPropertyName("type")]
        public string Type { get; set; } = "";
        
        [JsonPropertyName("sizes")]
        public SizesClass Sizes { get; set; } = new();
        
        [JsonPropertyName("source_status_id")]
        public double SourceStatusId { get; set; }
        
        [JsonPropertyName("source_status_id_str")]
        public string SourceStatusIdStr { get; set; } = "";
    }

    [JsonPropertyName("hashtags")]
    public List<HashtagsItem>? Hashtags { get; set; }
    
    [JsonPropertyName("symbols")]
    public List<string>? Symbols { get; set; }
    
    [JsonPropertyName("urls")]
    public List<UrlsItem>? Urls { get; set; }
    
    [JsonPropertyName("user_mentions")]
    public List<UserMentionsItem>? UserMentions { get; set; }
    
    [JsonPropertyName("media")]
    public List<MediaItem>? Media { get; set; }
}

public class UserEntities
{
    public class DescriptionClass
    {
        [JsonPropertyName("urls")]
        public List<UrlsItem>? Urls { get; set; }
    }
    
    public class UrlClass
    {
        [JsonPropertyName("urls")]
        public List<UrlsItem>? Urls { get; set; }
    }

    [JsonPropertyName("description")]
    public DescriptionClass? Description { get; set; }
    
    [JsonPropertyName("url")]
    public UrlClass? Url { get; set; }
}

public class User
{
    [JsonPropertyName("id")]
    public double? Id { get; set; }
    
    [JsonPropertyName("id_str")]
    public string? IdStr { get; set; }
    
    [JsonPropertyName("name")]
    public string? Name { get; set; }
    
    [JsonPropertyName("screen_name")]
    public string? ScreenName { get; set; }
    
    [JsonPropertyName("location")]
    public string? Location { get; set; }
    
    [JsonPropertyName("description")]
    public string? Description { get; set; }
    
    [JsonPropertyName("url")]
    public string? Url { get; set; }
    
    [JsonPropertyName("entities")]
    public UserEntities? Entities { get; set; }
    
    [JsonPropertyName("protected")]  // C# handles keyword with attribute
    public bool? Protected { get; set; }
    
    [JsonPropertyName("followers_count")]
    public double? FollowersCount { get; set; }
    
    [JsonPropertyName("friends_count")]
    public double? FriendsCount { get; set; }
    
    [JsonPropertyName("listed_count")]
    public double? ListedCount { get; set; }
    
    [JsonPropertyName("created_at")]
    public string? CreatedAt { get; set; }
    
    [JsonPropertyName("favourites_count")]
    public double? FavouritesCount { get; set; }
    
    [JsonPropertyName("utc_offset")]
    public double? UtcOffset { get; set; }
    
    [JsonPropertyName("time_zone")]
    public string? TimeZone { get; set; }
    
    [JsonPropertyName("geo_enabled")]
    public bool? GeoEnabled { get; set; }
    
    [JsonPropertyName("verified")]
    public bool? Verified { get; set; }
    
    [JsonPropertyName("statuses_count")]
    public double? StatusesCount { get; set; }
    
    [JsonPropertyName("lang")]
    public string? Lang { get; set; }
    
    [JsonPropertyName("contributors_enabled")]
    public bool? ContributorsEnabled { get; set; }
    
    [JsonPropertyName("is_translator")]
    public bool? IsTranslator { get; set; }
    
    [JsonPropertyName("is_translation_enabled")]
    public bool? IsTranslationEnabled { get; set; }
    
    [JsonPropertyName("profile_background_color")]
    public string? ProfileBackgroundColor { get; set; }
    
    [JsonPropertyName("profile_background_image_url")]
    public string? ProfileBackgroundImageUrl { get; set; }
    
    [JsonPropertyName("profile_background_image_url_https")]
    public string? ProfileBackgroundImageUrlHttps { get; set; }
    
    [JsonPropertyName("profile_background_tile")]
    public bool? ProfileBackgroundTile { get; set; }
    
    [JsonPropertyName("profile_image_url")]
    public string? ProfileImageUrl { get; set; }
    
    [JsonPropertyName("profile_image_url_https")]
    public string? ProfileImageUrlHttps { get; set; }
    
    [JsonPropertyName("profile_banner_url")]
    public string? ProfileBannerUrl { get; set; }
    
    [JsonPropertyName("profile_link_color")]
    public string? ProfileLinkColor { get; set; }
    
    [JsonPropertyName("profile_sidebar_border_color")]
    public string? ProfileSidebarBorderColor { get; set; }
    
    [JsonPropertyName("profile_sidebar_fill_color")]
    public string? ProfileSidebarFillColor { get; set; }
    
    [JsonPropertyName("profile_text_color")]
    public string? ProfileTextColor { get; set; }
    
    [JsonPropertyName("profile_use_background_image")]
    public bool? ProfileUseBackgroundImage { get; set; }
    
    [JsonPropertyName("default_profile")]
    public bool? DefaultProfile { get; set; }
    
    [JsonPropertyName("default_profile_image")]
    public bool? DefaultProfileImage { get; set; }
    
    [JsonPropertyName("following")]
    public bool? Following { get; set; }
    
    [JsonPropertyName("follow_request_sent")]
    public bool? FollowRequestSent { get; set; }
    
    [JsonPropertyName("notifications")]
    public bool? Notifications { get; set; }
}

public class Status
{
    public class RetweetedStatusClass
    {
        [JsonPropertyName("metadata")]
        public Metadata? Metadata { get; set; }
        
        [JsonPropertyName("created_at")]
        public string? CreatedAt { get; set; }
        
        [JsonPropertyName("id")]
        public double? Id { get; set; }
        
        [JsonPropertyName("id_str")]
        public string? IdStr { get; set; }
        
        [JsonPropertyName("text")]
        public string? Text { get; set; }
        
        [JsonPropertyName("source")]
        public string? Source { get; set; }
        
        [JsonPropertyName("truncated")]
        public bool? Truncated { get; set; }
        
        [JsonPropertyName("in_reply_to_status_id")]
        public double? InReplyToStatusId { get; set; }
        
        [JsonPropertyName("in_reply_to_status_id_str")]
        public string? InReplyToStatusIdStr { get; set; }
        
        [JsonPropertyName("in_reply_to_user_id")]
        public double? InReplyToUserId { get; set; }
        
        [JsonPropertyName("in_reply_to_user_id_str")]
        public string? InReplyToUserIdStr { get; set; }
        
        [JsonPropertyName("in_reply_to_screen_name")]
        public string? InReplyToScreenName { get; set; }
        
        [JsonPropertyName("user")]
        public User? User { get; set; }
        
        [JsonPropertyName("geo")]
        public bool? Geo { get; set; }
        
        [JsonPropertyName("coordinates")]
        public bool? Coordinates { get; set; }
        
        [JsonPropertyName("place")]
        public bool? Place { get; set; }
        
        [JsonPropertyName("contributors")]
        public bool? Contributors { get; set; }
        
        [JsonPropertyName("retweet_count")]
        public double? RetweetCount { get; set; }
        
        [JsonPropertyName("favorite_count")]
        public double? FavoriteCount { get; set; }
        
        [JsonPropertyName("entities")]
        public Entities? Entities { get; set; }
        
        [JsonPropertyName("favorited")]
        public bool? Favorited { get; set; }
        
        [JsonPropertyName("retweeted")]
        public bool? Retweeted { get; set; }
        
        [JsonPropertyName("possibly_sensitive")]
        public bool? PossiblySensitive { get; set; }
        
        [JsonPropertyName("lang")]
        public string? Lang { get; set; }
    }

    [JsonPropertyName("metadata")]
    public Metadata Metadata { get; set; } = new();
    
    [JsonPropertyName("created_at")]
    public string CreatedAt { get; set; } = "";
    
    [JsonPropertyName("id")]
    public double Id { get; set; }
    
    [JsonPropertyName("id_str")]
    public string IdStr { get; set; } = "";
    
    [JsonPropertyName("text")]
    public string Text { get; set; } = "";
    
    [JsonPropertyName("source")]
    public string Source { get; set; } = "";
    
    [JsonPropertyName("truncated")]
    public bool Truncated { get; set; }
    
    [JsonPropertyName("in_reply_to_status_id")]
    public double? InReplyToStatusId { get; set; }
    
    [JsonPropertyName("in_reply_to_status_id_str")]
    public string? InReplyToStatusIdStr { get; set; }
    
    [JsonPropertyName("in_reply_to_user_id")]
    public double? InReplyToUserId { get; set; }
    
    [JsonPropertyName("in_reply_to_user_id_str")]
    public string? InReplyToUserIdStr { get; set; }
    
    [JsonPropertyName("in_reply_to_screen_name")]
    public string? InReplyToScreenName { get; set; }
    
    [JsonPropertyName("user")]
    public User User { get; set; } = new();
    
    [JsonPropertyName("geo")]
    public bool? Geo { get; set; }
    
    [JsonPropertyName("coordinates")]
    public bool? Coordinates { get; set; }
    
    [JsonPropertyName("place")]
    public bool? Place { get; set; }
    
    [JsonPropertyName("contributors")]
    public bool? Contributors { get; set; }
    
    [JsonPropertyName("retweet_count")]
    public double RetweetCount { get; set; }
    
    [JsonPropertyName("favorite_count")]
    public double FavoriteCount { get; set; }
    
    [JsonPropertyName("entities")]
    public Entities Entities { get; set; } = new();
    
    [JsonPropertyName("favorited")]
    public bool Favorited { get; set; }
    
    [JsonPropertyName("retweeted")]
    public bool Retweeted { get; set; }
    
    [JsonPropertyName("lang")]
    public string Lang { get; set; } = "";
    
    [JsonPropertyName("retweeted_status")]
    public RetweetedStatusClass RetweetedStatus { get; set; } = new();
    
    [JsonPropertyName("possibly_sensitive")]
    public bool PossiblySensitive { get; set; }
}

public class SearchMetadata
{
    [JsonPropertyName("completed_in")]
    public double? CompletedIn { get; set; }
    
    [JsonPropertyName("max_id")]
    public double? MaxId { get; set; }
    
    [JsonPropertyName("max_id_str")]
    public string? MaxIdStr { get; set; }
    
    [JsonPropertyName("next_results")]
    public string? NextResults { get; set; }
    
    [JsonPropertyName("query")]
    public string? Query { get; set; }
    
    [JsonPropertyName("refresh_url")]
    public string? RefreshUrl { get; set; }
    
    [JsonPropertyName("count")]
    public double? Count { get; set; }
    
    [JsonPropertyName("since_id")]
    public double? SinceId { get; set; }
    
    [JsonPropertyName("since_id_str")]
    public string? SinceIdStr { get; set; }
}

public class TwitterData
{
    [JsonPropertyName("statuses")]
    public List<Status>? Statuses { get; set; }
    
    [JsonPropertyName("search_metadata")]
    public SearchMetadata? SearchMetadata { get; set; }
}

