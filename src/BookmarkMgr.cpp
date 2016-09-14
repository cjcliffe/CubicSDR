#include "BookmarkMgr.h"


void BookmarkMgr::saveToFile(std::string bookmarkFn) {
    
    
}


void BookmarkMgr::loadFromFile(std::string bookmarkFn) {
    
    
}

void BookmarkMgr::addBookmark(std::string group, DemodulatorInstance *demod) {
    std::lock_guard < std::mutex > lock(busy_lock);

}

BookmarkList BookmarkMgr::getBookmarks(std::string group, std::string folder) {
    std::lock_guard < std::mutex > lock(busy_lock);

    BookmarkList results;
    
    
    
    return results;
}
