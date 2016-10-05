#include "BookmarkMgr.h"
#include "CubicSDR.h"
#include "DataTree.h"


void BookmarkMgr::saveToFile(std::string bookmarkFn) {
    
    
}


void BookmarkMgr::loadFromFile(std::string bookmarkFn) {
    
    
}


void BookmarkMgr::addBookmark(std::string group, DemodulatorInstance *demod) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    BookmarkEntry *be = demodToBookmarkEntry(demod);
    
    wxGetApp().getDemodMgr().saveInstance(be->node, demod);
    
    bmData[group].insert(be);
}

void BookmarkMgr::removeBookmark(std::string group, BookmarkEntry *be) {
    std::lock_guard < std::mutex > lockData(busy_lock);
    std::lock_guard < std::mutex > lockEnt(be->busy_lock);
    
    bmData[group].erase(be);
}


BookmarkList BookmarkMgr::getBookmarks(std::string group) {
    std::lock_guard < std::mutex > lock(busy_lock);

    BookmarkList results;
    
    for (auto be_i : bmData[group]) {
        results.push_back(be_i);
    }
    
    return results;
}

BookmarkGroup BookmarkMgr::getGroup(std::string group) {
    return bmData[group];
}

BookmarkNames BookmarkMgr::getGroups() {
    BookmarkNames results;
    for (BookmarkMap::iterator i = bmData.begin(); i!= bmData.end(); ++i) {
        results.push_back(i->first);
    }
    return results;
}


void BookmarkMgr::updateActiveList() {
    BookmarkView *bmv = wxGetApp().getAppFrame()->getBookmarkView();
    
    if (bmv) {
        bmv->updateActiveList();
    }
}

void BookmarkMgr::addRecent(DemodulatorInstance *demod) {
    std::lock_guard < std::mutex > lock(busy_lock);
    recents.push_back(demodToBookmarkEntry(demod));
    if (recents.size() > 10) {
        delete *(recents.begin());
        recents.erase(recents.begin(), recents.begin()+1);
    }
}


BookmarkList BookmarkMgr::getRecents() {
    return recents;
}


BookmarkEntry *BookmarkMgr::demodToBookmarkEntry(DemodulatorInstance *demod) {
    BookmarkEntry *be = new BookmarkEntry;
    
    be->bandwidth = demod->getBandwidth();
    be->type = demod->getDemodulatorType();
    be->label = demod->getLabel();
    be->frequency = demod->getFrequency();

    be->node = new DataNode;
    wxGetApp().getDemodMgr().saveInstance(be->node, demod);
    
    return be;
}
