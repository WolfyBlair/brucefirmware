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
    std::vector<String> labels;
    std::vector<String> assignees;
    String milestone;
    int comments;
    bool isPullRequest;
};

// Enhanced issue creation structure
struct GitHubIssueCreate {
    String title;
    String body;
    std::vector<String> labels;
    std::vector<String> assignees;
    String milestone;
    bool draft;
};

// Issue comment structure
struct GitHubIssueComment {
    int id;
    String body;
    String author;
    String createdAt;
    String updatedAt;
    String htmlUrl;
};

// Label structure
struct GitHubLabel {
    String name;
    String color;
    String description;
};

// Milestone structure
struct GitHubMilestone {
    String title;
    String description;
    int number;
    String state;
    String dueOn;
};

// Issue template structure
struct GitHubIssueTemplate {
    String name;
    String description;
    String content;
    String labels;
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
    bool createIssueEx(const String& owner, const String& repo, const GitHubIssueCreate& issueData);
    bool closeIssue(const String& owner, const String& repo, int issueNumber);
    bool reopenIssue(const String& owner, const String& repo, int issueNumber);
    bool updateIssue(const String& owner, const String& repo, int issueNumber, const GitHubIssueCreate& issueData);
    
    // Issue comment operations
    std::vector<GitHubIssueComment> listIssueComments(const String& owner, const String& repo, int issueNumber);
    bool addIssueComment(const String& owner, const String& repo, int issueNumber, const String& comment);
    bool editIssueComment(const String& owner, const String& repo, int commentId, const String& comment);
    bool deleteIssueComment(const String& owner, const String& repo, int commentId);
    
    // Issue enhancement operations
    std::vector<GitHubLabel> listLabels(const String& owner, const String& repo);
    bool addLabelToIssue(const String& owner, const String& repo, int issueNumber, const String& label);
    bool removeLabelFromIssue(const String& owner, const String& repo, int issueNumber, const String& label);
    std::vector<String> getAvailableLabels(const String& owner, const String& repo);
    
    // Collaborator operations
    std::vector<GitHubUser> listAssignees(const String& owner, const String& repo);
    bool addAssigneeToIssue(const String& owner, const String& repo, int issueNumber, const String& assignee);
    bool removeAssigneeFromIssue(const String& owner, const String& repo, int issueNumber, const String& assignee);
    std::vector<String> getAvailableAssignees(const String& owner, const String& repo);
    
    // Milestone operations
    std::vector<GitHubMilestone> listMilestones(const String& owner, const String& repo);
    bool addMilestoneToIssue(const String& owner, const String& repo, int issueNumber, const String& milestone);
    bool removeMilestoneFromIssue(const String& owner, const String& repo, int issueNumber);
    std::vector<String> getAvailableMilestones(const String& owner, const String& repo);
    
    // Issue templates
    std::vector<GitHubIssueTemplate> listIssueTemplates(const String& owner, const String& repo);
    String getIssueTemplateContent(const String& owner, const String& repo, const String& templateName);
    
    // Utility functions
    String formatIssueBody(const String& title, const String& description, const String& template = "");
    String generateIssueFromTemplate(const GitHubIssueTemplate& template, const std::vector<String>& variables);
    bool validateIssueData(const GitHubIssueCreate& issueData);
    
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