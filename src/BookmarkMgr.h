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
    BookmarkRangeEntry() : freq(0), startFreq(0), endFreq(0) {
        
    }
    BookmarkRangeEntry(std::wstring label, long long freq, long long startFreq, long long endFreq) : label(label), freq(freq), startFreq(startFreq), endFreq(endFreq) {
    }
     
    std::wstring label;
    
    long long freq;
    long long startFreq;
    long long endFreq;
};

typedef std::shared_ptr<BookmarkEntry> BookmarkEntryPtr;
typedef std::shared_ptr<BookmarkRangeEntry> BookmarkRangeEntryPtr;

typedef std::vector<BookmarkEntryPtr> BookmarkList;
typedef std::vector<BookmarkRangeEntryPtr> BookmarkRangeList;
typedef std::map<std::string, BookmarkList > BookmarkMap;
typedef std::map<std::string, bool > BookmarkMapSorted;
typedef std::vector<std::string> BookmarkNames;
typedef std::map<std::string, bool> BookmarkExpandState;

class BookmarkMgr {
public:
    BookmarkMgr();
    //if useFullpath = false, use the application config dir.
	//else assume bookmarkFn is a full path and use it for location.
    void saveToFile(const std::string& bookmarkFn, bool backup = true, bool useFullpath = false);
    bool loadFromFile(const std::string& bookmarkFn, bool backup = true, bool useFullpath = false);

	void resetBookmarks();

    bool hasLastLoad(const std::string& bookmarkFn);
    bool hasBackup(const std::string& bookmarkFn);

    void addBookmark(const std::string& group, const DemodulatorInstancePtr& demod);
    void addBookmark(const std::string& group, const BookmarkEntryPtr& be);
    void removeBookmark(const std::string& group, const BookmarkEntryPtr& be);
    void removeBookmark(const BookmarkEntryPtr& be);
    void moveBookmark(const BookmarkEntryPtr& be, const std::string& group);
    
    void addGroup(const std::string& group);
    void removeGroup(const std::string& group);
    void renameGroup(const std::string& group, const std::string& ngroup);
	//return an independent copy on purpose 
    BookmarkList getBookmarks(const std::string& group);

    void getGroups(BookmarkNames &arr);
    void getGroups(wxArrayString &arr);

    void setExpandState(const std::string& groupName, bool state);
    bool getExpandState(const std::string& groupName);

    void updateActiveList();
    void updateBookmarks();
    void updateBookmarks(const std::string& group);

    void addRecent(const DemodulatorInstancePtr& demod);
    void addRecent(const BookmarkEntryPtr& be);
    void removeRecent(const BookmarkEntryPtr& be);
    
	//return an independent copy on purpose 
	BookmarkList getRecents();

    void clearRecents();

	void removeActive(const DemodulatorInstancePtr& demod);

    void addRange(const BookmarkRangeEntryPtr& re);
    void removeRange(const BookmarkRangeEntryPtr& re);

	//return an independent copy on purpose 
	BookmarkRangeList getRanges();
    
	void clearRanges();

    static std::wstring getBookmarkEntryDisplayName(const BookmarkEntryPtr& bmEnt);
    static std::wstring getActiveDisplayName(const DemodulatorInstancePtr& demod);

    
protected:

    void trimRecents();
	void loadDefaultRanges();

    //utility method that attempts to decode the childNodeName as std::wstring, else as std::string, else 
    //return an empty string.
    static std::wstring getSafeWstringValue(DataNode* node, const std::string& childNodeName);
    
    BookmarkEntryPtr demodToBookmarkEntry(const DemodulatorInstancePtr& demod);
    BookmarkEntryPtr nodeToBookmark(DataNode *node);
    
    BookmarkMap bmData;
    BookmarkMapSorted bmDataSorted;
    BookmarkList recents;
    BookmarkRangeList ranges;
    bool rangesSorted;

    std::recursive_mutex busy_lock;
    
    BookmarkExpandState expandState;
};
