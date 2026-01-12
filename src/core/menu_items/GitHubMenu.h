#ifndef __GITHUB_MENU_H__
#define __GITHUB_MENU_H__

#include <MenuItemInterface.h>

class GitHubMenu : public MenuItemInterface {
public:
    GitHubMenu() : MenuItemInterface("GitHub") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    void drawIconImg() override;
    bool getTheme() override { return bruceConfig.theme.github; }

private:
    void configMenu(void);
    void authMenu(void);
    void startCaptivePortal(void);
    void authOAuthAP(void);
    void demoOAuth(void);
    void tokenFromFile(void);
    void repoMenu(void);
    void issueMenu(void);
    void listIssuesMenu(void);
    void createIssueMenu(void);
    void createAdvancedIssueMenu(void);
    void issueTemplatesMenu(void);
    void displayIssuesList(const std::vector<GitHubIssue>& issues, const String& repo);
    String selectFromList(const String& prompt, const std::vector<String>& options);
    void userMenu(void);
    void gistMenu(void);
    void fileMenu(void);
};

#endif