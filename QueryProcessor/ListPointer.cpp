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
    
    if (mtdDecoded.empty()) {
        std::cerr << "Metadata corrupted: empty mtdDecoded\n";
        exit(1);
    }
    
    // Extract <size of did block, size of freq block> and <last dids>
    size_t i = 0;
    int numChunks = (int)mtdDecoded[i++];
    while (numChunks--) {
        if (i >= mtdDecoded.size()) {
            std::cerr << "Metadata corrupted: Index out of range for mtdDecoded\n";
            exit(1);
        }
        chunkSizes.push_back(mtdDecoded[i++]);
    }
    int numDids = (int)mtdDecoded[i++];
    while (numDids--) {
        if (i >= mtdDecoded.size()) {
            std::cerr << "Metadata corrupted: Index out of range for mtdDecoded\n";
            exit(1);
        }
        lastDids.push_back(mtdDecoded[i++]);
    }
    
    // Sanity check
    if (lastDids.size()*2 != chunkSizes.size()) {
        std::cerr << "Metadata corrupted: chunkSizes doesn't have twice more items than lastDids\n";
        exit(1);
    }
}

size_t ListPointer::nextGEQ(size_t k) {
    // Skip chunks
    size_t numSkipBytes = 0;
    size_t chunkIdxToRead = 0;
    while (chunkIdxToRead < lastDids.size() && lastDids[chunkIdxToRead] < k) {
        numSkipBytes += (chunkSizes[chunkIdxToRead*2] + chunkSizes[chunkIdxToRead*2 + 1]);
        chunkIdxToRead++;
    }
    // Make sure chunkIdxToRead does not go out of bound
    if (chunkIdxToRead == lastDids.size()) {
        chunkIdxToRead--;
    }
    // Return MAX_SIZE if there is no k in this inv list
    if (lastDids[chunkIdxToRead] < k) {
        return (size_t) - 1;
    }
    
    // Uncompress chunk
    if (currDids.empty() || chunkIdxToRead > currChunkIdx) {
        if (currChunkIdx >= chunkSizes.size()) {
            std::cerr << "nextGEQ error: Reading out of bound chunkSizes\n";
            exit(1);
        }
        
        currChunkIdx = chunkIdxToRead;
        std::vector<unsigned char> didToRead(chunkSizes[currChunkIdx*2]);
        indexIfs.seekg(chunkStartPos + numSkipBytes);
        indexIfs.read((char *)&didToRead[0], chunkSizes[currChunkIdx*2]);
        currDids = decodeVB(didToRead);
        
        std::vector<unsigned char> freqToRead(chunkSizes[currChunkIdx*2 + 1]);
        indexIfs.seekg(chunkStartPos + numSkipBytes + chunkSizes[currChunkIdx*2]);
        indexIfs.read((char *)&freqToRead[0], chunkSizes[currChunkIdx*2 + 1]);
        currFreqs = decodeVB(freqToRead);
        
        if (currDids.size() != currFreqs.size() || currDids.size() > 128 || currFreqs.size() > 128) {
            std::cerr << "nextGEQ error: Each decoded chunk does not have the right amound of elements\n";
            exit(1);
        }
        
        currDidIdx = 0; // reset idx in currDids
    }
    
    // Find GEQ
    while (currDidIdx < currDids.size()) {
        if (currDids[currDidIdx] >= k) { break; }
        currDidIdx++;
    }
    
    if (currDidIdx >= currDids.size() || currDids[currDidIdx] < k) {
        std::cerr << "nextGEQ error: Chunk does not have GEQ did\n";
        exit(1);
    }
    
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
