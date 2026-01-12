#include "GitHubMenu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"
#include "modules/github/github_app.h"

void GitHubMenu::optionsMenu() {
    returnToMenu = false;
    options.clear();
    
    if (!githubApp.isAuthenticated()) {
        options = {
            {"Authenticate", [this]() { authMenu(); }},
            {"Back", [this]() { returnToMenu = true; }}
        };
    } else {
        options = {
            {"Repository Ops", [this]() { repoMenu(); }},
            {"Issue Ops", [this]() { issueMenu(); }},
            {"User Info", [this]() { userMenu(); }},
            {"Gist Ops", [this]() { gistMenu(); }},
            {"File Ops", [this]() { fileMenu(); }},
            {"Search", [this]() {
                String query = keyboard("Search query:");
                if (query.length() > 0) {
                    displayInfo("Searching...", true);
                    auto repos = githubApp.searchRepositories(query);
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
            }},
            {"Configure", [this]() { configMenu(); }},
            {"Disconnect", [this]() { 
                githubApp.end(); 
                displayInfo("Disconnected from GitHub", true); 
            }},
            {"Back", [this]() { returnToMenu = true; }}
        };
    }
    
    loopOptions(options, MENU_TYPE_SUBMENU, "GitHub");
}

void GitHubMenu::authMenu() {
    options.clear();
    options = {
        {"Token Setup", [this]() {
            String token = password("GitHub Personal Access Token:");
            if (token.length() > 0) {
                displayInfo("Authenticating...", true);
                if (githubApp.begin(token)) {
                    displayInfo("Authenticated as: " + githubApp.getUserInfo().login, true);
                } else {
                    displayInfo("Authentication failed: " + githubApp.getLastError(), true);
                }
            }
        }},
        {"Token from File", [this]() {
            String tokenFile = selectFile("Select token file:", {"txt"});
            if (tokenFile.length() > 0) {
                File file = SD.open(tokenFile);
                if (file) {
                    String token = file.readString();
                    token.trim();
                    file.close();
                    
                    if (token.length() > 0) {
                        displayInfo("Authenticating...", true);
                        if (githubApp.begin(token)) {
                            displayInfo("Authenticated as: " + githubApp.getUserInfo().login, true);
                        } else {
                            displayInfo("Authentication failed: " + githubApp.getLastError(), true);
                        }
                    } else {
                        displayInfo("Empty token file", true);
                    }
                } else {
                    displayInfo("Cannot read token file", true);
                }
            }
        }},
        {"Back", [this]() { optionsMenu(); }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "GitHub Auth");
}

void GitHubMenu::repoMenu() {
    options.clear();
    options = {
        {"List Repos", [this]() {
            displayInfo("Loading repositories...", true);
            auto repos = githubApp.listUserRepos();
            
            if (repos.size() > 0) {
                String result = "Your repositories (" + String(repos.size()) + "):";
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
                displayInfo("No repositories found or error: " + githubApp.getLastError(), true);
            }
        }},
        {"Create Repo", [this]() {
            String repoName = keyboard("Repository name:");
            if (repoName.length() > 0) {
                String description = keyboard("Description (optional):");
                bool isPrivate = confirmDialog("Private repository?", false);
                
                displayInfo("Creating repository...", true);
                if (githubApp.createRepo(repoName, description, isPrivate)) {
                    displayInfo("Repository created successfully!", true);
                } else {
                    displayInfo("Failed to create: " + githubApp.getLastError(), true);
                }
            }
        }},
        {"Get Repo Info", [this]() {
            String owner = keyboard("Repository owner:");
            if (owner.length() > 0) {
                String repo = keyboard("Repository name:");
                if (repo.length() > 0) {
                    displayInfo("Loading repository info...", true);
                    GitHubRepo repoInfo = githubApp.getRepo(owner, repo);
                    
                    if (repoInfo.name.length() > 0) {
                        String info = repoInfo.name + "\n";
                        info += repoInfo.description + "\n";
                        info += "Stars: " + String(repoInfo.stars) + " | Forks: " + String(repoInfo.forks) + "\n";
                        info += "Default branch: " + repoInfo.defaultBranch + "\n";
                        info += "URL: " + repoInfo.htmlUrl;
                        displayInfo(info, true);
                    } else {
                        displayInfo("Repository not found: " + githubApp.getLastError(), true);
                    }
                }
            }
        }},
        {"Back", [this]() { optionsMenu(); }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Repository Ops");
}

void GitHubMenu::issueMenu() {
    options.clear();
    options = {
        {"List Issues", [this]() {
            String owner = keyboard("Repository owner:");
            if (owner.length() > 0) {
                String repo = keyboard("Repository name:");
                if (repo.length() > 0) {
                    displayInfo("Loading issues...", true);
                    auto issues = githubApp.listIssues(owner, repo);
                    
                    if (issues.size() > 0) {
                        String result = "Issues in " + owner + "/" + repo + " (" + String(issues.size()) + "):";
                        for (size_t i = 0; i < issues.size() && i < 10; i++) {
                            result += "\n#" + String(issues[i].number) + " " + issues[i].title + " [" + issues[i].state + "]";
                        }
                        if (issues.size() > 10) {
                            result += "\n... and " + String(issues.size() - 10) + " more";
                        }
                        displayInfo(result, true);
                    } else {
                        displayInfo("No issues found", true);
                    }
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
                        if (githubApp.createIssue(owner, repo, title, body)) {
                            displayInfo("Issue created successfully!", true);
                        } else {
                            displayInfo("Failed to create issue: " + githubApp.getLastError(), true);
                        }
                    }
                }
            }
        }},
        {"Back", [this]() { optionsMenu(); }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Issue Ops");
}

void GitHubMenu::userMenu() {
    options.clear();
    options = {
        {"My Profile", [this]() {
            displayInfo("Loading profile...", true);
            GitHubUser user = githubApp.getUserInfo();
            
            if (user.login.length() > 0) {
                String info = user.login;
                if (user.name.length() > 0) info += " (" + user.name + ")";
                info += "\nRepos: " + String(user.publicRepos);
                info += " | Followers: " + String(user.followers);
                info += " | Following: " + String(user.following);
                if (user.bio.length() > 0) info += "\n" + user.bio;
                displayInfo(info, true);
            } else {
                displayInfo("Failed to load profile: " + githubApp.getLastError(), true);
            }
        }},
        {"User Search", [this]() {
            String query = keyboard("Username:");
            if (query.length() > 0) {
                displayInfo("Loading user info...", true);
                GitHubUser user = githubApp.getUserInfo(query);
                
                if (user.login.length() > 0) {
                    String info = user.login;
                    if (user.name.length() > 0) info += " (" + user.name + ")";
                    info += "\nRepos: " + String(user.publicRepos);
                    info += " | Followers: " + String(user.followers);
                    info += " | Following: " + String(user.following);
                    if (user.bio.length() > 0) info += "\n" + user.bio;
                    displayInfo(info, true);
                } else {
                    displayInfo("User not found: " + githubApp.getLastError(), true);
                }
            }
        }},
        {"Back", [this]() { optionsMenu(); }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "User Info");
}

void GitHubMenu::gistMenu() {
    options.clear();
    options = {
        {"Create Gist", [this]() {
            String description = keyboard("Gist description:");
            if (description.length() > 0) {
                String filename = keyboard("Filename:");
                if (filename.length() > 0) {
                    String content = keyboard("Content:");
                    if (content.length() > 0) {
                        bool isPublic = confirmDialog("Public gist?", false);
                        
                        displayInfo("Creating gist...", true);
                        String gistId = githubApp.createGist(description, filename, content, isPublic);
                        
                        if (gistId.length() > 0) {
                            displayInfo("Gist created! ID: " + gistId, true);
                        } else {
                            displayInfo("Failed to create gist: " + githubApp.getLastError(), true);
                        }
                    }
                }
            }
        }},
        {"Back", [this]() { optionsMenu(); }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Gist Ops");
}

void GitHubMenu::fileMenu() {
    options.clear();
    options = {
        {"View File", [this]() {
            String owner = keyboard("Repository owner:");
            if (owner.length() > 0) {
                String repo = keyboard("Repository name:");
                if (repo.length() > 0) {
                    String path = keyboard("File path (e.g., README.md):");
                    if (path.length() > 0) {
                        displayInfo("Loading file...", true);
                        String content = githubApp.getFileContent(owner, repo, path);
                        
                        if (content.length() > 0) {
                            displayInfo(content, true);
                        } else {
                            displayInfo("File not found or error: " + githubApp.getLastError(), true);
                        }
                    }
                }
            }
        }},
        {"Create File", [this]() {
            String owner = keyboard("Repository owner:");
            if (owner.length() > 0) {
                String repo = keyboard("Repository name:");
                if (repo.length() > 0) {
                    String path = keyboard("File path:");
                    if (path.length() > 0) {
                        String content = keyboard("File content:");
                        if (content.length() > 0) {
                            String message = keyboard("Commit message:");
                            if (message.length() > 0) {
                                displayInfo("Creating file...", true);
                                if (githubApp.createFile(owner, repo, path, content, message)) {
                                    displayInfo("File created successfully!", true);
                                } else {
                                    displayInfo("Failed to create file: " + githubApp.getLastError(), true);
                                }
                            }
                        }
                    }
                }
            }
        }},
        {"Back", [this]() { optionsMenu(); }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "File Ops");
}

void GitHubMenu::configMenu() {
    options.clear();
    options = {
        {"Set Default Repo", [this]() {
            String defaultRepo = keyboard("Default repository (owner/repo):");
            if (defaultRepo.length() > 0) {
                bruceConfig.setGitHubDefaultRepo(defaultRepo);
                displayInfo("Default repository set!", true);
            }
        }},
        {"View Config", [this]() {
            String configInfo = "Default Repo: " + bruceConfig.githubDefaultRepo + "\n";
            configInfo += "Enabled: " + String(bruceConfig.githubEnabled ? "Yes" : "No");
            configInfo += "\nAuthenticated: " + String(githubApp.isAuthenticated() ? "Yes" : "No");
            displayInfo(configInfo, true);
        }},
        {"Toggle Enable", [this]() {
            bruceConfig.setGitHubEnabled(!bruceConfig.githubEnabled);
            displayInfo("GitHub " + String(bruceConfig.githubEnabled ? "enabled" : "disabled"), true);
        }},
        {"Back", [this]() { optionsMenu(); }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "GitHub Config");
}

void GitHubMenu::drawIcon(float scale) {
    // GitHub icon drawing implementation
    int x = 10;
    int y = 10;
    int size = 20 * scale;
    
    // Draw simple GitHub icon (octocat-like shape)
    tft.drawRect(x, y, size, size, bruceConfig.priColor);
    tft.drawRect(x+2, y+2, size-4, size-4, bruceConfig.priColor);
    tft.drawPixel(x+size/2, y+size/2, bruceConfig.priColor);
}

void GitHubMenu::drawIconImg() {
    // Implementation for themed icon drawing
    // This would draw an image if available in theme
}