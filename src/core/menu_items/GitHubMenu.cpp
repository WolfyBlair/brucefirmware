#include "GitHubMenu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "core/wifi/webInterface.h"
#include "modules/github/github_app.h"
#include "modules/github/github_oauth.h"

void GitHubMenu::optionsMenu() {
    returnToMenu = false;
    options.clear();
    
    if (!githubApp.isAuthenticated()) {
        options = {
            {"Demo OAuth", [this]() { demoOAuth(); }},
            {"OAuth via AP", [this]() { authOAuthAP(); }},
            {"Manual Token", [this]() { authMenu(); }},
            {"Token from File", [this]() { tokenFromFile(); }},
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

void GitHubMenu::authOAuthAP() {
    // Check if OAuth is configured
    if (bruceConfig.githubClientId.length() == 0 || bruceConfig.githubClientSecret.length() == 0) {
        displayInfo("OAuth not configured.\nConfigure Client ID/Secret first.", true);
        return;
    }
    
    // Configure OAuth with saved credentials
    githubOAuth.setClientId(bruceConfig.githubClientId);
    githubOAuth.setClientSecret(bruceConfig.githubClientSecret);
    
    if (WiFi.getMode() != WIFI_MODE_STA && WiFi.getMode() != WIFI_MODE_APSTA) {
        displayInfo("WiFi connection required for OAuth", true);
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        displayInfo("Connecting to WiFi first...", true);
        if (!wifiConnectMenu()) {
            displayInfo("WiFi connection failed", true);
            return;
        }
    }
    
    // Start OAuth flow
    if (githubOAuth.startOAuthFlow(server)) {
        githubOAuth.setupOAuthRoutes(server);
        
        // Start access point for OAuth
        githubOAuth.startAccessPoint("Bruce-GitHub-Auth");
        
        // Show instructions
        String info = "OAuth Access Point Started!\n\n";
        info += "1. Connect to WiFi: Bruce-GitHub-Auth\n";
        info += "2. Open browser and go to:\n";
        info += "   172.0.0.1 or any website\n";
        info += "3. Click 'Authorize with GitHub'\n";
        info += "4. Complete GitHub authorization\n";
        info += "5. Return here when done\n\n";
        info += "Press any key to stop AP";
        
        displayInfo(info, true);
        
        // Wait for user input or OAuth completion
        while (!check(AnyKeyPress)) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            
            // Check if OAuth completed
            if (githubApp.isAuthenticated()) {
                githubOAuth.stopOAuthFlow();
                githubOAuth.stopAccessPoint();
                displayInfo("OAuth authentication successful!", true);
                return;
            }
        }
        
        // Stop OAuth flow if user pressed key
        githubOAuth.stopOAuthFlow();
        githubOAuth.stopAccessPoint();
        displayInfo("OAuth flow cancelled", true);
    } else {
        displayInfo("Failed to start OAuth flow", true);
    }
}

void GitHubMenu::demoOAuth() {
    // Demo OAuth that simulates the flow without requiring real GitHub app
    githubOAuth.startAccessPoint("Bruce-GitHub-Demo");
    
    String info = "Demo OAuth Access Point!\n\n";
    info += "This simulates OAuth flow\n";
    info += "1. Connect to: Bruce-GitHub-Demo\n";
    info += "2. Go to any website\n";
    info += "3. You'll see demo auth page\n";
    info += "4. Click authorize to simulate\n\n";
    info += "Press any key to stop demo";
    
    displayInfo(info, true);
    
    // Start demo web server on the access point
    AsyncWebServer demoServer(80);
    
    // Demo authentication page
    demoServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>GitHub OAuth Demo - Bruce ESP32</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f6f8fa; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 40px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #24292e; text-align: center; }
        .demo-btn { display: block; width: 100%; padding: 12px 16px; background: #0366d6; color: white; text-decoration: none; text-align: center; border-radius: 6px; font-weight: bold; margin: 20px 0; }
        .demo-btn:hover { background: #0256cc; }
        .info { background: #fff8dc; padding: 15px; border-radius: 6px; margin: 20px 0; border: 1px solid #f0e68c; }
        .logo { text-align: center; font-size: 48px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">🐙</div>
        <h1>GitHub OAuth Demo</h1>
        <div class="info">
            <h3>Demo Mode Active</h3>
            <p>This is a demonstration of the OAuth flow. In a real implementation, this would redirect to GitHub for authentication.</p>
            <p><strong>Demo Token:</strong> demo_token_12345</p>
        </div>
        <a href="/github/simulate-auth" class="demo-btn">Simulate Authorization</a>
        <div class="info">
            <strong>Note:</strong> This is a demo. Real OAuth requires GitHub app registration.
        </div>
    </div>
</body>
</html>
        )";
        request->send(200, "text/html", html);
    });
    
    // Simulate successful authorization
    demoServer.on("/github/simulate-auth", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Simulate storing a demo token
        bruceConfig.setGitHubToken("demo_token_12345");
        
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Demo Auth Successful</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f6f8fa; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 40px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }
        .success { color: #28a745; font-size: 48px; margin-bottom: 20px; }
    </style>
    <script>
        setTimeout(function() {
            window.close();
        }, 2000);
    </script>
</head>
<body>
    <div class="container">
        <div class="success">✓</div>
        <h1>Demo Authorization Successful!</h1>
        <p>Demo token has been stored on the device.</p>
        <p>This window will close automatically.</p>
    </div>
</body>
</html>
        )";
        request->send(200, "text/html", html);
    });
    
    demoServer.begin();
    
    // Wait for user to stop demo
    while (!check(AnyKeyPress)) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    demoServer.end();
    githubOAuth.stopAccessPoint();
    displayInfo("Demo OAuth stopped", true);
}

void GitHubMenu::authMenu() {
    options.clear();
    options = {
        {"Manual Token", [this]() {
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
        {"Back", [this]() { optionsMenu(); }}
    };
    
    loopOptions(options, MENU_TYPE_SUBMENU, "GitHub Auth");
}

void GitHubMenu::tokenFromFile() {
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
        {"Set Client ID", [this]() {
            String clientId = keyboard("GitHub OAuth Client ID:");
            if (clientId.length() > 0) {
                bruceConfig.setGitHubClientId(clientId);
                displayInfo("Client ID saved!", true);
            }
        }},
        {"Set Client Secret", [this]() {
            String clientSecret = password("GitHub OAuth Client Secret:");
            if (clientSecret.length() > 0) {
                bruceConfig.setGitHubClientSecret(clientSecret);
                displayInfo("Client Secret saved!", true);
            }
        }},
        {"Set Default Repo", [this]() {
            String defaultRepo = keyboard("Default repository (owner/repo):");
            if (defaultRepo.length() > 0) {
                bruceConfig.setGitHubDefaultRepo(defaultRepo);
                displayInfo("Default repository set!", true);
            }
        }},
        {"View Config", [this]() {
            String configInfo = "Default Repo: " + bruceConfig.githubDefaultRepo + "\n";
            configInfo += "Client ID: " + (bruceConfig.githubClientId.length() > 0 ? "Configured" : "Not set") + "\n";
            configInfo += "Client Secret: " + (bruceConfig.githubClientSecret.length() > 0 ? "Configured" : "Not set") + "\n";
            configInfo += "OAuth Enabled: " + String(bruceConfig.githubOAuthEnabled ? "Yes" : "No") + "\n";
            configInfo += "Authenticated: " + String(githubApp.isAuthenticated() ? "Yes" : "No");
            displayInfo(configInfo, true);
        }},
        {"Toggle OAuth", [this]() {
            bruceConfig.setGitHubOAuthEnabled(!bruceConfig.githubOAuthEnabled);
            displayInfo("OAuth " + String(bruceConfig.githubOAuthEnabled ? "enabled" : "disabled"), true);
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