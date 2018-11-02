//
//  main.cpp
//  QueryProcessor
//
//  Created by Hieu Do on 10/29/18.
//  Copyright © 2018 Hieu Do. All rights reserved.
//

#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <fstream>
#include <math.h>

#include "ListPointer.hpp"

const size_t MAX_SIZE = (size_t) - 1;

struct LexiconEntry {
    LexiconEntry() {} // for std::map
    LexiconEntry(size_t p, size_t s) : invListPos(p), metadataSize(s) {}
    size_t invListPos;
    size_t metadataSize;
};

struct UrlEntry {
    UrlEntry(const std::string& u, size_t d) : url(u), documentLen(d) {}
    std::string url;
    size_t documentLen;
};

void loadUrlTable(std::vector<UrlEntry>& urlTable, const std::string& fn);
void loadLexicon(std::map<std::string, LexiconEntry>& lexicon, const std::string& fn);
std::vector<std::string> parseQuery(const std::string& aQuery);
float getAvgDocLen(const std::vector<UrlEntry>& urlTable);

int main(int argc, const char * argv[]) {
    if (argc != 4) {
        std::cerr << "Argument error: you need to provide pathToUrlTable, pathToLexicon, and pathToInvertedIndex\n";
        exit(1);
    }
    
    // Load url table and lexicon
    std::vector<UrlEntry> urlTable;
    std::chrono::steady_clock::time_point beginLoadTable = std::chrono::steady_clock::now();
    loadUrlTable(urlTable, argv[1]);
    std::chrono::steady_clock::time_point endLoadTable = std::chrono::steady_clock::now();
    std::cout << std::to_string(urlTable.size()) << " entries loaded to urlTable. Elapsed: " << std::chrono::duration_cast<std::chrono::seconds>(endLoadTable - beginLoadTable).count() << "s.\n";
    
    // For BM25
    const size_t N = urlTable.size();
    const float K_1 = 1.2;
    const float B = 0.75;
    const float D_AVG = getAvgDocLen(urlTable);
    
    std::map<std::string, LexiconEntry> lexicon;
    std::chrono::steady_clock::time_point beginLoadLexicon = std::chrono::steady_clock::now();
    loadLexicon(lexicon, argv[2]);
    std::chrono::steady_clock::time_point endLoadLexicon = std::chrono::steady_clock::now();
    std::cout << std::to_string(lexicon.size()) << " entries loaded to lexicon. Elapsed: " << std::chrono::duration_cast<std::chrono::seconds>(endLoadLexicon - beginLoadLexicon).count() << "s.\n";
    
    std::string indexFn = argv[3];
    
    // Main loop: prompt user for input
    std::string query;
    std::priority_queue<std::pair<float, size_t>> topDids;
    while(std::cout << "Please enter your query: " && std::getline(std::cin, query)) {
        std::cout << "\nSearching '" << query << "'...\n\n";
        
        // Parse query
        std::vector<std::string> terms = parseQuery(query);
        
        // DAAT processing
        std::vector<ListPointer> allLps;
        for(auto t : terms) {
            // TODO: sort by length of inv list
            allLps.push_back(ListPointer(indexFn, lexicon[t].invListPos, lexicon[t].metadataSize));
        }
        
        size_t did = 0;
        while (did < MAX_SIZE) {
            // Get next post from shortest list
            did = allLps[0].nextGEQ(did);
            
            if (did == (size_t) - 1) { break; }
            
            size_t d = 0;
            for (size_t i = 1; (i < allLps.size()) && ((d = allLps[i].nextGEQ(did)) == did); ++i) {}
            
            if (d > did) { did = d; } // docId is NOT in intersection
            else { // docId is in intersection
                // Get all frequencies
                for (size_t i = 0; i < allLps.size(); ++i) { }
                
                // Computer BM25 score
                
                float score = 0;
                for (size_t i = 0; i < allLps.size(); ++i) {
                    float numerLeft = N - allLps[i].getNumDid() + 0.5;
                    float denomLeft = allLps[i].getNumDid() + 0.5;
                    float numerRight = (K_1 + 1) * allLps[i].getFreq();
                    const float K = K_1 * ((1 - B) + B * urlTable[d].documentLen / D_AVG);
                    float denomRight = K + allLps[i].getFreq();
                    score += (log(numerLeft / denomLeft) * (numerRight/denomRight));
                }
                
//                while (top10did.size() >= 10) {
//                    top10did.pop();
//                }
                topDids.push(std::make_pair(score, d));
                
                // Increase to search for next post
                did++;
            }
        }
        for (size_t i = 0; i < allLps.size(); ++i) { allLps[i].closeList(); }
            
        // end of DAAT
        
        // Print out results
        std::cout << topDids.size() << " results found:\n";
        size_t i = 0;
        while (!topDids.empty()) {
            std::cout << i << ". " << urlTable[topDids.top().second].url << '\n';
            std::cout << "\tRelevance score: " << topDids.top().first << "\n\n";
            topDids.pop();
            i++;
        }
    }
    
    
    
    return 0;
}

std::vector<std::string> parseQuery(const std::string& aQuery) {
    std::vector<std::string> terms;
    std::string delim = " and ";

    std::vector<size_t> allPos;
    size_t pos = aQuery.find(delim, 0);
    while(pos != std::string::npos) {
        allPos.push_back(pos);
        pos = aQuery.find(delim, pos+1);
    }
    
    size_t i = 0;
    for(size_t p : allPos) {
        terms.push_back(aQuery.substr(i, p-i));
        i = p + delim.size();
    }
    terms.push_back(aQuery.substr(i));
    
    return terms;
}

void loadUrlTable(std::vector<UrlEntry>& urlTable, const std::string& fn) {
    std::ifstream tableIfs(fn);
    if (!tableIfs) {
        std::cerr << "Failed to open " << fn << '\n';
        exit(1);
    }
    
    size_t did;
    std::string url;
    size_t urlLen;
    while (tableIfs >> did >> url >> urlLen) {
        urlTable.push_back(UrlEntry(url, urlLen));
    }
}

void loadLexicon(std::map<std::string, LexiconEntry>& lexicon, const std::string& fn) {
    std::ifstream lexiconIfs(fn);
    if (!lexiconIfs) {
        std::cerr << "Failed to open " << fn << '\n';
        exit(1);
    }
    
    std::string term;
    size_t invIdxPos;
    size_t metadataLen;
    while (lexiconIfs >> term >> invIdxPos >> metadataLen) {
        lexicon[term] = LexiconEntry(invIdxPos, metadataLen);
    }
}

float getAvgDocLen(const std::vector<UrlEntry>& urlTable) {
    float avg = 0;
    for (auto e : urlTable)
        avg += e.documentLen;
    return (avg / urlTable.size());
}
