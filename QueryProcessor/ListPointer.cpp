//
//  ListPointer.cpp
//  QueryProcessor
//
//  Created by Hieu Do on 10/31/18.
//  Copyright © 2018 Hieu Do. All rights reserved.
//

#include "ListPointer.hpp"
#include "VByteCompression.hpp"

ListPointer::ListPointer(const std::string& fn, size_t invLPos, size_t mtdSz) : currChunkIdx(0), currDidIdx(0) {
    indexIfs.open(fn, std::ifstream::binary);
    if (!indexIfs) {
        std::cerr << "Failed to open " << fn << '\n';
        exit(1);
    }
    // Read in metadata and decode
    std::vector<unsigned char> mtdBlock(mtdSz);
    indexIfs.seekg(invLPos);
    indexIfs.read((char *)&mtdBlock[0], mtdSz);
    chunkStartPos = invLPos + mtdSz;
    std::vector<size_t> mtdDecoded = decodeVB(mtdBlock);
    
//    std::cout << "mtdSz: " << mtdSz << '\n';
//    std::cout << "mtdDecoded:\n";
//    for (auto i : mtdDecoded)
//        std::cout << i << ' ';
//    std::cout << '\n';
    
    // Parse decoded metadata 
    size_t i = 0;
    int numChunks = (int)mtdDecoded[i++];
    while (numChunks--) {
        chunkSizes.push_back(mtdDecoded[i++]);
    }
    int numDids = (int)mtdDecoded[i++];
    while (numDids--) {
        lastDids.push_back(mtdDecoded[i++]);
    }
    
    if (lastDids.size()*2 != chunkSizes.size()) {
        std::cerr << "Internal error when constructing ListPointer\n";
        exit(1);
    }
    
//    std::cout << "chunkSizes:\n";
//    for (auto i : chunkSizes)
//        std::cout << i << ' ';
//    std::cout << "\n\n";
//    
//    std::cout << "lastDids:\n";
//    for (auto i : lastDids)
//        std::cout << i << ' ';
//    std::cout << "\n\n";
}

size_t ListPointer::nextGEQ(size_t k) {
    // Skip chunks
    size_t numSkipBytes = 0;
    size_t chunkIdxToRead = 0;
    while (chunkIdxToRead < lastDids.size() && lastDids[chunkIdxToRead] < k) {
        numSkipBytes += (chunkSizes[chunkIdxToRead*2] + chunkSizes[chunkIdxToRead*2 + 1]);
        chunkIdxToRead++;
    }
    // Return MAX_SIZE if there is no k in this inv list
    if (chunkIdxToRead == lastDids.size() && lastDids[chunkIdxToRead-1] < k) {
        return (size_t) - 1;
    }
    
    // Uncompress chunk
    if (currDids.empty() || chunkIdxToRead > currChunkIdx) {
        currChunkIdx = chunkIdxToRead;
        std::vector<unsigned char> didToRead(chunkSizes[currChunkIdx*2]);
        indexIfs.seekg(chunkStartPos + numSkipBytes);
        indexIfs.read((char *)&didToRead[0], chunkSizes[currChunkIdx*2]);
        currDids = decodeVB(didToRead);
        
        std::vector<unsigned char> freqToRead(chunkSizes[currChunkIdx*2 + 1]);
        indexIfs.seekg(chunkStartPos + numSkipBytes + chunkSizes[currChunkIdx*2]);
        indexIfs.read((char *)&freqToRead[0], chunkSizes[currChunkIdx*2 + 1]);
        currFreqs = decodeVB(freqToRead);
        
        currDidIdx = 0; // reset idx in currDids
    }
    
    // Find GEQ
    while (currDidIdx < currDids.size() && currDids[currDidIdx] < k) { currDidIdx++; }
    
    return currDids[currDidIdx];
}

size_t ListPointer::getFreq() const {
    return currFreqs[currDidIdx];
}

size_t ListPointer::getNumDid() const {
    return lastDids.size();
}

void ListPointer::closeList() {
    indexIfs.close();
}
