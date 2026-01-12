#ifndef GIT_PROVIDER_MENU_H
#define GIT_PROVIDER_MENU_H

#include "core/menu/Menu.h"
#include "modules/git/GitProvider.h"
#include "modules/git/GitProviderFactory.h"
#include <vector>
#include <memory>

class GitProviderMenu : public Menu {
private:
    GitProvider* currentProvider;
    GitProviderType currentProviderType;
    String currentProviderName;
    
    // Menu state
    bool returnToMain;
    
    // Provider management
    void selectProviderMenu();
    void manageCurrentProvider();
    
    // Provider-specific menus
    void repositoryOpsMenu();
    void issueOpsMenu();
    void userInfoMenu();
    void configurationMenu();
    
    // Authentication methods
    void authMenu();
    void tokenFromFile();
    void oauthFlow();
    void captivePortalAuth();
    
    // Utility methods
    String getCurrentProviderDisplay();
    bool ensureAuthenticated();
    void displayProviderInfo();
    
public:
    GitProviderMenu();
    ~GitProviderMenu();
    
    void begin();
    void show();
    
    // Static accessor for global provider
    static GitProvider* getCurrentProvider();
    static GitProviderType getCurrentProviderType();
    static String getCurrentProviderName();
    static void setCurrentProvider(GitProviderType type, GitProvider* provider);
};

#endif // GIT_PROVIDER_MENU_H