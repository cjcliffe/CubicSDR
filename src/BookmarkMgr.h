#pragma once

#include <vector>
#include <map>

#include "DataTree.h"
#include "DemodulatorInstance.h"

class BookmarkEntry {
public:
    std::mutex busy_lock;

    std::string folder;
    std::string type;
    std::string label;
    
    long long frequency;
    int bandwidth;
    
    DataNode *node;
};

typedef std::vector<BookmarkEntry *> BookmarkList;
typedef std::map<std::string, std::map<long long, BookmarkEntry *> > BookmarkMap;

class BookmarkMgr {
public:
    void saveToFile(std::string bookmarkFn);
    void loadFromFile(std::string bookmarkFn);
    
    void addBookmark(std::string group, DemodulatorInstance *demod);
    BookmarkList getBookmarks(std::string group, std::string folder = "");
    
protected:
    
    BookmarkMap bmData;
    std::mutex busy_lock;
};
