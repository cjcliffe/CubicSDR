#include "BookmarkMgr.h"
#include "CubicSDR.h"
#include "DataTree.h"

#define BOOKMARK_RECENTS_MAX 25


void BookmarkMgr::saveToFile(std::string bookmarkFn) {
    DataTree s("cubicsdr_bookmarks");
    DataNode *header = s.rootNode()->newChild("header");
    header->newChild("version")->element()->set(wxString(CUBICSDR_VERSION).ToStdWstring());
    
    DataNode *modems = s.rootNode()->newChild("modems");
    
    std::lock_guard < std::mutex > lockData(busy_lock);
    
    for (auto &bmd_i : bmData) {
        DataNode *group = modems->newChild("group");
        *group->newChild("@name") = bmd_i.first;

        for (auto &bm_i : bmd_i.second ) {
            std::lock_guard < std::mutex > lockEnt(bm_i->busy_lock);
            group->newChildCloneFrom("modem", bm_i->node);
        }
    }

    DataNode *recent_modems = s.rootNode()->newChild("recent_modems");
    
    for (auto demod : wxGetApp().getDemodMgr().getDemodulators()) {
        wxGetApp().getDemodMgr().saveInstance(recent_modems->newChild("modem"),demod);
    }

    for (auto &r_i : this->recents) {
        std::lock_guard < std::mutex > lockEnt(r_i->busy_lock);
        recent_modems->newChildCloneFrom("modem", r_i->node);
    }

    wxFileName saveFile(wxGetApp().getConfig()->getConfigDir(), bookmarkFn);
    wxFileName saveFileBackup(wxGetApp().getConfig()->getConfigDir(), bookmarkFn + ".backup");
    
    if (saveFile.IsDirWritable()) {
        // Hopefully leave at least a readable backup in case of failure..
        if (saveFile.FileExists() && (!saveFileBackup.FileExists() || saveFileBackup.IsFileWritable())) {
            wxCopyFile(saveFile.GetFullPath(wxPATH_NATIVE).ToStdString(), saveFileBackup.GetFullPath(wxPATH_NATIVE).ToStdString());
        }
        s.SaveToFileXML(saveFile.GetFullPath(wxPATH_NATIVE).ToStdString());
    }
}


void BookmarkMgr::loadFromFile(std::string bookmarkFn) {
    wxFileName loadFile(wxGetApp().getConfig()->getConfigDir(), bookmarkFn);

    DataTree s;

    if (!loadFile.IsFileReadable()) {
        return;
    }

    if (!s.LoadFromFileXML(loadFile.GetFullPath(wxPATH_NATIVE).ToStdString())) {
        // TODO: if exists; inform user & optionally load backup
        return;
    }
 
    if (s.rootNode()->hasAnother("modems")) {
        DataNode *modems = s.rootNode()->getNext("modems");
        while (modems->hasAnother("group")) {
            DataNode *group = modems->getNext("group");
            std::string groupName = "Unnamed";
            if (group->hasAnother("@name")) {
                groupName = group->getNext("@name")->element()->toString();
            }
            while (group->hasAnother("modem")) {
                DataNode *modem = group->getNext("modem");
                BookmarkEntry *be = nodeToBookmark("modem", modem);
                if (be) {
                    addBookmark(groupName.c_str(), be);
                } else {
                    std::cout << "error loading bookmarked modem.." << std::endl;
                }
            }
        }
    }
    
    if (s.rootNode()->hasAnother("recent_modems")) {
        DataNode *recent_modems = s.rootNode()->getNext("recent_modems");
        
        while (recent_modems->hasAnother("modem")) {
            DataNode *modem = recent_modems->getNext("modem");
            BookmarkEntry *be = nodeToBookmark("modem", modem);
            if (be) {
                addRecent(be);
            } else {
                std::cout << "error loading recent modem.." << std::endl;
            }
        }
    }
}

void BookmarkMgr::addBookmark(std::string group, DemodulatorInstance *demod) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    BookmarkEntry *be = demodToBookmarkEntry(demod);
    
    wxGetApp().getDemodMgr().saveInstance(be->node, demod);
    
    bmData[group].push_back(be);
    bmDataSorted[group] = false;
}

void BookmarkMgr::addBookmark(std::string group, BookmarkEntry *be) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    bmData[group].push_back(be);
    bmDataSorted[group] = false;
}


void BookmarkMgr::removeBookmark(std::string group, BookmarkEntry *be) {
    std::lock_guard < std::mutex > lockData(busy_lock);
    std::lock_guard < std::mutex > lockEnt(be->busy_lock);

    if (bmData.find(group) == bmData.end()) {
        return;
    }

    BookmarkList::iterator i = std::find(bmData[group].begin(), bmData[group].end(), be);
    
    if (i != bmData[group].end()) {
        delete *i;
        bmData[group].erase(i);
    }
}

void BookmarkMgr::removeBookmark(BookmarkEntry *be) {
    std::lock_guard < std::mutex > lockData(busy_lock);
    std::lock_guard < std::mutex > lockEnt(be->busy_lock);
    
    for (auto &bmd_i : bmData) {
        BookmarkList::iterator i = std::find(bmd_i.second.begin(), bmd_i.second.end(), be);
        if (i != bmd_i.second.end()) {
            bmd_i.second.erase(i);
        }
    }
}

void BookmarkMgr::moveBookmark(BookmarkEntry *be, std::string group) {
    std::lock_guard < std::mutex > lockData(busy_lock);
    std::lock_guard < std::mutex > lockEnt(be->busy_lock);
    
    for (auto &bmd_i : bmData) {
        BookmarkList::iterator i = std::find(bmd_i.second.begin(), bmd_i.second.end(), be);
        if (i != bmd_i.second.end()) {
            if (bmd_i.first == group) {
                return;
            }
            bmData[group].push_back(*i);
            bmd_i.second.erase(i);
            bmDataSorted[group] = false;
            bmDataSorted[bmd_i.first] = false;
            return;
        }
    }
}


void BookmarkMgr::addGroup(std::string group) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    if (bmData.find(group) == bmData.end()) {
        BookmarkList dummy = bmData[group];
    }
}

void BookmarkMgr::removeGroup(std::string group) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    BookmarkMap::iterator i = bmData.find(group);
    
    if (i != bmData.end()) {
        for (auto ii : bmData[group]) {
            delete ii;
        }
        bmData.erase(group);
    }
}

void BookmarkMgr::renameGroup(std::string group, std::string ngroup) {
    if (group == ngroup) {
        return;
    }
    
    std::lock_guard < std::mutex > lock(busy_lock);
    
    BookmarkMap::iterator i = bmData.find(group);
    BookmarkMap::iterator it = bmData.find(ngroup);
    
    if (i != bmData.end() && it != bmData.end()) {
        for (auto ii : bmData[group]) {
            bmData[ngroup].push_back(ii);
        }
        bmData.erase(group);
    } else if (i != bmData.end()) {
        bmData[ngroup] = bmData[group];
        bmData.erase(group);
    }
}

BookmarkList BookmarkMgr::getBookmarks(std::string group) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    if (bmData.find(group) == bmData.end()) {
        BookmarkList results;
        return results;
    }
    
    if (!bmDataSorted[group]) {
        std::sort(bmData[group].begin(), bmData[group].end(), BookmarkEntryCompare());
        bmDataSorted[group] = true;
    }
    
    return bmData[group];
}


void BookmarkMgr::getGroups(BookmarkNames &arr) {
    for (BookmarkMap::iterator i = bmData.begin(); i!= bmData.end(); ++i) {
        arr.push_back(i->first);
    }
}

void BookmarkMgr::getGroups(wxArrayString &arr) {
    for (BookmarkMap::iterator i = bmData.begin(); i!= bmData.end(); ++i) {
        arr.push_back(i->first);
    }
}


void BookmarkMgr::setExpandState(std::string groupName, bool state) {
    expandState[groupName] = state;
}


bool BookmarkMgr::getExpandState(std::string groupName) {
    if (expandState.find(groupName) == expandState.end()) {
        return true;
    }
    return expandState[groupName];
}


void BookmarkMgr::updateActiveList() {
    BookmarkView *bmv = wxGetApp().getAppFrame()->getBookmarkView();
    
    if (bmv) {
        bmv->updateActiveList();
    }
}

void BookmarkMgr::updateBookmarks() {
    BookmarkView *bmv = wxGetApp().getAppFrame()->getBookmarkView();
    
    if (bmv) {
        bmv->updateBookmarks();
    }
}

void BookmarkMgr::updateBookmarks(std::string group) {
    BookmarkView *bmv = wxGetApp().getAppFrame()->getBookmarkView();
    
    if (bmv) {
        bmv->updateBookmarks(group);
    }
}


void BookmarkMgr::addRecent(DemodulatorInstance *demod) {
    std::lock_guard < std::mutex > lock(busy_lock);
    recents.push_back(demodToBookmarkEntry(demod));

    trimRecents();
}

void BookmarkMgr::addRecent(BookmarkEntry *be) {
    std::lock_guard < std::mutex > lock(busy_lock);

    recents.push_back(be);

    trimRecents();
}



void BookmarkMgr::removeRecent(BookmarkEntry *be) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    BookmarkList::iterator bm_i = std::find(recents.begin(),recents.end(), be);
    
    if (bm_i != recents.end()) {
        recents.erase(bm_i);
    }
}


BookmarkList BookmarkMgr::getRecents() {
    return recents;
}


void BookmarkMgr::clearRecents() {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    recents.erase(recents.begin(),recents.end());
}


void BookmarkMgr::trimRecents() {
    if (recents.size() > BOOKMARK_RECENTS_MAX) {
        delete *(recents.begin());
        recents.erase(recents.begin(), recents.begin()+1);
    }
}

BookmarkEntry *BookmarkMgr::demodToBookmarkEntry(DemodulatorInstance *demod) {
    BookmarkEntry *be = new BookmarkEntry;
    
    be->bandwidth = demod->getBandwidth();
    be->type = demod->getDemodulatorType();
    be->label = demod->getDemodulatorUserLabel();
    be->frequency = demod->getFrequency();

    be->node = new DataNode;
    wxGetApp().getDemodMgr().saveInstance(be->node, demod);
    
    return be;
}

BookmarkEntry *BookmarkMgr::nodeToBookmark(const char *name_in, DataNode *node) {
    if (!node->hasAnother("frequency") || !node->hasAnother("type") || !node->hasAnother("bandwidth")) {
        return nullptr;
    }
    
    BookmarkEntry *be = new BookmarkEntry();
    node->getNext("frequency")->element()->get(be->frequency);
    node->getNext("type")->element()->get(be->type);
    node->getNext("bandwidth")->element()->get(be->bandwidth);

    if (node->hasAnother("user_label")) {
        node->getNext("user_label")->element()->get(be->label);
    }
    
    be->node = new DataNode("node",*node);
    
    return be;
}


std::wstring BookmarkMgr::getBookmarkEntryDisplayName(BookmarkEntry *bmEnt) {
    std::wstring dispName = bmEnt->label;
    
    if (dispName == "") {
        std::string freqStr = frequencyToStr(bmEnt->frequency) + " " + bmEnt->type;
        dispName = wstring(freqStr.begin(),freqStr.end());
    }
    
    return dispName;
}

std::wstring BookmarkMgr::getActiveDisplayName(DemodulatorInstance *demod) {
    std::wstring activeName = demod->getDemodulatorUserLabel();
    
    if (activeName == "") {
        std::string wstr = frequencyToStr(demod->getFrequency()) + " " + demod->getDemodulatorType();
        activeName = std::wstring(wstr.begin(),wstr.end());
    }
    
    return activeName;
}

