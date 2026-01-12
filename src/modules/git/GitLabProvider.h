#ifndef GITLAB_PROVIDER_H
#define GITLAB_PROVIDER_H

#include "GitProvider.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// GitLab API endpoints
#define GITLAB_API_BASE "https://gitlab.com/api/v4"
#define GITLAB_TOKEN_HEADER "PRIVATE-TOKEN: "
#define GITLAB_USER_AGENT "Bruce-ESP32/1.0"

class GitLabProvider : public GitProvider {
public:
    GitLabProvider();
    ~GitLabProvider();
    
    // Core functionality
    bool begin(const String& token = "") override;
    bool isAuthenticated() override;
    void end() override;
    String getProviderName() override { return "GitLab"; }
    
    // Repository operations
    std::vector<GitRepository> listUserRepos() override;
    GitRepository getRepo(const String& owner, const String& repo) override;
    bool createRepo(const String& name, const String& description = "", bool isPrivate = false) override;
    bool deleteRepo(const String& owner, const String& repo) override;
    
    // Issue operations
    std::vector<GitIssue> listIssues(const String& owner, const String& repo, const String& state = "open") override;
    GitIssue getIssue(const String& owner, const String& repo, int issueNumber) override;
    bool createIssue(const String& owner, const String& repo, const String& title, const String& body = "") override;
    bool createIssueEx(const String& owner, const String& repo, const GitIssueCreate& issueData) override;
    bool closeIssue(const String& owner, const String& repo, int issueNumber) override;
    bool reopenIssue(const String& owner, const String& repo, int issueNumber) override;
    bool updateIssue(const String& owner, const String& repo, int issueNumber, const GitIssueCreate& issueData) override;
    
    // Issue comment operations
    std::vector<GitIssueComment> listIssueComments(const String& owner, const String& repo, int issueNumber) override;
    bool addIssueComment(const String& owner, const String& repo, int issueNumber, const String& comment) override;
    
    // Label operations
    std::vector<GitLabel> listLabels(const String& owner, const String& repo) override;
    bool addLabelToIssue(const String& owner, const String& repo, int issueNumber, const String& label) override;
    std::vector<String> getAvailableLabels(const String& owner, const String& repo) override;
    
    // Collaborator operations - Note: GitLab uses members API
    std::vector<GitUser> listAssignees(const String& owner, const String& repo) override;
    bool addAssigneeToIssue(const String& owner, const String& repo, int issueNumber, const String& assignee) override;
    std::vector<String> getAvailableAssignees(const String& owner, const String& repo) override;
    
    // Milestone operations
    std::vector<GitMilestone> listMilestones(const String& owner, const String& repo) override;
    std::vector<String> getAvailableMilestones(const String& owner, const String& repo) override;
    
    // User operations
    GitUser getUserInfo(const String& username = "") override;
    
    // File operations
    String getFileContent(const String& owner, const String& repo, const String& path, const String& ref = "main") override;
    bool createFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& branch = "main") override;
    bool updateFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& sha, const String& branch = "main") override;
    
    // Search operations
    std::vector<GitRepository> searchRepositories(const String& query, int perPage = 10) override;
    
    // Utility functions
    String getLastError() override { return lastError; }
    int getResponseCode() override { return responseCode; }
    
    // Provider-specific configuration
    void setApiBaseUrl(const String& url) override;
    String getApiBaseUrl() override { return config.apiBaseUrl; }
    
private:
    GitProviderConfig config;
    HTTPClient http;
    String lastError;
    int responseCode;
    
    // GitLab-specific: We need project ID for most operations
    String getProjectId(const String& owner, const String& repo);
    
    // HTTP helper methods
    bool makeRequest(const String& method, const String& url, const String& data = "");
    String buildUrl(const String& endpoint, const String& params = "");
    
    // Authentication
    void setAuthHeader(const String& token);
    void clearAuthHeader();
    
    // JSON parsing helpers
    bool parseRepoFromJson(const String& json, GitRepository& repo);
    bool parseIssueFromJson(const String& json, GitIssue& issue);
    bool parseUserFromJson(const String& json, GitUser& user);
    bool parseReposArray(const String& json, std::vector<GitRepository>& repos);
    bool parseIssuesArray(const String& json, std::vector<GitIssue>& issues);
    bool parseUsersArray(const String& json, std::vector<GitUser>& users);
    
    // GitLab-specific parsing
    String extractGitLabJsonValue(const String& json, const String& key);
};

#endif // GITLAB_PROVIDER_H