#pragma once

#include <vector>
#include <set>

#include "DemodulatorInstance.h"

class DataNode;

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

struct BookmarkEntryCompare : public std::binary_function<BookmarkEntry *,BookmarkEntry *,bool>
{
    bool operator()(const BookmarkEntry *a, BookmarkEntry *b) const
    {
        return a->frequency < b->frequency;
    }
};


typedef std::vector<BookmarkEntry *> BookmarkList;
typedef std::map<std::string, std::set<BookmarkEntry *, BookmarkEntryCompare> > BookmarkMap;

class BookmarkMgr {
public:
    void saveToFile(std::string bookmarkFn);
    void loadFromFile(std::string bookmarkFn);
    
    void addBookmark(std::string group, DemodulatorInstance *demod, std::string folder = "");
    void removeBookmark(std::string group, BookmarkEntry *be);
    BookmarkList getBookmarks(std::string group, std::string folder = "");
    
    void updateActiveList();
    
protected:
    
    BookmarkMap bmData;
    std::mutex busy_lock;
};
