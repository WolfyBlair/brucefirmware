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

bool GitHubApp::createIssue(const String& owner, const String& repo, const String& title, const String& body) {
    if (!config.authenticated) {
        lastError = "Not authenticated";
        return false;
    }
    
    String url = buildUrl("/repos/" + owner + "/" + repo + "/issues");
    
    String data = "{";
    data += "\"title\":\"" + title + "\",";
    data += "\"body\":\"" + body + "\"";
    data += "}";
    
    bool success = makeRequest("POST", url, data);
    http.end();
    return success;
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