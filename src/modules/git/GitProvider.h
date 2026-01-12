#ifndef GIT_PROVIDER_H
#define GIT_PROVIDER_H

#include <Arduino.h>
#include <vector>
#include <string>

// Common data structures for all Git providers
struct GitRepository {
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

struct GitIssue {
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

struct GitIssueCreate {
    String title;
    String body;
    std::vector<String> labels;
    std::vector<String> assignees;
    String milestone;
    bool draft;
};

struct GitIssueComment {
    int id;
    String body;
    String author;
    String createdAt;
    String updatedAt;
    String htmlUrl;
};

struct GitLabel {
    String name;
    String color;
    String description;
};

struct GitMilestone {
    String title;
    String description;
    int number;
    String state;
    String dueOn;
};

struct GitUser {
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

// Base provider configuration
struct GitProviderConfig {
    String token;
    String username;
    String defaultRepo;
    String apiBaseUrl;
    String providerName;
    bool authenticated;
};

// Supported Git providers
enum class GitProviderType {
    GITHUB,
    GITLAB,
    GITEE,
    CUSTOM
};

// Abstract base class for all Git providers
class GitProvider {
public:
    virtual ~GitProvider() = default;
    
    // Core functionality
    virtual bool begin(const String& token = "") = 0;
    virtual bool isAuthenticated() = 0;
    virtual void end() = 0;
    virtual String getProviderName() = 0;
    
    // Repository operations
    virtual std::vector<GitRepository> listUserRepos() = 0;
    virtual GitRepository getRepo(const String& owner, const String& repo) = 0;
    virtual bool createRepo(const String& name, const String& description = "", bool isPrivate = false) = 0;
    virtual bool deleteRepo(const String& owner, const String& repo) = 0;
    
    // Issue operations
    virtual std::vector<GitIssue> listIssues(const String& owner, const String& repo, const String& state = "open") = 0;
    virtual GitIssue getIssue(const String& owner, const String& repo, int issueNumber) = 0;
    virtual bool createIssue(const String& owner, const String& repo, const String& title, const String& body = "") = 0;
    virtual bool createIssueEx(const String& owner, const String& repo, const GitIssueCreate& issueData) = 0;
    virtual bool closeIssue(const String& owner, const String& repo, int issueNumber) = 0;
    virtual bool reopenIssue(const String& owner, const String& repo, int issueNumber) = 0;
    virtual bool updateIssue(const String& owner, const String& repo, int issueNumber, const GitIssueCreate& issueData) = 0;
    
    // Issue comment operations
    virtual std::vector<GitIssueComment> listIssueComments(const String& owner, const String& repo, int issueNumber) = 0;
    virtual bool addIssueComment(const String& owner, const String& repo, int issueNumber, const String& comment) = 0;
    
    // Label operations
    virtual std::vector<GitLabel> listLabels(const String& owner, const String& repo) = 0;
    virtual bool addLabelToIssue(const String& owner, const String& repo, int issueNumber, const String& label) = 0;
    virtual std::vector<String> getAvailableLabels(const String& owner, const String& repo) = 0;
    
    // Collaborator operations
    virtual std::vector<GitUser> listAssignees(const String& owner, const String& repo) = 0;
    virtual bool addAssigneeToIssue(const String& owner, const String& repo, int issueNumber, const String& assignee) = 0;
    virtual std::vector<String> getAvailableAssignees(const String& owner, const String& repo) = 0;
    
    // Milestone operations
    virtual std::vector<GitMilestone> listMilestones(const String& owner, const String& repo) = 0;
    virtual std::vector<String> getAvailableMilestones(const String& owner, const String& repo) = 0;
    
    // User operations
    virtual GitUser getUserInfo(const String& username = "") = 0;
    
    // Gist operations (if supported)
    virtual String createGist(const String& description, const String& filename, const String& content, bool isPublic = false) { return ""; }
    virtual bool deleteGist(const String& gistId) { return false; }
    
    // File operations
    virtual String getFileContent(const String& owner, const String& repo, const String& path, const String& ref = "main") = 0;
    virtual bool createFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& branch = "main") = 0;
    virtual bool updateFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& sha, const String& branch = "main") = 0;
    
    // Search operations
    virtual std::vector<GitRepository> searchRepositories(const String& query, int perPage = 10) = 0;
    
    // Utility functions
    virtual String getLastError() = 0;
    virtual int getResponseCode() = 0;
    
    // Provider-specific configuration
    virtual void setApiBaseUrl(const String& url) = 0;
    virtual String getApiBaseUrl() = 0;
};

#endif // GIT_PROVIDER_H