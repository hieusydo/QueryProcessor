//
//  ListPointer.hpp
//  QueryProcessor
//
//  Created by Hieu Do on 10/31/18.
//  Copyright © 2018 Hieu Do. All rights reserved.
//

#ifndef ListPointer_hpp
#define ListPointer_hpp

#include <string>
#include <vector>

class ListPointer {
private:
    // Meta-data info
    std::vector<size_t> chunkSizes;
    std::vector<size_t> lastDids;
    
    // Current chunk
    std::vector<size_t> didChunk;
    std::vector<size_t> freqChunk;
    size_t didIdx;
    size_t freqIdx;
    
    size_t chunkStartPos;
    
    std::string indexFn;
public:
    ListPointer(const std::string& fn, size_t invLPos, size_t mtdSz);
    
    size_t nextGEQ(size_t did);
    
    void closeList();
};

#endif /* ListPointer_hpp */
