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

int main(int argc, const char * argv[]) {
    if (argc != 4) {
        std::cerr << "Argument error: you need to provide pathToUrlTable, pathToLexicon, and pathToInvertedIndex\n";
        exit(1);
    }
    
    // Load url table and lexicon
    std::vector<UrlEntry> urlTable;
    loadUrlTable(urlTable, argv[1]);
    std::map<std::string, LexiconEntry> lexicon;
    loadLexicon(lexicon, argv[2]);
    
    std::string indexFn = argv[3];
    
    // Main loop: prompt user for input
    std::string query;
    std::priority_queue<std::pair<float, size_t>> top10did;
    while(std::cout << "Please enter your query: " && std::getline(std::cin, query)) {
        std::cout << "Looking up '" << query << "'...\n";
        // Parse query
        std::vector<std::string> terms = parseQuery(query);
        for (auto t : terms)
            std::cout << t << '\n';
        
        // DAAT
        std::vector<ListPointer> allLps;
        for(auto t : terms) {
            allLps.push_back(ListPointer(indexFn, lexicon[t].invListPos, lexicon[t].metadataSize));
        }
        
        size_t did = 0;
        while (did < MAX_SIZE) {
            // Get next post from shortest list
            did = allLps[0].nextGEQ(did);
            
            size_t d = 0;
            for (size_t i = 1; (i < allLps.size()) && ((d = allLps[i].nextGEQ(did)) == did); ++i) {}
            
            if (d > did) { did = d; } // docId is NOT in intersection
            else { // docId is in intersection
                // Get all frequencies
                for (size_t i = 0; i < allLps.size(); ++i) { }
                
                // Computer BM25 score
                
                // Increase to search for next post
                did++;
            }
        }
        for (size_t i = 0; i < allLps.size(); ++i) { allLps[i].closeList(); }
            
        // end of DAAT
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
    
}

void loadLexicon(std::map<std::string, LexiconEntry>& lexicon, const std::string& fn) {
    
}
