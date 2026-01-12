#include "GiteeProvider.h"

GiteeProvider::GiteeProvider() {
    config.authenticated = false;
    config.apiBaseUrl = GITEE_API_BASE;
    config.providerName = "Gitee";
    lastError = "";
    responseCode = 0;
}

GiteeProvider::~GiteeProvider() {
    end();
}

bool GiteeProvider::begin(const String& token) {
    if (token.length() > 0) {
        config.token = token;
    }
    
    if (config.token.length() == 0) {
        lastError = "No token provided";
        return false;
    }
    
    config.authenticated = true;
    return true;
}

bool GiteeProvider::isAuthenticated() {
    return config.authenticated;
}

void GiteeProvider::end() {
    config.authenticated = false;
    config.token = "";
    lastError = "";
    responseCode = 0;
}

std::vector<GitRepository> GiteeProvider::listUserRepos() {
    std::vector<GitRepository> repos;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return repos;
    }
    
    String url = buildUrl("/user/repos", "sort=updated&per_page=100");
    
    if (!makeRequest("GET", url)) {
        return repos;
    }
    
    String response = http.getString();
    parseReposArray(response, repos);
    
    return repos;
}

GitRepository GiteeProvider::getRepo(const String& owner, const String& repo) {
    GitRepository repository;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return repository;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo);
    
    if (!makeRequest("GET", url)) {
        return repository;
    }
    
    String response = http.getString();
    parseRepoFromJson(response, repository);
    
    return repository;
}

bool GiteeProvider::createRepo(const String& name, const String& description, bool isPrivate) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/user/repos");
    String data = "name=" + urlEncode(name);
    
    if (description.length() > 0) {
        data += "&description=" + urlEncode(description);
    }
    
    data += "&private=" + String(isPrivate ? "true" : "false");
    
    return makeRequest("POST", url, data);
}

bool GiteeProvider::deleteRepo(const String& owner, const String& repo) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo);
    return makeRequest("DELETE", url);
}

std::vector<GitIssue> GiteeProvider::listIssues(const String& owner, const String& repo, const String& state) {
    std::vector<GitIssue> issues;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return issues;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues", "state=" + state + "&per_page=100");
    
    if (!makeRequest("GET", url)) {
        return issues;
    }
    
    String response = http.getString();
    parseIssuesArray(response, issues);
    
    return issues;
}

GitIssue GiteeProvider::getIssue(const String& owner, const String& repo, int issueNumber) {
    GitIssue issue;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return issue;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    
    if (!makeRequest("GET", url)) {
        return issue;
    }
    
    String response = http.getString();
    parseIssueFromJson(response, issue);
    
    return issue;
}

bool GiteeProvider::createIssue(const String& owner, const String& repo, const String& title, const String& body) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues");
    String data = "title=" + urlEncode(title);
    
    if (body.length() > 0) {
        data += "&body=" + urlEncode(body);
    }
    
    return makeRequest("POST", url, data);
}

bool GiteeProvider::createIssueEx(const String& owner, const String& repo, const GitIssueCreate& issueData) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues");
    String data = "title=" + urlEncode(issueData.title);
    
    if (issueData.body.length() > 0) {
        data += "&body=" + urlEncode(issueData.body);
    }
    
    if (issueData.labels.size() > 0) {
        data += "&labels=";
        for (size_t i = 0; i < issueData.labels.size(); i++) {
            if (i > 0) data += ",";
            data += urlEncode(issueData.labels[i]);
        }
    }
    
    if (issueData.assignees.size() > 0) {
        data += "&assignees=";
        for (size_t i = 0; i < issueData.assignees.size(); i++) {
            if (i > 0) data += ",";
            data += urlEncode(issueData.assignees[i]);
        }
    }
    
    return makeRequest("POST", url, data);
}

bool GiteeProvider::closeIssue(const String& owner, const String& repo, int issueNumber) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    return makeRequest("PATCH", url, "state=closed");
}

bool GiteeProvider::reopenIssue(const String& owner, const String& repo, int issueNumber) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    return makeRequest("PATCH", url, "state=open");
}

bool GiteeProvider::updateIssue(const String& owner, const String& repo, int issueNumber, const GitIssueCreate& issueData) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    String data = "";
    
    if (issueData.title.length() > 0) {
        data += "title=" + urlEncode(issueData.title);
    }
    
    if (issueData.body.length() > 0) {
        if (data.length() > 0) data += "&";
        data += "body=" + urlEncode(issueData.body);
    }
    
    return makeRequest("PATCH", url, data);
}

std::vector<GitIssueComment> GiteeProvider::listIssueComments(const String& owner, const String& repo, int issueNumber) {
    std::vector<GitIssueComment> comments;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return comments;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber) + "/comments", "per_page=100");
    
    if (!makeRequest("GET", url)) {
        return comments;
    }
    
    String response = http.getString();
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc.is<JsonArray>()) {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject item : array) {
            GitIssueComment comment;
            comment.id = item["id"];
            comment.body = item["body"].as<String>();
            comment.author = item["user"]["login"].as<String>();
            comment.createdAt = item["created_at"].as<String>();
            comment.updatedAt = item["updated_at"].as<String>();
            comment.htmlUrl = item["html_url"].as<String>();
            comments.push_back(comment);
        }
    }
    
    return comments;
}

bool GiteeProvider::addIssueComment(const String& owner, const String& repo, int issueNumber, const String& comment) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber) + "/comments");
    return makeRequest("POST", url, "body=" + urlEncode(comment));
}

std::vector<GitLabel> GiteeProvider::listLabels(const String& owner, const String& repo) {
    std::vector<GitLabel> labels;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return labels;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/labels", "per_page=100");
    
    if (!makeRequest("GET", url)) {
        return labels;
    }
    
    String response = http.getString();
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc.is<JsonArray>()) {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject item : array) {
            GitLabel label;
            label.name = item["name"].as<String>();
            label.color = item["color"].as<String>();
            label.description = item["description"].as<String>();
            labels.push_back(label);
        }
    }
    
    return labels;
}

bool GiteeProvider::addLabelToIssue(const String& owner, const String& repo, int issueNumber, const String& label) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    return makeRequest("PATCH", url, "labels=" + urlEncode(label));
}

std::vector<String> GiteeProvider::getAvailableLabels(const String& owner, const String& repo) {
    std::vector<GitLabel> labels = listLabels(owner, repo);
    std::vector<String> labelNames;
    
    for (const auto& label : labels) {
        labelNames.push_back(label.name);
    }
    
    return labelNames;
}

std::vector<GitUser> GiteeProvider::listAssignees(const String& owner, const String& repo) {
    std::vector<GitUser> users;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return users;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/assignees", "per_page=100");
    
    if (!makeRequest("GET", url)) {
        return users;
    }
    
    String response = http.getString();
    parseUsersArray(response, users);
    
    return users;
}

bool GiteeProvider::addAssigneeToIssue(const String& owner, const String& repo, int issueNumber, const String& assignee) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues/" + String(issueNumber));
    return makeRequest("PATCH", url, "assignees=" + urlEncode(assignee));
}

std::vector<String> GiteeProvider::getAvailableAssignees(const String& owner, const String& repo) {
    std::vector<GitUser> users = listAssignees(owner, repo);
    std::vector<String> usernames;
    
    for (const auto& user : users) {
        usernames.push_back(user.login);
    }
    
    return usernames;
}

std::vector<GitMilestone> GiteeProvider::listMilestones(const String& owner, const String& repo) {
    std::vector<GitMilestone> milestones;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return milestones;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/milestones", "state=all&per_page=100");
    
    if (!makeRequest("GET", url)) {
        return milestones;
    }
    
    String response = http.getString();
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc.is<JsonArray>()) {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject item : array) {
            GitMilestone milestone;
            milestone.title = item["title"].as<String>();
            milestone.description = item["description"].as<String>();
            milestone.number = item["number"];
            milestone.state = item["state"].as<String>();
            milestone.dueOn = item["due_on"].as<String>();
            milestones.push_back(milestone);
        }
    }
    
    return milestones;
}

std::vector<String> GiteeProvider::getAvailableMilestones(const String& owner, const String& repo) {
    std::vector<GitMilestone> milestones = listMilestones(owner, repo);
    std::vector<String> milestoneTitles;
    
    for (const auto& milestone : milestones) {
        milestoneTitles.push_back(milestone.title);
    }
    
    return milestoneTitles;
}

GitUser GiteeProvider::getUserInfo(const String& username) {
    GitUser user;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return user;
    }
    
    String url;
    if (username.length() > 0) {
        url = buildUrl("/users/" + username);
    } else {
        url = buildUrl("/user");
    }
    
    if (!makeRequest("GET", url)) {
        return user;
    }
    
    String response = http.getString();
    parseUserFromJson(response, user);
    
    return user;
}

String GiteeProvider::getFileContent(const String& owner, const String& repo, const String& path, const String& ref) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return "";
    }
    
    String encodedPath = urlEncode(path);
    String encodedRef = urlEncode(ref);
    String url = buildUrl("/repos/" + owner + "/" + repo + "/contents/" + encodedPath, "ref=" + encodedRef);
    
    if (!makeRequest("GET", url)) {
        return "";
    }
    
    String response = http.getString();
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
        if (doc.containsKey("content")) {
            String content = doc["content"].as<String>();
            // Gitee returns base64 encoded content
            // In a real implementation, you'd need to decode base64
            return "<Base64 content - decode needed>";
        }
    }
    
    return "";
}

bool GiteeProvider::createFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& branch) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/contents/" + urlEncode(path));
    String data = "message=" + urlEncode(message);
    data += "&content=" + urlEncode(content); // In real implementation, this should be base64 encoded
    data += "&branch=" + urlEncode(branch);
    
    return makeRequest("POST", url, data);
}

bool GiteeProvider::updateFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& sha, const String& branch) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/contents/" + urlEncode(path));
    String data = "message=" + urlEncode(message);
    data += "&content=" + urlEncode(content); // In real implementation, this should be base64 encoded
    data += "&sha=" + urlEncode(sha);
    data += "&branch=" + urlEncode(branch);
    
    return makeRequest("PUT", url, data);
}

std::vector<GitRepository> GiteeProvider::searchRepositories(const String& query, int perPage) {
    std::vector<GitRepository> repos;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return repos;
    }
    
    String url = buildUrl("/search/repositories", "q=" + urlEncode(query) + "&per_page=" + String(perPage));
    
    if (!makeRequest("GET", url)) {
        return repos;
    }
    
    String response = http.getString();
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc.containsKey("items")) {
        JsonArray items = doc["items"];
        for (JsonObject item : items) {
            GitRepository repo;
            if (parseRepoFromJson(item.as<String>(), repo)) {
                repos.push_back(repo);
            }
        }
    }
    
    return repos;
}

void GiteeProvider::setApiBaseUrl(const String& url) {
    config.apiBaseUrl = url;
    if (!config.apiBaseUrl.endsWith("/")) {
        config.apiBaseUrl += "/";
    }
}

bool GiteeProvider::makeRequest(const String& method, const String& url, const String& data) {
    http.begin(url);
    http.setUserAgent(GITEE_USER_AGENT);
    http.setTimeout(10000);
    
    // Set authentication header
    setAuthHeader(config.token);
    
    // Set content type for POST/PUT/PATCH requests
    if (method == "POST" || method == "PUT" || method == "PATCH") {
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    }
    
    // Make the request
    int httpCode;
    if (method == "GET") {
        httpCode = http.GET();
    } else if (method == "POST") {
        httpCode = http.POST(data);
    } else if (method == "PUT") {
        httpCode = http.PUT(data);
    } else if (method == "PATCH") {
        httpCode = http.sendRequest("PATCH", data);
    } else if (method == "DELETE") {
        httpCode = http.sendRequest("DELETE", data);
    } else {
        lastError = "Unsupported HTTP method";
        http.end();
        return false;
    }
    
    responseCode = httpCode;
    bool success = (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_NO_CONTENT);
    
    if (!success) {
        lastError = "HTTP " + String(httpCode) + ": " + http.getString();
    }
    
    http.end();
    return success;
}

String GiteeProvider::buildUrl(const String& endpoint, const String& params) {
    String url = config.apiBaseUrl;
    
    // Remove trailing slash if present in API base
    if (url.endsWith("/") && endpoint.startsWith("/")) {
        url = url.substring(0, url.length() - 1);
    }
    
    url += endpoint;
    
    if (params.length() > 0) {
        url += "?" + params;
    }
    
    return url;
}

void GiteeProvider::setAuthHeader(const String& token) {
    http.addHeader("Authorization", "token " + token);
}

void GiteeProvider::clearAuthHeader() {
    // Nothing to do here as we don't use a global header variable
}

String GiteeProvider::urlEncode(const String& str) {
    String encoded = "";
    const char* cstr = str.c_str();
    
    for (int i = 0; i < str.length(); i++) {
        char c = cstr[i];
        
        if ((c >= '0' && c <= '9') || 
            (c >= 'A' && c <= 'Z') || 
            (c >= 'a' && c <= 'z') || 
            c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += "%" + String(c, HEX);
        }
    }
    
    return encoded;
}

bool GiteeProvider::parseRepoFromJson(const String& json, GitRepository& repo) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    repo.name = doc["name"].as<String>();
    repo.fullName = doc["full_name"].as<String>();
    repo.description = doc["description"].as<String>();
    repo.cloneUrl = doc["clone_url"].as<String>();
    repo.sshUrl = doc["ssh_url"].as<String>();
    repo.htmlUrl = doc["html_url"].as<String>();
    repo.isPrivate = doc["private"];
    repo.defaultBranch = doc["default_branch"].as<String>();
    repo.stars = doc["stargazers_count"];
    repo.forks = doc["forks_count"];
    
    return true;
}

bool GiteeProvider::parseIssueFromJson(const String& json, GitIssue& issue) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    issue.number = doc["number"];
    issue.title = doc["title"].as<String>();
    issue.body = doc["body"].as<String>();
    issue.state = doc["state"].as<String>();
    issue.author = doc["user"]["login"].as<String>();
    issue.createdAt = doc["created_at"].as<String>();
    issue.updatedAt = doc["updated_at"].as<String>();
    issue.htmlUrl = doc["html_url"].as<String>();
    
    // Parse labels
    if (doc.containsKey("labels")) {
        JsonArray labelArray = doc["labels"];
        for (JsonVariant label : labelArray) {
            issue.labels.push_back(label["name"].as<String>());
        }
    }
    
    // Parse assignees
    if (doc.containsKey("assignees")) {
        JsonArray assigneeArray = doc["assignees"];
        for (JsonObject assignee : assigneeArray) {
            issue.assignees.push_back(assignee["login"].as<String>());
        }
    }
    
    if (doc.containsKey("milestone")) {
        issue.milestone = doc["milestone"]["title"].as<String>();
    }
    
    issue.comments = doc["comments"];
    issue.isPullRequest = doc.containsKey("pull_request");
    
    return true;
}

bool GiteeProvider::parseUserFromJson(const String& json, GitUser& user) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    user.login = doc["login"].as<String>();
    user.name = doc["name"].as<String>();
    user.email = doc["email"].as<String>();
    user.bio = doc["bio"].as<String>();
    user.avatarUrl = doc["avatar_url"].as<String>();
    user.htmlUrl = doc["html_url"].as<String>();
    user.publicRepos = doc["public_repos"];
    user.followers = doc["followers"];
    user.following = doc["following"];
    
    return true;
}

bool GiteeProvider::parseReposArray(const String& json, std::vector<GitRepository>& repos) {
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    if (doc.is<JsonArray>()) {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject item : array) {
            GitRepository repo;
            if (parseRepoFromJson(item.as<String>(), repo)) {
                repos.push_back(repo);
            }
        }
    }
    
    return true;
}

bool GiteeProvider::parseIssuesArray(const String& json, std::vector<GitIssue>& issues) {
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    if (doc.is<JsonArray>()) {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject item : array) {
            GitIssue issue;
            if (parseIssueFromJson(item.as<String>(), issue)) {
                issues.push_back(issue);
            }
        }
    }
    
    return true;
}

bool GiteeProvider::parseUsersArray(const String& json, std::vector<GitUser>& users) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    if (doc.is<JsonArray>()) {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject item : array) {
            GitUser user;
            if (parseUserFromJson(item.as<String>(), user)) {
                users.push_back(user);
            }
        }
    }
    
    return true;
}