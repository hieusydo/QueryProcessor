//
//  QueryProcessor.hpp
//  QueryProcessor
//
//  Created by Hieu Do on 11/5/18.
//  Copyright © 2018 Hieu Do. All rights reserved.
//

#ifndef QueryProcessor_hpp
#define QueryProcessor_hpp

#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <fstream>
#include <math.h>
#include <locale>
#include <algorithm>

#include "DocumentStore.hpp"
#include "ListPointer.hpp"

class QueryProcessor {
    struct UrlEntry {
        UrlEntry(const std::string& u, size_t d) : url(u), documentLen(d) {}
        std::string url;
        size_t documentLen;
    };
    
    struct LexiconEntry {
        LexiconEntry() {} // for std::map
        LexiconEntry(size_t p, size_t s) : invListPos(p), metadataSize(s) {}
        size_t invListPos;
        size_t metadataSize;
    };
    
    struct DocScore {
        DocScore(float s, size_t d) : score(s), did(d) {}
        float score;
        size_t did;
    };
    
    class DocScoreComparator
    {
    public:
        int operator() (const DocScore& d1, const DocScore& d2)
        {
            return d1.score > d2.score;
        }
    };
    
private:
    std::string urlTableFn;
    std::string lexiconFn;
    std::string indexFn;
    
    std::vector<UrlEntry> urlTable;
    std::map<std::string, LexiconEntry> lexicon;
    
    // Max heap of all top results for this search
    std::priority_queue<DocScore, std::vector<DocScore>, DocScoreComparator> topDids;
    size_t resultCnt; 
    
    // Constants
    float D_AVG;
    size_t N;
    const float K_1 = 1.2;
    const float B = 0.75;
    const size_t MAX_SIZE = (size_t) - 1;
    
    // Read the entire urlTable into memory
    void loadUrlTable();
    // Read the entire lexicon into memory
    void loadLexicon();
    
    // Returns a vector of terms from the query
    std::vector<std::string> parseQuery(const std::string& aQuery, const std::string& boolTerm);
    
    // Call to run DAAT intersection algorithm
    void processConjunctiveDAAT(const std::vector<std::string>& terms);
    
    // Call to run DAAT union algorithm
    void processDisjunctiveDAAT(const std::vector<std::string>& terms);
    
    // Returns an excerpt of the document with the terms
    std::string generateSnippet(const std::vector<std::string>& terms, std::string& document);
    
public:
    QueryProcessor(const std::string& urlTableFn, const std::string& lexiconFn, const std::string& indexFn);
    
    void search(const std::string& query);
};

#endif /* QueryProcessor_hpp */
