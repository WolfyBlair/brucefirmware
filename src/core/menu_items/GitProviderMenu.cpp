#include "GitProviderMenu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "core/wifi/webInterface.h"
#include "modules/github/github_app.h"
#include <WiFi.h>

// Global static variables for current provider
static GitProvider* g_currentProvider = nullptr;
static GitProviderType g_currentProviderType = GitProviderType::GITHUB;
static String g_currentProviderName = "GitHub";

GitProviderMenu::GitProviderMenu() {
    currentProvider = nullptr;
    currentProviderType = GitProviderType::GITHUB;
    returnToMain = false;
}

GitProviderMenu::~GitProviderMenu() {
    if (currentProvider) {
        delete currentProvider;
    }
}

void GitProviderMenu::begin() {
    // Initialize with GitHub as default if no provider is set
    if (!g_currentProvider && githubApp.isAuthenticated()) {
        g_currentProviderType = GitProviderType::GITHUB;
        g_currentProviderName = "GitHub";
    }
}

void GitProviderMenu::show() {
    optionsMenu();
}

String GitProviderMenu::getCurrentProviderDisplay() {
    String display = GitProviderFactory::providerTypeToString(currentProviderType);
    if (currentProvider && currentProvider->isAuthenticated()) {
        GitUser user = currentProvider->getUserInfo();
        display += " (" + user.login + ")";
    }
    return display;
}

void GitProviderMenu::optionsMenu() {
    returnToMain = false;
    options.clear();
    
    String providerStatus = getCurrentProviderDisplay();
    
    options = {
        {"Select Provider", [this]() { selectProviderMenu(); }},
        {"Current: " + providerStatus, nullptr},
        {"Repository Ops", [this]() { repositoryOpsMenu(); }},
        {"Issue Ops", [this]() { issueOpsMenu(); }},
        {"User Info", [this]() { userInfoMenu(); }},
        {"Configuration", [this]() { configurationMenu(); }},
        {"Disconnect", [this]() { 
            if (currentProvider) {
                currentProvider->end();
                delete currentProvider;
                currentProvider = nullptr;
            }
            displayInfo("Disconnected from " + currentProviderName, true);
        }},
        {"Back", [this]() { returnToMain = true; }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Git Providers");
}

void GitProviderMenu::selectProviderMenu() {
    options.clear();
    options = {
        {"GitHub", [this]() {
            currentProviderType = GitProviderType::GITHUB;
            currentProviderName = "GitHub";
            displayInfo("GitHub selected. Using existing GitHub integration.", true);
        }},
        {"GitLab", [this]() {
            if (currentProvider) {
                delete currentProvider;
            }
            currentProvider = new GitLabProvider();
            currentProviderType = GitProviderType::GITLAB;
            currentProviderName = "GitLab";
            manageCurrentProvider();
        }},
        {"Gitee", [this]() {
            if (currentProvider) {
                delete currentProvider;
            }
            currentProvider = new GiteeProvider();
            currentProviderType = GitProviderType::GITEE;
            currentProviderName = "Gitee";
            manageCurrentProvider();
        }}
    };
    
    options.push_back({"Back", [this]() { returnToMain = true; }});
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Select Provider");
}

void GitProviderMenu::manageCurrentProvider() {
    if (!currentProvider) {
        displayInfo("Error: No provider selected", true);
        return;
    }
    
    options.clear();
    options = {
        {"Authenticate", [this]() { authMenu(); }},
        {"Token from File", [this]() { tokenFromFile(); }},
        { "Test Connection", [this]() {
            displayInfo("Testing connection...", true);
            if (currentProvider) {
                GitUser user = currentProvider->getUserInfo();
                if (user.login.length() > 0) {
                    displayInfo("Connected as: " + user.login, true);
                } else {
                    displayInfo("Connection failed: " + currentProvider->getLastError(), true);
                }
            }
        }}
    };
    
    options.push_back({"Back", [this]() { optionsMenu(); }});
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Manage " + currentProviderName);
}

void GitProviderMenu::authMenu() {
    if (!currentProvider) {
        displayInfo("Error: No provider selected", true);
        return;
    }
    
    options.clear();
    options = {
        {"Manual Token", [this]() {
            String token = password("Enter token for " + currentProviderName + ":");
            if (token.length() > 0) {
                displayInfo("Authenticating...", true);
                if (currentProvider->begin(token)) {
                    GitUser user = currentProvider->getUserInfo();
                    String successMsg = "Authenticated as: " + user.login;
                    displayInfo(successMsg, true);
                    setCurrentProvider(currentProviderType, currentProvider);
                } else {
                    displayInfo("Authentication failed: " + currentProvider->getLastError(), true);
                }
            }
        }}
    };
    
    options.push_back({"Back", [this]() { manageCurrentProvider(); }});
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Authenticate " + currentProviderName);
}

void GitProviderMenu::tokenFromFile() {
    if (!currentProvider) {
        displayInfo("Error: No provider selected", true);
        return;
    }
    
    String tokenFile = selectFile("Select token file:", {"txt", "cfg", "token"});
    if (tokenFile.length() > 0) {
        File file = SD.open(tokenFile);
        if (file) {
            String token = file.readString();
            token.trim();
            file.close();
            
            if (token.length() > 0) {
                displayInfo("Authenticating...", true);
                if (currentProvider->begin(token)) {
                    GitUser user = currentProvider->getUserInfo();
                    String successMsg = "Authenticated as: " + user.login;
                    displayInfo(successMsg, true);
                    setCurrentProvider(currentProviderType, currentProvider);
                } else {
                    displayInfo("Authentication failed: " + currentProvider->getLastError(), true);
                }
            } else {
                displayInfo("Empty token file", true);
            }
        } else {
            displayInfo("Cannot read token file", true);
        }
    }
}

void GitProviderMenu::repositoryOpsMenu() {
    if (!ensureAuthenticated()) {
        return;
    }
    
    options.clear();
    options = {
        {"List Repos", [this]() {
            displayInfo("Loading repositories...", true);
            auto repos = currentProvider->listUserRepos();
            
            if (repos.size() > 0) {
                String result = "Your " + currentProviderName + " repositories (" + String(repos.size()) + "):";
                for (size_t i = 0; i < repos.size() && i < 10; i++) {
                    result += "\n" + repos[i].name;
                    if (repos[i].isPrivate) result += " [PRIVATE]";
                    result += " (" + String(repos[i].stars) + "★)";
                }
                if (repos.size() > 10) {
                    result += "\n... and " + String(repos.size() - 10) + " more";
                }
                displayInfo(result, true);
            } else {
                displayInfo("No repositories found or error: " + currentProvider->getLastError(), true);
            }
        }},
        {"Get Repo Info", [this]() {
            String owner = keyboard("Repository owner:");
            if (owner.length() > 0) {
                String repo = keyboard("Repository name:");
                if (repo.length() > 0) {
                    displayInfo("Loading repository info...", true);
                    GitRepository repoInfo = currentProvider->getRepo(owner, repo);
                    
                    if (repoInfo.name.length() > 0) {
                        String info = repoInfo.name + "\n";
                        info += repoInfo.description + "\n";
                        info += "Stars: " + String(repoInfo.stars) + " | Forks: " + String(repoInfo.forks) + "\n";
                        info += "Default branch: " + repoInfo.defaultBranch + "\n";
                        info += "URL: " + repoInfo.htmlUrl;
                        displayInfo(info, true);
                    } else {
                        displayInfo("Repository not found: " + currentProvider->getLastError(), true);
                    }
                }
            }
        }}
    };
    
    options.push_back({"Search", [this]() {
        String query = keyboard("Search query:");
        if (query.length() > 0) {
            displayInfo("Searching...", true);
            auto repos = currentProvider->searchRepositories(query);
            if (repos.size() > 0) {
                String result = "Found " + String(repos.size()) + " repos:";
                for (const auto& repo : repos) {
                    result += "\n" + repo.name + " (" + String(repo.stars) + "★)";
                }
                displayInfo(result, true);
            } else {
                displayInfo("No repositories found", true);
            }
        }
    }});
    
    options.push_back({"Back", [this]() { optionsMenu(); }});
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Repository Ops - " + currentProviderName);
}

void GitProviderMenu::issueOpsMenu() {
    if (!ensureAuthenticated()) {
        return;
    }
    
    options.clear();
    options = {
        {"List Issues", [this]() {
            String owner = keyboard("Repository owner:");
            if (owner.length() > 0) {
                String repo = keyboard("Repository name:");
                if (repo.length() > 0) {
                    options.clear();
                    options = {
                        {"Open Issues", [this, owner, repo]() {
                            displayInfo("Loading open issues...", true);
                            auto issues = currentProvider->listIssues(owner, repo, "open");
                            displayIssuesList(issues, owner + "/" + repo);
                        }},
                        {"Closed Issues", [this, owner, repo]() {
                            displayInfo("Loading closed issues...", true);
                            auto issues = currentProvider->listIssues(owner, repo, "closed");
                            displayIssuesList(issues, owner + "/" + repo);
                        }},
                        {"Back", [this]() { issueOpsMenu(); }}
                    };
                    loopOptions(options, MENU_TYPE_SUBMENU, "List Issues - " + currentProviderName);
                }
            }
        }},
        {"Create Issue", [this]() {
            String owner = keyboard("Repository owner:");
            if (owner.length() > 0) {
                String repo = keyboard("Repository name:");
                if (repo.length() > 0) {
                    String title = keyboard("Issue title:");
                    if (title.length() > 0) {
                        String body = keyboard("Issue description (optional):");
                        
                        displayInfo("Creating issue...", true);
                        if (currentProvider->createIssue(owner, repo, title, body)) {
                            displayInfo("Issue created successfully!", true);
                        } else {
                            displayInfo("Failed to create issue: " + currentProvider->getLastError(), true);
                        }
                    }
                }
            }
        }}
    };
    
    options.push_back({"Back", [this]() { optionsMenu(); }});
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Issue Ops - " + currentProviderName);
}

void GitProviderMenu::userInfoMenu() {
    if (!ensureAuthenticated()) {
        return;
    }
    
    displayInfo("Loading user info...", true);
    GitUser user = currentProvider->getUserInfo();
    
    if (user.login.length() > 0) {
        String info = "User: " + user.login;
        if (user.name.length() > 0) {
            info += "\nName: " + user.name;
        }
        if (user.email.length() > 0) {
            info += "\nEmail: " + user.email;
        }
        if (user.bio.length() > 0) {
            info += "\nBio: " + user.bio;
        }
        info += "\nRepos: " + String(user.publicRepos);
        info += "\nFollowers: " + String(user.followers);
        info += "\nFollowing: " + String(user.following);
        info += "\nProvider: " + currentProviderName;
        
        displayInfo(info, true);
    } else {
        displayInfo("Failed to get user info: " + currentProvider->getLastError(), true);
    }
}

void GitProviderMenu::configurationMenu() {
    options.clear();
    options = {
        {"Set API URL", [this]() {
            if (!currentProvider) {
                displayInfo("No provider selected", true);
                return;
            }
            String customUrl = keyboard("API Base URL:", currentProvider->getApiBaseUrl());
            if (customUrl.length() > 0) {
                currentProvider->setApiBaseUrl(customUrl);
                displayInfo("API URL set to: " + customUrl, true);
            }
        }}
    };
    
    options.push_back({"Back", [this]() { optionsMenu(); }});
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Configuration - " + currentProviderName);
}

bool GitProviderMenu::ensureAuthenticated() {
    if (!currentProvider) {
        displayInfo("Error: No provider selected", true);
        return false;
    }
    
    if (!currentProvider->isAuthenticated()) {
        displayInfo("Not authenticated. Please authenticate first.", true);
        manageCurrentProvider();
        return false;
    }
    
    return true;
}

void GitProviderMenu::displayIssuesList(const std::vector<GitIssue>& issues, const String& repoInfo) {
    if (issues.size() > 0) {
        String result = "Issues in " + repoInfo + " (" + String(issues.size()) + "):";
        for (size_t i = 0; i < issues.size() && i < 10; i++) {
            result += "\n#" + String(issues[i].number) + ": " + issues[i].title + " [" + issues[i].state + "]";
            if (issues[i].labels.size() > 0) {
                result += " (" + String(issues[i].labels.size()) + " labels)";
            }
        }
        if (issues.size() > 10) {
            result += "\n... and " + String(issues.size() - 10) + " more";
        }
        displayInfo(result, true);
    } else {
        displayInfo("No issues found or error: " + currentProvider->getLastError(), true);
    }
}

// Static methods for global provider access
GitProvider* GitProviderMenu::getCurrentProvider() {
    return g_currentProvider;
}

GitProviderType GitProviderMenu::getCurrentProviderType() {
    return g_currentProviderType;
}

String GitProviderMenu::getCurrentProviderName() {
    return g_currentProviderName;
}

void GitProviderMenu::setCurrentProvider(GitProviderType type, GitProvider* provider) {
    if (g_currentProvider && g_currentProvider != provider) {
        delete g_currentProvider;
    }
    g_currentProviderType = type;
    g_currentProvider = provider;
    g_currentProviderName = GitProviderFactory::providerTypeToString(type);
}

// Helper function to display issues
void displayIssuesList(const std::vector<GitIssue>& issues, const String& repoInfo) {
    // Implementation would go here - similar to the one above but global
}