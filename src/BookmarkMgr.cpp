#include "BookmarkMgr.h"
#include "CubicSDR.h"
#include "DataTree.h"


void BookmarkMgr::saveToFile(std::string bookmarkFn) {
    
    
}


void BookmarkMgr::loadFromFile(std::string bookmarkFn) {
    
    
}

void BookmarkMgr::addBookmark(std::string group, DemodulatorInstance *demod, std::string folder) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    BookmarkEntry *be = new BookmarkEntry;
    
    be->bandwidth = demod->getBandwidth();
    be->type = demod->getDemodulatorType();
    be->label = demod->getLabel();
    be->frequency = demod->getFrequency();
    be->folder = folder;
    
    wxGetApp().getDemodMgr().saveInstance(be->node, demod);
    
    bmData[group].insert(be);
}

void BookmarkMgr::removeBookmark(std::string group, BookmarkEntry *be) {
    std::lock_guard < std::mutex > lockData(busy_lock);
    std::lock_guard < std::mutex > lockEnt(be->busy_lock);
    
    bmData[group].erase(be);
}


BookmarkList BookmarkMgr::getBookmarks(std::string group, std::string folder) {
    std::lock_guard < std::mutex > lock(busy_lock);

    BookmarkList results;
    
    if (folder != "") {
        for (auto be_i : bmData[group]) {
            results.push_back(be_i);
        }
    } else {
        for (auto be_i : bmData[group]) {
            if (be_i->folder != folder) {
                continue;
            }
            results.push_back(be_i);
        }
    }
    
    return results;
}

void BookmarkMgr::updateActiveList() {
    BookmarkView *bmv = wxGetApp().getAppFrame()->getBookmarkView();
    
    if (bmv) {
        bmv->updateActiveList();
    }
}