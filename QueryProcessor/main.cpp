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
#include <locale>
#include <algorithm>

#include "ListPointer.hpp"
#include "DocumentStore.hpp"

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

// Read the entire urlTable into memory
// and return the average document length to be used in BM25
float loadUrlTable(std::vector<UrlEntry>& urlTable, const std::string& fn);

// Read the entire lexicon into memory
void loadLexicon(std::map<std::string, LexiconEntry>& lexicon, const std::string& fn);

// Returns a vector of terms from the query
std::vector<std::string> parseQuery(const std::string& aQuery);

// Returns an excerpt of the document with the terms
std::string generateSnippet(const std::vector<std::string>& terms, std::string& document);

int main(int argc, const char * argv[]) {
    if (argc != 4) {
        std::cerr << "Argument error: you need to provide pathToUrlTable, pathToLexicon, and pathToInvertedIndex\n";
        exit(1);
    }
    
    // Load url table
    std::cout << "Loading urlTable...\n";
    std::vector<UrlEntry> urlTable;
    std::chrono::steady_clock::time_point beginLoadTable = std::chrono::steady_clock::now();
    const float D_AVG = loadUrlTable(urlTable, argv[1]);
    std::chrono::steady_clock::time_point endLoadTable = std::chrono::steady_clock::now();
    std::cout << std::to_string(urlTable.size()) << " entries loaded to urlTable. Elapsed: " << std::chrono::duration_cast<std::chrono::seconds>(endLoadTable - beginLoadTable).count() << "s.\n";

    // Constants for BM25
    const size_t N = urlTable.size();
    const float K_1 = 1.2;
    const float B = 0.75;

    // Load lexicon
    std::cout << "Loading lexicon...\n";
    std::map<std::string, LexiconEntry> lexicon;
    std::chrono::steady_clock::time_point beginLoadLexicon = std::chrono::steady_clock::now();
    loadLexicon(lexicon, argv[2]);
    std::chrono::steady_clock::time_point endLoadLexicon = std::chrono::steady_clock::now();
    std::cout << std::to_string(lexicon.size()) << " entries loaded to lexicon. Elapsed: " << std::chrono::duration_cast<std::chrono::seconds>(endLoadLexicon - beginLoadLexicon).count() << "s.\n";

    // Get final inverted index filename
    std::string indexFn = argv[3];

    // Init for snippet generation
    DocumentStore docStore("sqlite");

    // Main loop: prompt user for input
    std::string query;
    while(std::cout << "\n====================\n\nPlease enter your query: " && std::getline(std::cin, query)) {
        if (query.empty()) { continue; }

        std::chrono::steady_clock::time_point beginSearch = std::chrono::steady_clock::now();
        std::cout << "\nSearching '" << query << "'...\n\n";

        // Max heap of all top results for this search
        std::priority_queue<std::pair<float, size_t>> topDids;

        // Parse query
        std::vector<std::string> terms = parseQuery(query);
        if (terms.empty()) {
            std::cout << "\nNo keyword found.\n\n";
            continue;
        }

        // TODO: disjunctive queries
        
        // start of DAAT processing
        std::vector<ListPointer> allLps;
        for(auto t : terms) {
            // TODO: optimize by sorting by length of inv list
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

                // Keep all intersected dids in heap
                topDids.push(std::make_pair(score, d));

                // Increase to search for next post
                did++;
            }
        }
        for (size_t i = 0; i < allLps.size(); ++i) { allLps[i].closeList(); }
        // end of DAAT

        // Print out results
        std::chrono::steady_clock::time_point endSearch = std::chrono::steady_clock::now();
        std::cout << topDids.size() << " results found (" << std::chrono::duration_cast<std::chrono::milliseconds>(endSearch - beginSearch).count() << " ms). Most relevant ones:\n\n";
        size_t i = 0;
        while (!topDids.empty() && i < 10) {
            size_t did = topDids.top().second;
            std::cout << ++i << ".\tLink: " << urlTable[did].url << '\n';
            std::cout << "\tRelevance score (BM25): " << topDids.top().first << "\n";
            std::string document = docStore.getDocument(did);
            std::string snippet = generateSnippet(terms, document);
            std::cout << "\tSnippet: ..." << snippet << "...\n\n";
            topDids.pop();
        }
    }
    
    docStore.close();
    
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

float loadUrlTable(std::vector<UrlEntry>& urlTable, const std::string& fn) {
    std::ifstream tableIfs(fn);
    if (!tableIfs) {
        std::cerr << "Failed to open " << fn << '\n';
        exit(1);
    }
    
    float avg = 0;
    size_t did;
    std::string url;
    size_t urlLen;
    while (tableIfs >> did >> url >> urlLen) {
        urlTable.push_back(UrlEntry(url, urlLen));
        avg += urlLen;
    }
    
    return (avg / urlTable.size());
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

std::string generateSnippet(const std::vector<std::string>& terms, std::string& document) {
    // Since doc was normalized when indexing, need to do it again to find the term...
    std::transform(document.begin(), document.end(), document.begin(), ::tolower);
    
    std::vector<size_t> termPos;
    for (std::string t : terms) {
        size_t pos = document.find(t);
        if (pos == std::string::npos) {
            std::cerr << "Error: Cannot find term in document. Weird?\n";
            exit(1);
        }
        termPos.push_back((size_t)pos);
    }
    size_t startSnippet = *(std::min_element(termPos.begin(), termPos.end()));
    startSnippet = (startSnippet < 500) ? startSnippet : (startSnippet - 500);
    size_t endSnippet = *(std::max_element(termPos.begin(), termPos.end())) + 500;
    size_t lenSnippet = (endSnippet - startSnippet) > 2000 ? 2000 : (endSnippet - startSnippet);
    
    std::string snippet = document.substr(startSnippet, lenSnippet);
    std::replace(snippet.begin(), snippet.end(), '\n', ' ');
    return snippet;
}
