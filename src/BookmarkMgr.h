#pragma once

#include <vector>
#include <set>

#include "DemodulatorInstance.h"

class DataNode;

class BookmarkEntry {
public:
    std::mutex busy_lock;

    std::string type;
    std::string label;
    
    long long frequency;
    int bandwidth;
    
    DataNode *node;
};

struct BookmarkEntryCompare : public std::binary_function<BookmarkEntry *,BookmarkEntry *,bool>
{
    bool operator()(const BookmarkEntry *a, BookmarkEntry *b) const
    {
        return a->frequency < b->frequency;
    }
};


typedef std::vector<BookmarkEntry *> BookmarkList;
typedef std::set<BookmarkEntry *, BookmarkEntryCompare> BookmarkGroup;
typedef std::map<std::string, BookmarkGroup > BookmarkMap;
typedef std::vector<std::string> BookmarkNames;

class BookmarkMgr {
public:
    void saveToFile(std::string bookmarkFn);
    void loadFromFile(std::string bookmarkFn);
    
    void addBookmark(std::string group, DemodulatorInstance *demod);
    void removeBookmark(std::string group, BookmarkEntry *be);
    
    BookmarkList getBookmarks(std::string group);
    
    BookmarkGroup getGroup(std::string group);
    BookmarkNames getGroups();
    
    void updateActiveList();

    void addRecent(DemodulatorInstance *demod);
    BookmarkList getRecents();

    
protected:
    
    BookmarkEntry *demodToBookmarkEntry(DemodulatorInstance *demod);
    
    BookmarkMap bmData;
    BookmarkList recents;
    std::mutex busy_lock;
};
