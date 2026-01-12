#ifndef GITHUB_APP_H
#define GITHUB_APP_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string>
#include <vector>

// GitHub API endpoints
#define GITHUB_API_BASE "https://api.github.com"
#define GITHUB_TOKEN_HEADER "Authorization: token "
#define USER_AGENT "Bruce-ESP32/1.0"

// Repository information structure
struct GitHubRepo {
    String name;
    String fullName;
    String description;
    String cloneUrl;
    String sshUrl;
    String htmlUrl;
    bool isPrivate;
    String defaultBranch;
    int stars;
    int forks;
};

// Issue information structure
struct GitHubIssue {
    int number;
    String title;
    String body;
    String state;
    String author;
    String createdAt;
    String updatedAt;
    String htmlUrl;
};

// User information structure
struct GitHubUser {
    String login;
    String name;
    String email;
    String bio;
    String avatarUrl;
    String htmlUrl;
    int publicRepos;
    int followers;
    int following;
};

// GitHub app configuration
struct GitHubConfig {
    String token;
    String username;
    String defaultRepo;
    bool authenticated;
};

class GitHubApp {
public:
    GitHubApp();
    ~GitHubApp();
    
    // Core functionality
    bool begin(const String& token = "");
    bool isAuthenticated();
    void end();
    
    // Repository operations
    std::vector<GitHubRepo> listUserRepos();
    GitHubRepo getRepo(const String& owner, const String& repo);
    bool createRepo(const String& name, const String& description = "", bool isPrivate = false);
    bool deleteRepo(const String& owner, const String& repo);
    
    // Issue operations
    std::vector<GitHubIssue> listIssues(const String& owner, const String& repo, const String& state = "open");
    GitHubIssue getIssue(const String& owner, const String& repo, int issueNumber);
    bool createIssue(const String& owner, const String& repo, const String& title, const String& body = "");
    bool closeIssue(const String& owner, const String& repo, int issueNumber);
    
    // User operations
    GitHubUser getUserInfo(const String& username = "");
    std::vector<GitHubUser> listUserFollowers(const String& username = "");
    std::vector<GitHubUser> listUserFollowing(const String& username = "");
    
    // Gist operations
    String createGist(const String& description, const String& filename, const String& content, bool isPublic = false);
    bool deleteGist(const String& gistId);
    
    // File operations
    String getFileContent(const String& owner, const String& repo, const String& path, const String& ref = "main");
    bool createFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& branch = "main");
    bool updateFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& sha, const String& branch = "main");
    bool deleteFile(const String& owner, const String& repo, const String& path, const String& message, const String& sha, const String& branch = "main");
    
    // Search operations
    std::vector<GitHubRepo> searchRepositories(const String& query, int perPage = 10);
    std::vector<GitHubUser> searchUsers(const String& query, int perPage = 10);
    
    // Webhook operations
    bool createWebhook(const String& owner, const String& repo, const String& url, const String& events = "push");
    bool deleteWebhook(const String& owner, const String& repo, const String& url);
    
    // Utility functions
    String getLastError() { return lastError; }
    int getResponseCode() { return responseCode; }
    
private:
    GitHubConfig config;
    HTTPClient http;
    String lastError;
    int responseCode;
    
    // HTTP helper methods
    bool makeRequest(const String& method, const String& url, const String& data = "");
    String buildUrl(const String& endpoint, const String& params = "");
    String extractJsonValue(const String& json, const String& key);
    
    // Authentication
    void setAuthHeader(const String& token);
    void clearAuthHeader();
    
    // JSON parsing helpers
    bool parseRepoFromJson(const String& json, GitHubRepo& repo);
    bool parseIssueFromJson(const String& json, GitHubIssue& issue);
    bool parseUserFromJson(const String& json, GitHubUser& user);
    bool parseReposArray(const String& json, std::vector<GitHubRepo>& repos);
    bool parseIssuesArray(const String& json, std::vector<GitHubIssue>& issues);
    bool parseUsersArray(const String& json, std::vector<GitHubUser>& users);
};

// Global instance
extern GitHubApp githubApp;

#endif // GITHUB_APP_H