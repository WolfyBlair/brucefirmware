#include "github_app.h"
#include "core/display.h"
#include "core/utils.h"

// Global instance
GitHubApp githubApp;

GitHubApp::GitHubApp() {
    config.token = "";
    config.username = "";
    config.defaultRepo = "";
    config.authenticated = false;
    lastError = "";
    responseCode = 0;
}

GitHubApp::~GitHubApp() {
    end();
}

bool GitHubApp::begin(const String& token) {
    if (token.length() > 0) {
        config.token = token;
    }
    
    if (config.token.length() == 0) {
        lastError = "No GitHub token provided";
        return false;
    }
    
    // Test authentication by getting user info
    GitHubUser user = getUserInfo();
    if (user.login.length() > 0) {
        config.authenticated = true;
        config.username = user.login;
        return true;
    }
    
    return false;
}

void GitHubApp::end() {
    http.end();
    config.authenticated = false;
    lastError = "";
}

bool GitHubApp::isAuthenticated() {
    return config.authenticated;
}

bool GitHubApp::makeRequest(const String& method, const String& url, const String& data) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    http.begin(url.c_str());
    http.setUserAgent(USER_AGENT);
    setAuthHeader(config.token);
    http.addHeader("Content-Type", "application/json");
    
    responseCode = -1;
    String response;
    
    if (method == "GET") {
        responseCode = http.GET();
    } else if (method == "POST") {
        responseCode = http.POST(data);
    } else if (method == "PUT") {
        http.addHeader("Content-Length", String(data.length()));
        responseCode = http.PUT(data);
    } else if (method == "PATCH") {
        responseCode = http.PATCH(data);
    } else if (method == "DELETE") {
        responseCode = http.sendRequest("DELETE", data);
    }
    
    if (responseCode > 0) {
        response = http.getString();
        if (responseCode >= 200 && responseCode < 300) {
            lastError = "";
            return true;
        } else {
            lastError = "HTTP " + String(responseCode) + ": " + response;
            return false;
        }
    } else {
        lastError = "Connection failed: " + http.errorToString(responseCode);
        return false;
    }
}

String GitHubApp::buildUrl(const String& endpoint, const String& params) {
    String url = GITHUB_API_BASE + endpoint;
    if (params.length() > 0) {
        url += "?" + params;
    }
    return url;
}

void GitHubApp::setAuthHeader(const String& token) {
    http.addHeader("Authorization", String(GITHUB_TOKEN_HEADER) + token);
}

void GitHubApp::clearAuthHeader() {
    http.addHeader("Authorization", "");
}

std::vector<GitHubRepo> GitHubApp::listUserRepos() {
    std::vector<GitHubRepo> repos;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return repos;
    }
    
    String url = buildUrl("/user/repos", "per_page=100&sort=updated");
    http.begin(url.c_str());
    http.setUserAgent(USER_AGENT);
    setAuthHeader(config.token);
    
    responseCode = http.GET();
    
    if (responseCode == 200) {
        String response = http.getString();
        // Simple JSON parsing for repositories array
        int start = response.indexOf("[");
        int end = response.lastIndexOf("]");
        
        if (start >= 0 && end > start) {
            String arrayJson = response.substring(start, end + 1);
            parseReposArray(arrayJson, repos);
        }
    } else {
        lastError = "Failed to fetch repositories: HTTP " + String(responseCode);
    }
    
    http.end();
    return repos;
}

GitHubRepo GitHubApp::getRepo(const String& owner, const String& repo) {
    GitHubRepo result;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return result;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo);
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        parseRepoFromJson(response, result);
    }
    
    http.end();
    return result;
}

GitHubUser GitHubApp::getUserInfo(const String& username) {
    GitHubUser result;
    
    String user = username.length() > 0 ? username : config.username;
    if (user.length() == 0) {
        lastError = "No username available";
        return result;
    }
    
    String url = buildUrl("/users/" + user);
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        parseUserFromJson(response, result);
    }
    
    http.end();
    return result;
}

bool GitHubApp::createRepo(const String& name, const String& description, bool isPrivate) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/user/repos");
    
    String data = "{";
    data += "\"name\":\"" + name + "\",";
    data += "\"description\":\"" + description + "\",";
    data += "\"private\":" + (isPrivate ? "true" : "false");
    data += "}";
    
    bool success = makeRequest("POST", url, data);
    http.end();
    return success;
}

std::vector<GitHubIssue> GitHubApp::listIssues(const String& owner, const String& repo, const String& state) {
    std::vector<GitHubIssue> issues;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return issues;
    }
    
    String params = "state=" + state + "&per_page=100";
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues", params);
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        parseIssuesArray(response, issues);
    }
    
    http.end();
    return issues;
}

GitHubIssue GitHubApp::getIssue(const String& owner, const String& repo, int issueNumber) {
    GitHubIssue result;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return result;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        parseIssueFromJson(response, result);
    }
    
    http.end();
    return result;
}

bool GitHubApp::createIssueEx(const String& owner, const String& repo, const GitHubIssueCreate& issueData) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    if (!validateIssueData(issueData)) {
        lastError = "Invalid issue data";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues");
    
    // Build JSON payload
    String data = "{";
    data += "\"title\":\"" + escapeJson(issueData.title) + "\",";
    data += "\"body\":\"" + escapeJson(issueData.body) + "\"";
    
    // Add labels if provided
    if (issueData.labels.size() > 0) {
        data += ",\"labels\":[";
        for (size_t i = 0; i < issueData.labels.size(); i++) {
            if (i > 0) data += ",";
            data += "\"" + issueData.labels[i] + "\"";
        }
        data += "]";
    }
    
    // Add assignees if provided
    if (issueData.assignees.size() > 0) {
        data += ",\"assignees\":[";
        for (size_t i = 0; i < issueData.assignees.size(); i++) {
            if (i > 0) data += ",";
            data += "\"" + issueData.assignees[i] + "\"";
        }
        data += "]";
    }
    
    // Add milestone if provided
    if (issueData.milestone.length() > 0) {
        data += ",\"milestone\":\"" + issueData.milestone + "\"";
    }
    
    // Add draft flag if true
    if (issueData.draft) {
        data += ",\"draft\":true";
    }
    
    data += "}";
    
    bool success = makeRequest("POST", url, data);
    http.end();
    return success;
}

bool GitHubApp::reopenIssue(const String& owner, const String& repo, int issueNumber) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    
    String data = "{\"state\":\"open\"}";
    
    bool success = makeRequest("PATCH", url, data);
    http.end();
    return success;
}

bool GitHubApp::updateIssue(const String& owner, const String& repo, int issueNumber, const GitHubIssueCreate& issueData) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    if (!validateIssueData(issueData)) {
        lastError = "Invalid issue data";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    
    // Build JSON payload for update
    String data = "{";
    data += "\"title\":\"" + escapeJson(issueData.title) + "\",";
    data += "\"body\":\"" + escapeJson(issueData.body) + "\"";
    
    // Note: For updates, labels and assignees need to be managed separately
    // through the specific endpoints
    
    data += "}";
    
    bool success = makeRequest("PATCH", url, data);
    http.end();
    return success;
}

std::vector<GitHubIssueComment> GitHubApp::listIssueComments(const String& owner, const String& repo, int issueNumber) {
    std::vector<GitHubIssueComment> comments;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return comments;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber) + "/comments");
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        // Simple parsing for comments array
        int start = 0;
        while (true) {
            int objStart = response.indexOf("{", start);
            if (objStart < 0) break;
            
            int objEnd = response.indexOf("}", objStart);
            if (objEnd < 0) break;
            
            String objJson = response.substring(objStart, objEnd + 1);
            GitHubIssueComment comment;
            
            comment.id = extractJsonValue(objJson, "\"id\"").toInt();
            comment.body = extractJsonValue(objJson, "\"body\"");
            comment.createdAt = extractJsonValue(objJson, "\"created_at\"");
            comment.updatedAt = extractJsonValue(objJson, "\"updated_at\"");
            comment.htmlUrl = extractJsonValue(objJson, "\"html_url\"");
            
            // Get author from user object
            int userStart = objJson.indexOf("\"user\":{");
            if (userStart >= 0) {
                int userEnd = objJson.indexOf("}", userStart);
                String userJson = objJson.substring(userStart + 7, userEnd + 1);
                comment.author = extractJsonValue(userJson, "\"login\"");
            }
            
            comments.push_back(comment);
            start = objEnd + 1;
        }
    }
    
    http.end();
    return comments;
}

bool GitHubApp::addIssueComment(const String& owner, const String& repo, int issueNumber, const String& comment) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber) + "/comments");
    
    String data = "{\"body\":\"" + escapeJson(comment) + "\"}";
    
    bool success = makeRequest("POST", url, data);
    http.end();
    return success;
}

std::vector<GitHubLabel> GitHubApp::listLabels(const String& owner, const String& repo) {
    std::vector<GitHubLabel> labels;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return labels;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/labels");
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        // Simple parsing for labels array
        int start = 0;
        while (true) {
            int objStart = response.indexOf("{", start);
            if (objStart < 0) break;
            
            int objEnd = response.indexOf("}", objStart);
            if (objEnd < 0) break;
            
            String objJson = response.substring(objStart, objEnd + 1);
            GitHubLabel label;
            
            label.name = extractJsonValue(objJson, "\"name\"");
            label.color = extractJsonValue(objJson, "\"color\"");
            label.description = extractJsonValue(objJson, "\"description\"");
            
            labels.push_back(label);
            start = objEnd + 1;
        }
    }
    
    http.end();
    return labels;
}

bool GitHubApp::addLabelToIssue(const String& owner, const String& repo, int issueNumber, const String& label) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber) + "/labels");
    
    String data = "[\"" + label + "\"]";
    
    bool success = makeRequest("POST", url, data);
    http.end();
    return success;
}

std::vector<String> GitHubApp::getAvailableLabels(const String& owner, const String& repo) {
    std::vector<String> labelNames;
    auto labels = listLabels(owner, repo);
    
    for (const auto& label : labels) {
        labelNames.push_back(label.name);
    }
    
    return labelNames;
}

std::vector<GitHubUser> GitHubApp::listAssignees(const String& owner, const String& repo) {
    std::vector<GitHubUser> assignees;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return assignees;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/assignees");
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        parseUsersArray(response, assignees);
    }
    
    http.end();
    return assignees;
}

bool GitHubApp::addAssigneeToIssue(const String& owner, const String& repo, int issueNumber, const String& assignee) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber) + "/assignees");
    
    String data = "{\"assignees\":[\"" + assignee + "\"]}";
    
    bool success = makeRequest("POST", url, data);
    http.end();
    return success;
}

std::vector<String> GitHubApp::getAvailableAssignees(const String& owner, const String& repo) {
    std::vector<String> assigneeLogins;
    auto assignees = listAssignees(owner, repo);
    
    for (const auto& assignee : assignees) {
        assigneeLogins.push_back(assignee.login);
    }
    
    return assigneeLogins;
}

std::vector<GitHubMilestone> GitHubApp::listMilestones(const String& owner, const String& repo) {
    std::vector<GitHubMilestone> milestones;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return milestones;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/milestones");
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        // Simple parsing for milestones array
        int start = 0;
        while (true) {
            int objStart = response.indexOf("{", start);
            if (objStart < 0) break;
            
            int objEnd = response.indexOf("}", objStart);
            if (objEnd < 0) break;
            
            String objJson = response.substring(objStart, objEnd + 1);
            GitHubMilestone milestone;
            
            milestone.number = extractJsonValue(objJson, "\"number\"").toInt();
            milestone.title = extractJsonValue(objJson, "\"title\"");
            milestone.description = extractJsonValue(objJson, "\"description\"");
            milestone.state = extractJsonValue(objJson, "\"state\"");
            milestone.dueOn = extractJsonValue(objJson, "\"due_on\"");
            
            milestones.push_back(milestone);
            start = objEnd + 1;
        }
    }
    
    http.end();
    return milestones;
}

bool GitHubApp::removeLabelFromIssue(const String& owner, const String& repo, int issueNumber, const String& label) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber) + "/labels/" + urlencode(label));
    
    bool success = makeRequest("DELETE", url);
    http.end();
    return success;
}

bool GitHubApp::removeAssigneeFromIssue(const String& owner, const String& repo, int issueNumber, const String& assignee) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber) + "/assignees/" + assignee);
    
    String data = "{\"assignees\":[\"" + assignee + "\"]}";
    
    bool success = makeRequest("DELETE", url, data);
    http.end();
    return success;
}

bool GitHubApp::removeMilestoneFromIssue(const String& owner, const String& repo, int issueNumber) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    
    String data = "{\"milestone\":null}";
    
    bool success = makeRequest("PATCH", url, data);
    http.end();
    return success;
}

bool GitHubApp::addMilestoneToIssue(const String& owner, const String& repo, int issueNumber, const String& milestone) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    
    String data = "{\"milestone\":\"" + milestone + "\"}";
    
    bool success = makeRequest("PATCH", url, data);
    http.end();
    return success;
}

bool GitHubApp::editIssueComment(const String& owner, const String& repo, int commentId, const String& comment) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/comments/" + String(commentId));
    
    String data = "{\"body\":\"" + escapeJson(comment) + "\"}";
    
    bool success = makeRequest("PATCH", url, data);
    http.end();
    return success;
}

bool GitHubApp::deleteIssueComment(const String& owner, const String& repo, int commentId) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/comments/" + String(commentId));
    
    bool success = makeRequest("DELETE", url);
    http.end();
    return success;
}

String GitHubApp::generateIssueFromTemplate(const GitHubIssueTemplate& template, const std::vector<String>& variables) {
    String content = template.content;
    
    // Simple variable replacement
    for (size_t i = 0; i < variables.size(); i++) {
        String placeholder = "{{VAR" + String(i + 1) + "}}";
        content.replace(placeholder, variables[i]);
    }
    
    return content;
}

std::vector<String> GitHubApp::getAvailableMilestones(const String& owner, const String& repo) {
    std::vector<String> milestoneTitles;
    auto milestones = listMilestones(owner, repo);
    
    for (const auto& milestone : milestones) {
        if (milestone.state == "open") {
            milestoneTitles.push_back(milestone.title);
        }
    }
    
    return milestoneTitles;
}

std::vector<GitHubIssueTemplate> GitHubApp::listIssueTemplates(const String& owner, const String& repo) {
    std::vector<GitHubIssueTemplate> templates;
    
    // Note: GitHub's issue templates API is limited
    // This is a simplified implementation
    // In practice, you'd need to parse the .github/ISSUE_TEMPLATE directory
    
    // Common template names
    templates.push_back({"Bug Report", "Report a bug", "Describe the bug", "bug"});
    templates.push_back({"Feature Request", "Suggest a new feature", "Describe the feature", "enhancement"});
    templates.push_back({"Question", "Ask a question", "What's your question?", "question"});
    
    return templates;
}

String GitHubApp::getIssueTemplateContent(const String& owner, const String& repo, const String& templateName) {
    // This is a simplified implementation
    // Real implementation would fetch from GitHub API or local files
    
    if (templateName == "Bug Report") {
        return "## Bug Description\n\nDescribe the bug here.\n\n## Steps to Reproduce\n\n1. Go to '...'\n2. Click on '....'\n3. Scroll down to '....'\n4. See error\n\n## Expected Behavior\n\nDescribe what you expected to happen.\n\n## Screenshots\n\nIf applicable, add screenshots.\n\n## Environment\n\n- Device: [e.g. ESP32, M5Stack]\n- Firmware Version: [e.g. 1.0.0]";
    } else if (templateName == "Feature Request") {
        return "## Feature Description\n\nDescribe the feature you'd like to see.\n\n## Problem/Need\n\nWhat problem would this solve?\n\n## Proposed Solution\n\nDescribe your proposed solution.\n\n## Alternative Solutions\n\nDescribe any alternative solutions you've considered.";
    } else if (templateName == "Question") {
        return "## Question\n\nWhat's your question?\n\n## Context\n\nProvide additional context for your question.\n\n## What I've Tried\n\nDescribe what you've already tried.";
    }
    
    return "";
}

String GitHubApp::formatIssueBody(const String& title, const String& description, const String& template) {
    String body = "";
    
    if (template.length() > 0) {
        body = getIssueTemplateContent("", "", template);
        if (body.length() > 0) {
            body.replace("{{TITLE}}", title);
            body.replace("{{DESCRIPTION}}", description);
        }
    }
    
    if (body.length() == 0) {
        body = description;
    }
    
    return body;
}

bool GitHubApp::validateIssueData(const GitHubIssueCreate& issueData) {
    // Basic validation
    if (issueData.title.length() == 0 || issueData.title.length() > 256) {
        lastError = "Title must be between 1 and 256 characters";
        return false;
    }
    
    if (issueData.body.length() > 65536) {
        lastError = "Body cannot exceed 65536 characters";
        return false;
    }
    
    return true;
}

String GitHubApp::escapeJson(const String& str) {
    String escaped = "";
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

String GitHubApp::getFileContent(const String& owner, const String& repo, const String& path, const String& ref) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return "";
    }
    
    String params = "ref=" + ref;
    String url = buildUrl("/repos/" + owner + "/" + repo + "/contents/" + path, params);
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        // Extract content from base64 encoded response
        int contentStart = response.indexOf("\"content\":\"") + 11;
        int contentEnd = response.indexOf("\",\"sha\"", contentStart);
        
        if (contentStart > 10 && contentEnd > contentStart) {
            String encodedContent = response.substring(contentStart, contentEnd);
            http.end();
            return decodeBase64(encodedContent);
        }
    }
    
    http.end();
    return "";
}

bool GitHubApp::createFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& branch) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/contents/" + path);
    
    String data = "{";
    data += "\"message\":\"" + message + "\",";
    data += "\"content\":\"" + encodeBase64(content) + "\",";
    data += "\"branch\":\"" + branch + "\"";
    data += "}";
    
    bool success = makeRequest("PUT", url, data);
    http.end();
    return success;
}

std::vector<GitHubRepo> GitHubApp::searchRepositories(const String& query, int perPage) {
    std::vector<GitHubRepo> repos;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return repos;
    }
    
    String params = "q=" + urlencode(query) + "&per_page=" + String(perPage);
    String url = buildUrl("/search/repositories", params);
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        // Parse search results
        int itemsStart = response.indexOf("\"items\":[");
        if (itemsStart >= 0) {
            int itemsEnd = response.indexOf("]", itemsStart);
            if (itemsEnd > itemsStart) {
                String itemsArray = response.substring(itemsStart + 9, itemsEnd + 1);
                parseReposArray(itemsArray, repos);
            }
        }
    }
    
    http.end();
    return repos;
}

// Simple JSON parsing functions (minimal implementation)
bool GitHubApp::parseRepoFromJson(const String& json, GitHubRepo& repo) {
    repo.name = extractJsonValue(json, "\"name\"");
    repo.fullName = extractJsonValue(json, "\"full_name\"");
    repo.description = extractJsonValue(json, "\"description\"");
    repo.cloneUrl = extractJsonValue(json, "\"clone_url\"");
    repo.sshUrl = extractJsonValue(json, "\"ssh_url\"");
    repo.htmlUrl = extractJsonValue(json, "\"html_url\"");
    repo.isPrivate = (extractJsonValue(json, "\"private\"") == "true");
    repo.defaultBranch = extractJsonValue(json, "\"default_branch\"");
    repo.stars = extractJsonValue(json, "\"stargazers_count\"").toInt();
    repo.forks = extractJsonValue(json, "\"forks_count\"").toInt();
    return true;
}

bool GitHubApp::parseIssueFromJson(const String& json, GitHubIssue& issue) {
    issue.number = extractJsonValue(json, "\"number\"").toInt();
    issue.title = extractJsonValue(json, "\"title\"");
    issue.body = extractJsonValue(json, "\"body\"");
    issue.state = extractJsonValue(json, "\"state\"");
    
    // Get author from user object
    int userStart = json.indexOf("\"user\":{");
    if (userStart >= 0) {
        int userEnd = json.indexOf("}", userStart);
        String userJson = json.substring(userStart + 7, userEnd + 1);
        issue.author = extractJsonValue(userJson, "\"login\"");
    }
    
    issue.createdAt = extractJsonValue(json, "\"created_at\"");
    issue.updatedAt = extractJsonValue(json, "\"updated_at\"");
    issue.htmlUrl = extractJsonValue(json, "\"html_url\"");
    
    // Parse labels array
    int labelsStart = json.indexOf("\"labels\":[");
    if (labelsStart >= 0) {
        int labelsEnd = json.indexOf("]", labelsStart);
        if (labelsEnd > labelsStart) {
            String labelsArray = json.substring(labelsStart + 10, labelsEnd);
            // Simple parsing for label names
            int labelStart = 0;
            while (true) {
                int nameStart = labelsArray.indexOf("\"name\":\"", labelStart);
                if (nameStart < 0) break;
                nameStart += 9;
                int nameEnd = labelsArray.indexOf("\"", nameStart);
                if (nameEnd > nameStart) {
                    String labelName = labelsArray.substring(nameStart, nameEnd);
                    issue.labels.push_back(labelName);
                }
                labelStart = nameEnd + 1;
            }
        }
    }
    
    // Parse assignees array
    int assigneesStart = json.indexOf("\"assignees\":[");
    if (assigneesStart >= 0) {
        int assigneesEnd = json.indexOf("]", assigneesStart);
        if (assigneesEnd > assigneesStart) {
            String assigneesArray = json.substring(assigneesStart + 12, assigneesEnd);
            // Simple parsing for assignee logins
            int assigneeStart = 0;
            while (true) {
                int loginStart = assigneesArray.indexOf("\"login\":\"", assigneeStart);
                if (loginStart < 0) break;
                loginStart += 9;
                int loginEnd = assigneesArray.indexOf("\"", loginStart);
                if (loginEnd > loginStart) {
                    String assigneeLogin = assigneesArray.substring(loginStart, loginEnd);
                    issue.assignees.push_back(assigneeLogin);
                }
                assigneeStart = loginEnd + 1;
            }
        }
    }
    
    // Parse milestone
    int milestoneStart = json.indexOf("\"milestone\":{");
    if (milestoneStart >= 0) {
        int milestoneEnd = json.indexOf("}", milestoneStart);
        if (milestoneEnd > milestoneStart) {
            String milestoneJson = json.substring(milestoneStart + 12, milestoneEnd);
            issue.milestone = extractJsonValue(milestoneJson, "\"title\"");
        }
    }
    
    issue.comments = extractJsonValue(json, "\"comments\"").toInt();
    issue.isPullRequest = (extractJsonValue(json, "\"pull_request\"") != "");
    
    return true;
}

bool GitHubApp::parseUserFromJson(const String& json, GitHubUser& user) {
    user.login = extractJsonValue(json, "\"login\"");
    user.name = extractJsonValue(json, "\"name\"");
    user.email = extractJsonValue(json, "\"email\"");
    user.bio = extractJsonValue(json, "\"bio\"");
    user.avatarUrl = extractJsonValue(json, "\"avatar_url\"");
    user.htmlUrl = extractJsonValue(json, "\"html_url\"");
    user.publicRepos = extractJsonValue(json, "\"public_repos\"").toInt();
    user.followers = extractJsonValue(json, "\"followers\"").toInt();
    user.following = extractJsonValue(json, "\"following\"").toInt();
    return true;
}

bool GitHubApp::parseReposArray(const String& json, std::vector<GitHubRepo>& repos) {
    int start = 0;
    while (true) {
        int objStart = json.indexOf("{", start);
        if (objStart < 0) break;
        
        int objEnd = json.indexOf("}", objStart);
        if (objEnd < 0) break;
        
        String objJson = json.substring(objStart, objEnd + 1);
        GitHubRepo repo;
        if (parseRepoFromJson(objJson, repo)) {
            repos.push_back(repo);
        }
        
        start = objEnd + 1;
    }
    return repos.size() > 0;
}

bool GitHubApp::parseIssuesArray(const String& json, std::vector<GitHubIssue>& issues) {
    int start = 0;
    while (true) {
        int objStart = json.indexOf("{", start);
        if (objStart < 0) break;
        
        int objEnd = json.indexOf("}", objStart);
        if (objEnd < 0) break;
        
        String objJson = json.substring(objStart, objEnd + 1);
        GitHubIssue issue;
        if (parseIssueFromJson(objJson, issue)) {
            issues.push_back(issue);
        }
        
        start = objEnd + 1;
    }
    return issues.size() > 0;
}

bool GitHubApp::parseUsersArray(const String& json, std::vector<GitHubUser>& users) {
    int start = 0;
    while (true) {
        int objStart = json.indexOf("{", start);
        if (objStart < 0) break;
        
        int objEnd = json.indexOf("}", objStart);
        if (objEnd < 0) break;
        
        String objJson = json.substring(objStart, objEnd + 1);
        GitHubUser user;
        if (parseUserFromJson(objJson, user)) {
            users.push_back(user);
        }
        
        start = objEnd + 1;
    }
    return users.size() > 0;
}

String GitHubApp::extractJsonValue(const String& json, const String& key) {
    String pattern = key + ":\"";
    int start = json.indexOf(pattern);
    if (start < 0) {
        pattern = key + ":";
        start = json.indexOf(pattern);
        if (start < 0) return "";
        start += pattern.length();
        int end = json.indexOf(",", start);
        if (end < 0) end = json.indexOf("}", start);
        return json.substring(start, end);
    }
    
    start += pattern.length();
    int end = json.indexOf("\"", start);
    if (end < 0) return "";
    return json.substring(start, end);
}

// Utility functions for encoding/decoding
String GitHubApp::encodeBase64(const String& data) {
    // Simple base64 encoding for small strings
    const String base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String result = "";
    int i = 0;
    
    while (i < data.length()) {
        byte a = i < data.length() ? data.charAt(i++) : 0;
        byte b = i < data.length() ? data.charAt(i++) : 0;
        byte c = i < data.length() ? data.charAt(i++) : 0;
        
        byte triplet = (a << 16) | (b << 8) | c;
        
        result += base64Chars.charAt((triplet >> 18) & 0x3F);
        result += base64Chars.charAt((triplet >> 12) & 0x3F);
        result += (i > data.length() + 1) ? '=' : base64Chars.charAt((triplet >> 6) & 0x3F);
        result += (i > data.length()) ? '=' : base64Chars.charAt(triplet & 0x3F);
    }
    
    return result;
}

String GitHubApp::decodeBase64(const String& data) {
    // Simple base64 decoding
    const String base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String result = "";
    
    for (int i = 0; i < data.length(); i += 4) {
        byte a = base64Chars.indexOf(data.charAt(i));
        byte b = base64Chars.indexOf(data.charAt(i + 1));
        byte c = (i + 2 < data.length()) ? base64Chars.indexOf(data.charAt(i + 2)) : 0;
        byte d = (i + 3 < data.length()) ? base64Chars.indexOf(data.charAt(i + 3)) : 0;
        
        byte triplet = (a << 18) | (b << 12) | (c << 6) | d;
        
        result += (char)((triplet >> 16) & 0xFF);
        if (data.charAt(i + 2) != '=') {
            result += (char)((triplet >> 8) & 0xFF);
        }
        if (data.charAt(i + 3) != '=') {
            result += (char)(triplet & 0xFF);
        }
    }
    
    return result;
}

std::vector<GitHubUser> GitHubApp::listUserFollowers(const String& username) {
    std::vector<GitHubUser> users;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return users;
    }
    
    String user = username.length() > 0 ? username : config.username;
    String url = buildUrl("/users/" + user + "/followers", "per_page=100");
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        parseUsersArray(response, users);
    }
    
    http.end();
    return users;
}

std::vector<GitHubUser> GitHubApp::listUserFollowing(const String& username) {
    std::vector<GitHubUser> users;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return users;
    }
    
    String user = username.length() > 0 ? username : config.username;
    String url = buildUrl("/users/" + user + "/following", "per_page=100");
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        parseUsersArray(response, users);
    }
    
    http.end();
    return users;
}

String GitHubApp::createGist(const String& description, const String& filename, const String& content, bool isPublic) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return "";
    }
    
    String url = buildUrl("/gists");
    
    String data = "{";
    data += "\"description\":\"" + description + "\",";
    data += "\"public\":" + (isPublic ? "true" : "false") + ",";
    data += "\"files\":{\"" + filename + "\":{\"content\":\"" + content + "\"}}";
    data += "}";
    
    if (makeRequest("POST", url, data)) {
        String response = http.getString();
        String gistId = extractJsonValue(response, "\"id\"");
        http.end();
        return gistId;
    }
    
    http.end();
    return "";
}

bool GitHubApp::deleteGist(const String& gistId) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/gists/" + gistId);
    bool success = makeRequest("DELETE", url);
    http.end();
    return success;
}

bool GitHubApp::closeIssue(const String& owner, const String& repo, int issueNumber) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    
    String data = "{\"state\":\"closed\"}";
    
    bool success = makeRequest("PATCH", url, data);
    http.end();
    return success;
}

bool GitHubApp::deleteRepo(const String& owner, const String& repo) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo);
    bool success = makeRequest("DELETE", url);
    http.end();
    return success;
}

bool GitHubApp::updateFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& sha, const String& branch) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/contents/" + path);
    
    String data = "{";
    data += "\"message\":\"" + message + "\",";
    data += "\"content\":\"" + encodeBase64(content) + "\",";
    data += "\"sha\":\"" + sha + "\",";
    data += "\"branch\":\"" + branch + "\"";
    data += "}";
    
    bool success = makeRequest("PUT", url, data);
    http.end();
    return success;
}

bool GitHubApp::deleteFile(const String& owner, const String& repo, const String& path, const String& message, const String& sha, const String& branch) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/contents/" + path);
    
    String data = "{";
    data += "\"message\":\"" + message + "\",";
    data += "\"sha\":\"" + sha + "\"";
    if (branch.length() > 0) {
        data += ",\"branch\":\"" + branch + "\"";
    }
    data += "}";
    
    bool success = makeRequest("DELETE", url, data);
    http.end();
    return success;
}

std::vector<GitHubUser> GitHubApp::searchUsers(const String& query, int perPage) {
    std::vector<GitHubUser> users;
    
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return users;
    }
    
    String params = "q=" + urlencode(query) + "&per_page=" + String(perPage);
    String url = buildUrl("/search/users", params);
    
    if (makeRequest("GET", url)) {
        String response = http.getString();
        // Parse search results
        int itemsStart = response.indexOf("\"items\":[");
        if (itemsStart >= 0) {
            int itemsEnd = response.indexOf("]", itemsStart);
            if (itemsEnd > itemsStart) {
                String itemsArray = response.substring(itemsStart + 9, itemsEnd + 1);
                parseUsersArray(itemsArray, users);
            }
        }
    }
    
    http.end();
    return users;
}

bool GitHubApp::createWebhook(const String& owner, const String& repo, const String& url, const String& events) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String webhookUrl = buildUrl("/repos/" + owner + "/" + repo + "/hooks");
    
    String data = "{";
    data += "\"name\":\"web\",";
    data += "\"active\":true,";
    data += "\"events\":[" + events + "],";
    data += "\"config\":{\"url\":\"" + url + "\",\"content_type\":\"json\"}";
    data += "}";
    
    bool success = makeRequest("POST", webhookUrl, data);
    http.end();
    return success;
}

bool GitHubApp::deleteWebhook(const String& owner, const String& repo, const String& url) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    // First get the list of webhooks to find the ID
    String webhookUrl = buildUrl("/repos/" + owner + "/" + repo + "/hooks");
    
    if (makeRequest("GET", webhookUrl)) {
        String response = http.getString();
        
        // Find webhook by URL
        int start = 0;
        while (true) {
            int objStart = response.indexOf("{", start);
            if (objStart < 0) break;
            
            int objEnd = response.indexOf("}", objStart);
            if (objEnd < 0) break;
            
            String objJson = response.substring(objStart, objEnd + 1);
            String configUrl = extractJsonValue(objJson, "\"url\"");
            
            if (configUrl == url) {
                String hookId = extractJsonValue(objJson, "\"id\"");
                http.end();
                
                // Delete the webhook
                String deleteUrl = buildUrl("/repos/" + owner + "/" + repo + "/hooks/" + hookId);
                bool success = makeRequest("DELETE", deleteUrl);
                http.end();
                return success;
            }
            
            start = objEnd + 1;
        }
    }
    
    http.end();
    return false;
}

String GitHubApp::urlencode(const String& str) {
    String encoded = "";
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += '%' + String(c, HEX);
        }
    }
    return encoded;
}