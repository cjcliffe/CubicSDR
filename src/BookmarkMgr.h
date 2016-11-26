#pragma once

#include <vector>
#include <set>
#include <wx/arrstr.h>

#include "DemodulatorInstance.h"

class DataNode;

class BookmarkEntry {
public:
    std::mutex busy_lock;

    std::string type;
    std::wstring label;
    
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
typedef std::map<std::string, BookmarkList > BookmarkMap;
typedef std::map<std::string, bool > BookmarkMapSorted;
typedef std::vector<std::string> BookmarkNames;

class BookmarkMgr {
public:
    void saveToFile(std::string bookmarkFn);
    void loadFromFile(std::string bookmarkFn);
    
    void addBookmark(std::string group, DemodulatorInstance *demod);
    void addBookmark(std::string group, BookmarkEntry *be);
    void removeBookmark(std::string group, BookmarkEntry *be);
    void removeBookmark(BookmarkEntry *be);
    void moveBookmark(BookmarkEntry *be, std::string group);
    
    void addGroup(std::string group);
    void removeGroup(std::string group);
    void renameGroup(std::string group, std::string ngroup);
    BookmarkList getBookmarks(std::string group);
    void getGroups(BookmarkNames &arr);
    void getGroups(wxArrayString &arr);
    
    void updateActiveList();
    void updateBookmarks();
    void updateBookmarks(std::string group);

    void addRecent(DemodulatorInstance *demod);
    void removeRecent(BookmarkEntry *be);
    BookmarkList getRecents();

    
protected:
    
    BookmarkEntry *demodToBookmarkEntry(DemodulatorInstance *demod);
    
    BookmarkMap bmData;
    BookmarkMapSorted bmDataSorted;
    BookmarkList recents;
    std::mutex busy_lock;
};
