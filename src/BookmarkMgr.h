// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <wx/arrstr.h>

#include <vector>
#include <set>
#include <memory>

#include "DemodulatorInstance.h"


class DataNode;

class BookmarkEntry {
public:
    std::mutex busy_lock;

    std::string type;
	//maps on the Demod user label.
    std::wstring label; 
    
    long long frequency;
    int bandwidth;
    
    DataNode *node;
    
    virtual ~BookmarkEntry();
};


class BookmarkRangeEntry {
public:
    BookmarkRangeEntry() : label(L""), freq(0), startFreq(0), endFreq(0) {
        
    }
    BookmarkRangeEntry(std::wstring label, long long freq, long long startFreq, long long endFreq) : label(label), freq(freq), startFreq(startFreq), endFreq(endFreq) {
    }
    
    std::mutex busy_lock;
    
    std::wstring label;
    
    long long freq;
    long long startFreq;
    long long endFreq;
};

typedef std::shared_ptr<BookmarkEntry> BookmarkEntryPtr;
typedef std::shared_ptr<BookmarkRangeEntry> BookmarkRangeEntryPtr;

struct BookmarkEntryCompare : public std::binary_function<BookmarkEntryPtr,BookmarkEntryPtr,bool>
{
    bool operator()(const BookmarkEntryPtr a, BookmarkEntryPtr b) const
    {
        return a->frequency < b->frequency;
    }
};


struct BookmarkRangeEntryCompare : public std::binary_function<BookmarkRangeEntryPtr ,BookmarkRangeEntryPtr ,bool>
{
    bool operator()(const BookmarkRangeEntryPtr a, BookmarkRangeEntryPtr b) const
    {
        return a->freq < b->freq;
    }
};

typedef std::vector<BookmarkEntryPtr> BookmarkList;
typedef std::vector<BookmarkRangeEntryPtr> BookmarkRangeList;
typedef std::map<std::string, BookmarkList > BookmarkMap;
typedef std::map<std::string, bool > BookmarkMapSorted;
typedef std::vector<std::string> BookmarkNames;
typedef std::map<std::string, bool> BookmarkExpandState;

class BookmarkMgr {
public:
    BookmarkMgr();
    
    void saveToFile(std::string bookmarkFn, bool backup = true);
    bool loadFromFile(std::string bookmarkFn, bool backup = true);

    bool hasLastLoad(std::string bookmarkFn);
    bool hasBackup(std::string bookmarkFn);

    void addBookmark(std::string group, DemodulatorInstance *demod);
    void addBookmark(std::string group, BookmarkEntryPtr be);
    void removeBookmark(std::string group, BookmarkEntryPtr be);
    void removeBookmark(BookmarkEntryPtr be);
    void moveBookmark(BookmarkEntryPtr be, std::string group);
    
    void addGroup(std::string group);
    void removeGroup(std::string group);
    void renameGroup(std::string group, std::string ngroup);
    const BookmarkList& getBookmarks(std::string group);
    void getGroups(BookmarkNames &arr);
    void getGroups(wxArrayString &arr);

    void setExpandState(std::string groupName, bool state);
    bool getExpandState(std::string groupName);

    void updateActiveList();
    void updateBookmarks();
    void updateBookmarks(std::string group);

    void addRecent(DemodulatorInstance *demod);
    void addRecent(BookmarkEntryPtr be);
    void removeRecent(BookmarkEntryPtr be);
    const BookmarkList& getRecents();
    void clearRecents();

	void removeActive(DemodulatorInstance *demod);

    void addRange(BookmarkRangeEntryPtr re);
    void removeRange(BookmarkRangeEntryPtr re);
    const BookmarkRangeList& getRanges();
    void clearRanges();
	
    static std::wstring getBookmarkEntryDisplayName(BookmarkEntryPtr bmEnt);
    static std::wstring getActiveDisplayName(DemodulatorInstance *demod);

protected:

    void trimRecents();
    
    BookmarkEntryPtr demodToBookmarkEntry(DemodulatorInstance *demod);
    BookmarkEntryPtr nodeToBookmark(const char *name_in, DataNode *node);
    
    BookmarkMap bmData;
    BookmarkMapSorted bmDataSorted;
    BookmarkList recents;
    BookmarkRangeList ranges;
    bool rangesSorted;
    std::recursive_mutex busy_lock;
    
    BookmarkExpandState expandState;

	//represents an empty BookMarkList that is returned by reference by some functions.
	static const BookmarkList emptyResults;
};
