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
#include <iostream>
#include <fstream>

class ListPointer {
private:
    // Meta-data info
    std::vector<size_t> chunkSizes;
    std::vector<size_t> lastDids;
    size_t currChunkIdx;
    
    // Current chunk
    std::vector<size_t> currDids;
    std::vector<size_t> currFreqs;
    size_t currDidIdx;
    
    size_t chunkStartPos;
    
    std::ifstream indexIfs;
public:
    // Essentially, openList interface
    ListPointer(const std::string& fn, size_t invLPos, size_t mtdSz);
    
    size_t nextGEQ(size_t k);
    
    size_t getFreq() const;
    
    size_t getNumDid() const;
    
    void closeList();
};

#endif /* ListPointer_hpp */
