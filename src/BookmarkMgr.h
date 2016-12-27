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
    std::wstring userLabel;
    
    long long frequency;
    int bandwidth;
    
    DataNode *node;
};


class BookmarkRangeEntry {
public:
    std::mutex busy_lock;
    
    std::wstring label;
    
    long long startFreq;
    long long endFreq;
};


struct BookmarkEntryCompare : public std::binary_function<BookmarkEntry *,BookmarkEntry *,bool>
{
    bool operator()(const BookmarkEntry *a, BookmarkEntry *b) const
    {
        return a->frequency < b->frequency;
    }
};


struct BookmarkRangeEntryCompare : public std::binary_function<BookmarkRangeEntry *,BookmarkRangeEntry *,bool>
{
    bool operator()(const BookmarkRangeEntry *a, BookmarkRangeEntry *b) const
    {
        return a->startFreq < b->startFreq;
    }
};

typedef std::vector<BookmarkEntry *> BookmarkList;
typedef std::vector<BookmarkRangeEntry *> BookmarkRangeList;
typedef std::map<std::string, BookmarkList > BookmarkMap;
typedef std::map<std::string, bool > BookmarkMapSorted;
typedef std::vector<std::string> BookmarkNames;
typedef std::map<std::string, bool> BookmarkExpandState;

class BookmarkMgr {
public:
    BookmarkMgr();
    
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

    void setExpandState(std::string groupName, bool state);
    bool getExpandState(std::string groupName);

    void updateActiveList();
    void updateBookmarks();
    void updateBookmarks(std::string group);

    void addRecent(DemodulatorInstance *demod);
    void addRecent(BookmarkEntry *be);
    void removeRecent(BookmarkEntry *be);
    BookmarkList getRecents();
    void clearRecents();

    void addRange(BookmarkRangeEntry *re);
    void removeRange(BookmarkRangeEntry *re);
    BookmarkRangeList getRanges();
    void clearRanges();
    

    static std::wstring getBookmarkEntryDisplayName(BookmarkEntry *bmEnt);
    static std::wstring getActiveDisplayName(DemodulatorInstance *demod);

protected:

    void trimRecents();
    
    BookmarkEntry *demodToBookmarkEntry(DemodulatorInstance *demod);    
    BookmarkEntry *nodeToBookmark(const char *name_in, DataNode *node);
    
    BookmarkMap bmData;
    BookmarkMapSorted bmDataSorted;
    BookmarkList recents;
    BookmarkRangeList ranges;
    bool rangesSorted;
    std::mutex busy_lock;
    
    BookmarkExpandState expandState;
};
