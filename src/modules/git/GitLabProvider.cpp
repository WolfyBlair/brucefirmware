#include "GitLabProvider.h"

GitLabProvider::GitLabProvider() {
    config.authenticated = false;
    config.apiBaseUrl = GITLAB_API_BASE;
    config.providerName = "GitLab";
    lastError = "";
    responseCode = 0;
}

GitLabProvider::~GitLabProvider() {
    end();
}

bool GitLabProvider::begin(const String& token) {
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

bool GitLabProvider::isAuthenticated() {
    return config.authenticated;
}

void GitLabProvider::end() {
    config.authenticated = false;
    config.token = "";
    lastError = "";
    responseCode = 0;
}

String GitLabProvider::getProjectId(const String& owner, const String& repo) {
    // For GitLab, we need to get the project ID from the namespace/repo
    // First, try to verify the project exists and get its ID
    String url = buildUrl("/projects/" + urlEncode(owner + "/" + repo));
    
    if (!makeRequest("GET", url)) {
        return "";
    }
    
    // Parse the project ID from the response
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, http.getString());
    
    if (error) {
        lastError = "JSON parsing error";
        return "";
    }
    
    if (doc.containsKey("id")) {
        return String(doc["id"]);
    }
    
    return "";
}

std::vector<GitRepository> GitLabProvider::listUserRepos() {
    std::vector<GitRepository> repos;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return repos;
    }
    
    String url = buildUrl("/projects", "membership=true&per_page=100");
    
    if (!makeRequest("GET", url)) {
        return repos;
    }
    
    String response = http.getString();
    parseReposArray(response, repos);
    
    return repos;
}

GitRepository GitLabProvider::getRepo(const String& owner, const String& repo) {
    GitRepository repository;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return repository;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        lastError = "Project not found";
        return repository;
    }
    
    String url = buildUrl("/projects/" + projectId);
    
    if (!makeRequest("GET", url)) {
        return repository;
    }
    
    String response = http.getString();
    parseRepoFromJson(response, repository);
    
    return repository;
}

bool GitLabProvider::createRepo(const String& name, const String& description, bool isPrivate) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/projects");
    String data = "name=" + urlEncode(name);
    
    if (description.length() > 0) {
        data += "&description=" + urlEncode(description);
    }
    
    data += "&visibility=" + String(isPrivate ? "private" : "public");
    
    return makeRequest("POST", url, data);
}

bool GitLabProvider::deleteRepo(const String& owner, const String& repo) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId);
    return makeRequest("DELETE", url);
}

std::vector<GitIssue> GitLabProvider::listIssues(const String& owner, const String& repo, const String& state) {
    std::vector<GitIssue> issues;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return issues;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return issues;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues", "state=" + state + "&per_page=100");
    
    if (!makeRequest("GET", url)) {
        return issues;
    }
    
    String response = http.getString();
    parseIssuesArray(response, issues);
    
    return issues;
}

GitIssue GitLabProvider::getIssue(const String& owner, const String& repo, int issueNumber) {
    GitIssue issue;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return issue;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return issue;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues/" + String(issueNumber));
    
    if (!makeRequest("GET", url)) {
        return issue;
    }
    
    String response = http.getString();
    parseIssueFromJson(response, issue);
    
    return issue;
}

bool GitLabProvider::createIssue(const String& owner, const String& repo, const String& title, const String& body) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues");
    String data = "title=" + urlEncode(title);
    
    if (body.length() > 0) {
        data += "&description=" + urlEncode(body);
    }
    
    return makeRequest("POST", url, data);
}

bool GitLabProvider::createIssueEx(const String& owner, const String& repo, const GitIssueCreate& issueData) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues");
    String data = "title=" + urlEncode(issueData.title);
    
    if (issueData.body.length() > 0) {
        data += "&description=" + urlEncode(issueData.body);
    }
    
    if (issueData.labels.size() > 0) {
        data += "&labels=";
        for (size_t i = 0; i < issueData.labels.size(); i++) {
            if (i > 0) data += ",";
            data += urlEncode(issueData.labels[i]);
        }
    }
    
    return makeRequest("POST", url, data);
}

bool GitLabProvider::closeIssue(const String& owner, const String& repo, int issueNumber) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues/" + String(issueNumber));
    return makeRequest("PUT", url, "state_event=close");
}

bool GitLabProvider::reopenIssue(const String& owner, const String& repo, int issueNumber) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues/" + String(issueNumber));
    return makeRequest("PUT", url, "state_event=reopen");
}

bool GitLabProvider::updateIssue(const String& owner, const String& repo, int issueNumber, const GitIssueCreate& issueData) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues/" + String(issueNumber));
    String data = "";
    
    if (issueData.title.length() > 0) {
        data += "title=" + urlEncode(issueData.title);
    }
    
    if (issueData.body.length() > 0) {
        if (data.length() > 0) data += "&";
        data += "description=" + urlEncode(issueData.body);
    }
    
    return makeRequest("PUT", url, data);
}

std::vector<GitIssueComment> GitLabProvider::listIssueComments(const String& owner, const String& repo, int issueNumber) {
    std::vector<GitIssueComment> comments;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return comments;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return comments;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues/" + String(issueNumber) + "/notes", "per_page=100");
    
    if (!makeRequest("GET", url)) {
        return comments;
    }
    
    // GitLab returns all notes, we need to filter out system notes
    String response = http.getString();
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error && doc.is<JsonArray>()) {
        JsonArray array = doc.as<JsonArray>();
        for (JsonObject item : array) {
            if (!item["system"].as<bool>()) { // Skip system notes
                GitIssueComment comment;
                comment.id = item["id"];
                comment.body = item["body"].as<String>();
                comment.author = item["author"]["username"].as<String>();
                comment.createdAt = item["created_at"].as<String>();
                comment.updatedAt = item["updated_at"].as<String>();
                comment.htmlUrl = ""; // GitLab doesn't provide direct comment URL
                comments.push_back(comment);
            }
        }
    }
    
    return comments;
}

bool GitLabProvider::addIssueComment(const String& owner, const String& repo, int issueNumber, const String& comment) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues/" + String(issueNumber) + "/notes");
    return makeRequest("POST", url, "body=" + urlEncode(comment));
}

std::vector<GitLabel> GitLabProvider::listLabels(const String& owner, const String& repo) {
    std::vector<GitLabel> labels;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return labels;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return labels;
    }
    
    String url = buildUrl("/projects/" + projectId + "/labels", "per_page=100");
    
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

bool GitLabProvider::addLabelToIssue(const String& owner, const String& repo, int issueNumber, const String& label) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues/" + String(issueNumber));
    return makeRequest("PUT", url, "add_labels=" + urlEncode(label));
}

std::vector<String> GitLabProvider::getAvailableLabels(const String& owner, const String& repo) {
    std::vector<GitLabel> labels = listLabels(owner, repo);
    std::vector<String> labelNames;
    
    for (const auto& label : labels) {
        labelNames.push_back(label.name);
    }
    
    return labelNames;
}

std::vector<GitUser> GitLabProvider::listAssignees(const String& owner, const String& repo) {
    std::vector<GitUser> users;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return users;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return users;
    }
    
    String url = buildUrl("/projects/" + projectId + "/members", "per_page=100");
    
    if (!makeRequest("GET", url)) {
        return users;
    }
    
    String response = http.getString();
    parseUsersArray(response, users);
    
    return users;
}

bool GitLabProvider::addAssigneeToIssue(const String& owner, const String& repo, int issueNumber, const String& assignee) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/issues/" + String(issueNumber));
    return makeRequest("PUT", url, "assignee_ids=" + urlEncode(assignee));
}

std::vector<String> GitLabProvider::getAvailableAssignees(const String& owner, const String& repo) {
    std::vector<GitUser> users = listAssignees(owner, repo);
    std::vector<String> usernames;
    
    for (const auto& user : users) {
        usernames.push_back(user.login);
    }
    
    return usernames;
}

std::vector<GitMilestone> GitLabProvider::listMilestones(const String& owner, const String& repo) {
    std::vector<GitMilestone> milestones;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return milestones;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return milestones;
    }
    
    String url = buildUrl("/projects/" + projectId + "/milestones", "per_page=100");
    
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
            milestone.number = item["iid"];
            milestone.state = item["state"].as<String>();
            milestone.dueOn = item["due_date"].as<String>();
            milestones.push_back(milestone);
        }
    }
    
    return milestones;
}

std::vector<String> GitLabProvider::getAvailableMilestones(const String& owner, const String& repo) {
    std::vector<GitMilestone> milestones = listMilestones(owner, repo);
    std::vector<String> milestoneTitles;
    
    for (const auto& milestone : milestones) {
        milestoneTitles.push_back(milestone.title);
    }
    
    return milestoneTitles;
}

GitUser GitLabProvider::getUserInfo(const String& username) {
    GitUser user;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return user;
    }
    
    String url;
    if (username.length() > 0) {
        url = buildUrl("/users", "username=" + urlEncode(username));
    } else {
        url = buildUrl("/user");
    }
    
    if (!makeRequest("GET", url)) {
        return user;
    }
    
    String response = http.getString();
    
    if (username.length() > 0) {
        // For username search, GitLab returns an array
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error && doc.is<JsonArray>() && doc.size() > 0) {
            JsonObject userObj = doc[0];
            parseUserFromJson(userObj.as<String>(), user);
        }
    } else {
        // For current user, parse directly
        parseUserFromJson(response, user);
    }
    
    return user;
}

String GitLabProvider::getFileContent(const String& owner, const String& repo, const String& path, const String& ref) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return "";
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return "";
    }
    
    String encodedPath = urlEncode(path);
    String encodedRef = urlEncode(ref);
    String url = buildUrl("/projects/" + projectId + "/repository/files/" + encodedPath + "/raw", "ref=" + encodedRef);
    
    if (!makeRequest("GET", url)) {
        return "";
    }
    
    return http.getString();
}

bool GitLabProvider::createFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& branch) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/repository/files/" + urlEncode(path));
    String data = "branch=" + urlEncode(branch);
    data += "&content=" + urlEncode(content);
    data += "&commit_message=" + urlEncode(message);
    
    return makeRequest("POST", url, data);
}

bool GitLabProvider::updateFile(const String& owner, const String& repo, const String& path, const String& content, const String& message, const String& sha, const String& branch) {
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return false;
    }
    
    String projectId = getProjectId(owner, repo);
    if (projectId.length() == 0) {
        return false;
    }
    
    String url = buildUrl("/projects/" + projectId + "/repository/files/" + urlEncode(path));
    String data = "branch=" + urlEncode(branch);
    data += "&content=" + urlEncode(content);
    data += "&commit_message=" + urlEncode(message);
    
    return makeRequest("PUT", url, data);
}

std::vector<GitRepository> GitLabProvider::searchRepositories(const String& query, int perPage) {
    std::vector<GitRepository> repos;
    
    if (!isAuthenticated()) {
        lastError = "Not authenticated";
        return repos;
    }
    
    String url = buildUrl("/projects", "search=" + urlEncode(query) + "&per_page=" + String(perPage));
    
    if (!makeRequest("GET", url)) {
        return repos;
    }
    
    String response = http.getString();
    parseReposArray(response, repos);
    
    return repos;
}

void GitLabProvider::setApiBaseUrl(const String& url) {
    config.apiBaseUrl = url;
    if (!config.apiBaseUrl.endsWith("/")) {
        config.apiBaseUrl += "/";
    }
}

bool GitLabProvider::makeRequest(const String& method, const String& url, const String& data) {
    http.begin(url);
    http.setUserAgent(GITLAB_USER_AGENT);
    http.setTimeout(10000);
    
    // Set authentication header
    setAuthHeader(config.token);
    
    // Set content type for POST/PUT requests
    if (method == "POST" || method == "PUT") {
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
    } else if (method == "DELETE") {
        httpCode = http.sendRequest("DELETE", data);
    } else {
        lastError = "Unsupported HTTP method";
        http.end();
        return false;
    }
    
    responseCode = httpCode;
    bool success = (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED);
    
    if (!success) {
        lastError = "HTTP " + String(httpCode) + ": " + http.getString();
    }
    
    http.end();
    return success;
}

String GitLabProvider::buildUrl(const String& endpoint, const String& params) {
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

void GitLabProvider::setAuthHeader(const String& token) {
    http.addHeader("PRIVATE-TOKEN", token);
}

void GitLabProvider::clearAuthHeader() {
    // Nothing to do here as we don't use a global header variable
}

// URL encoding helper (member function)
String GitLabProvider::urlEncode(const String& str) {
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

bool GitLabProvider::parseRepoFromJson(const String& json, GitRepository& repo) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    repo.name = doc["name"].as<String>();
    repo.fullName = doc["path_with_namespace"].as<String>();
    repo.description = doc["description"].as<String>();
    repo.cloneUrl = doc["http_url_to_repo"].as<String>();
    repo.sshUrl = doc["ssh_url_to_repo"].as<String>();
    repo.htmlUrl = doc["web_url"].as<String>();
    repo.isPrivate = doc["visibility"] != "public";
    repo.defaultBranch = doc["default_branch"].as<String>();
    repo.stars = doc["star_count"];
    repo.forks = doc["forks_count"];
    
    return true;
}

bool GitLabProvider::parseIssueFromJson(const String& json, GitIssue& issue) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    issue.number = doc["iid"];
    issue.title = doc["title"].as<String>();
    issue.body = doc["description"].as<String>();
    issue.state = doc["state"].as<String>();
    issue.author = doc["author"]["username"].as<String>();
    issue.createdAt = doc["created_at"].as<String>();
    issue.updatedAt = doc["updated_at"].as<String>();
    
    if (doc.containsKey("web_url")) {
        issue.htmlUrl = doc["web_url"].as<String>();
    }
    
    // Parse labels
    if (doc.containsKey("labels")) {
        JsonArray labelArray = doc["labels"];
        for (JsonVariant label : labelArray) {
            issue.labels.push_back(label.as<String>());
        }
    }
    
    // Parse assignees
    if (doc.containsKey("assignees")) {
        JsonArray assigneeArray = doc["assignees"];
        for (JsonObject assignee : assigneeArray) {
            issue.assignees.push_back(assignee["username"].as<String>());
        }
    }
    
    if (doc.containsKey("milestone")) {
        issue.milestone = doc["milestone"]["title"].as<String>();
    }
    
    issue.comments = doc["user_notes_count"];
    issue.isPullRequest = doc.containsKey("merge_request_iid");
    
    return true;
}

bool GitLabProvider::parseUserFromJson(const String& json, GitUser& user) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        return false;
    }
    
    user.login = doc["username"].as<String>();
    user.name = doc["name"].as<String>();
    user.email = doc["email"].as<String>();
    user.bio = doc["bio"].as<String>();
    user.avatarUrl = doc["avatar_url"].as<String>();
    user.htmlUrl = doc["web_url"].as<String>();
    user.publicRepos = doc["public_repos"];
    user.followers = doc["followers"];
    user.following = doc["following"];
    
    return true;
}

bool GitLabProvider::parseReposArray(const String& json, std::vector<GitRepository>& repos) {
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

bool GitLabProvider::parseIssuesArray(const String& json, std::vector<GitIssue>& issues) {
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

bool GitLabProvider::parseUsersArray(const String& json, std::vector<GitUser>& users) {
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