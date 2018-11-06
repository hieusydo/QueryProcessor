//
//  QueryProcessor.cpp
//  QueryProcessor
//
//  Created by Hieu Do on 11/5/18.
//  Copyright © 2018 Hieu Do. All rights reserved.
//

#include "QueryProcessor.hpp"

QueryProcessor::QueryProcessor(const std::string& urlTableFn, const std::string& lexiconFn, const std::string& indexFn) : urlTableFn(urlTableFn), lexiconFn(lexiconFn), indexFn(indexFn) {
    // Load url table
    std::cout << "Loading urlTable...\n";
    std::chrono::steady_clock::time_point beginLoadTable = std::chrono::steady_clock::now();
    loadUrlTable();
    std::chrono::steady_clock::time_point endLoadTable = std::chrono::steady_clock::now();
    std::cout << std::to_string(urlTable.size()) << " entries loaded to urlTable. Elapsed: " << std::chrono::duration_cast<std::chrono::seconds>(endLoadTable - beginLoadTable).count() << "s.\n";
    
    // Load lexicon
    std::cout << "Loading lexicon...\n";
    std::chrono::steady_clock::time_point beginLoadLexicon = std::chrono::steady_clock::now();
    loadLexicon();
    std::chrono::steady_clock::time_point endLoadLexicon = std::chrono::steady_clock::now();
    std::cout << std::to_string(lexicon.size()) << " entries loaded to lexicon. Elapsed: " << std::chrono::duration_cast<std::chrono::seconds>(endLoadLexicon - beginLoadLexicon).count() << "s.\n";
}

void QueryProcessor::loadUrlTable() {
    std::ifstream tableIfs(urlTableFn);
    if (!tableIfs) {
        std::cerr << "Failed to open " << urlTableFn << '\n';
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
    
    D_AVG = (avg / urlTable.size());
    N = urlTable.size();
}

void QueryProcessor::loadLexicon() {
    std::ifstream lexiconIfs(lexiconFn);
    if (!lexiconIfs) {
        std::cerr << "Failed to open " << lexiconFn << '\n';
        exit(1);
    }
    
    std::string term;
    size_t invIdxPos;
    size_t metadataLen;
    while (lexiconIfs >> term >> invIdxPos >> metadataLen) {
        lexicon[term] = LexiconEntry(invIdxPos, metadataLen);
    }
}

void QueryProcessor::search(const std::string& query) {
    if (query.empty()) { return; }
    
    std::chrono::steady_clock::time_point beginSearch = std::chrono::steady_clock::now();
    
    resultCnt = 0;
    
    // Parse and process query
    std::string conjunctive = " and ";
    std::string disjunctive = " or ";
    std::vector<std::string> terms;
    if (query.find(conjunctive) != std::string::npos) {
        terms = parseQuery(query, conjunctive);
        processConjunctiveDAAT(terms);
    } else if (query.find(disjunctive) != std::string::npos) {
        terms = parseQuery(query, disjunctive);
        processDisjunctiveDAAT(terms);
    } else {
        terms.push_back(query);
        processConjunctiveDAAT(terms);
    }
    
    // Print out results
    std::chrono::steady_clock::time_point endSearch = std::chrono::steady_clock::now();
    std::cout << resultCnt << " results found (" << std::chrono::duration_cast<std::chrono::milliseconds>(endSearch - beginSearch).count() << " ms).";
    
    if (!topDids.empty()) { std::cout << "Most relevant ones:\n\n"; }
    
    DocumentStore docStore("sqlite");
    std::vector<DocScore> sortedResults;
    while (!topDids.empty()) {
        DocScore d = topDids.top();
        sortedResults.push_back(d);
        topDids.pop();
    }
    for (int i = (int)sortedResults.size() - 1; i >= 0; --i) {
        size_t currDid = sortedResults[i].did;
        std::cout << (sortedResults.size() - i) << ".\tLink: " << urlTable[currDid].url << '\n';
        
        std::cout << "\tRelevance score (BM25): " << sortedResults[i].score << "\n";
        
        std::string document = docStore.getDocument(currDid);
        if (!document.empty()) {
            std::string snippet = generateSnippet(terms, document);
            std::cout << "\tSnippet: ..." << snippet << "...\n\n";
        }
    }
    docStore.close();
}

std::vector<std::string> QueryProcessor::parseQuery(const std::string& aQuery, const std::string& boolTerm) const {
    std::vector<std::string> terms;
    
    std::vector<size_t> allPos;
    size_t pos = aQuery.find(boolTerm, 0);
    while(pos != std::string::npos) {
        allPos.push_back(pos);
        pos = aQuery.find(boolTerm, pos+1);
    }
    
    size_t i = 0;
    for(size_t p : allPos) {
        terms.push_back(aQuery.substr(i, p-i));
        i = p + boolTerm.size();
    }
    terms.push_back(aQuery.substr(i));
    
    return terms;
}

void QueryProcessor::processConjunctiveDAAT(const std::vector<std::string>& terms) {    
    // start of DAAT processing
    std::vector<ListPointer> allLps;
    for(const std::string& t : terms) {
        // Do not openList if the term wasn't index
        if (lexicon.find(t) == lexicon.end()) { break; }
        
        // TODO: optimize by sorting by length of inv list
        allLps.push_back(ListPointer(indexFn, lexicon[t].invListPos, lexicon[t].metadataSize));
    }
    
    size_t did = 0;
    while (did < MAX_SIZE && allLps.size() == terms.size()) {
        // Get next post from shortest list
        did = allLps[0].nextGEQ(did);
        
        if (did == MAX_SIZE) { break; }
        
        size_t d = did;
        for (size_t i = 1; (i < allLps.size()) && ((d = allLps[i].nextGEQ(did)) == did); ++i) {}
        
        if (d > did) { did = d; } // docId is NOT in intersection
        else { // docId is in intersection            
            // Computer BM25 score
            float bm25Score = getBM25Score(allLps, d);
            
            // Keep top-K intersected dids in heap
            DocScore newDid(bm25Score, d);
            topDids.push(newDid);
            while (topDids.size() > K) { topDids.pop(); }
            resultCnt += 1;
            
            // Increase to search for next post
            did++;
        }
    }
    for (size_t i = 0; i < allLps.size(); ++i) { allLps[i].closeList(); }
}

float QueryProcessor::getBM25Score(const std::vector<ListPointer>& allLps, size_t d) const {
    float score = 0;
    for (size_t i = 0; i < allLps.size(); ++i) {
        // Used number of chunks as f_t instead of number of dids
        // Still reserve relative order
        float numerLeft = N - allLps[i].getNumDid() + 0.5;
        float denomLeft = allLps[i].getNumDid() + 0.5;
        float numerRight = (K_1 + 1) * allLps[i].getFreq();
        const float K = K_1 * ((1 - B) + B * urlTable[d].documentLen / D_AVG);
        float denomRight = K + allLps[i].getFreq();
        score += (log(numerLeft / denomLeft) * (numerRight/denomRight));
    }
    return score;
}

std::string QueryProcessor::generateSnippet(const std::vector<std::string>& terms, std::string& document) const {
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

bool QueryProcessor::terminateDisjunctiveDAAT(const std::vector<size_t>& allCandidates) const {
    // Only stop when ALL LPs are at the end
    for (size_t c : allCandidates) {
        if (c != MAX_SIZE) { return false; }
    }
    return true;
}

void QueryProcessor::processDisjunctiveDAAT(const std::vector<std::string>& terms) {    
    // start of DAAT processing
    std::vector<ListPointer> allLps;
    for(const std::string& t : terms) {
        // Do not openList if the term wasn't index
        if (lexicon.find(t) == lexicon.end()) { break; }
        
        allLps.push_back(ListPointer(indexFn, lexicon[t].invListPos, lexicon[t].metadataSize));
    }
    
    size_t did = 0;
    while (true) {
        std::vector<size_t> didCandidates;
        for (size_t i = 0; i < allLps.size(); ++i) {
            didCandidates.push_back(allLps[i].nextGEQ(did));
        }
        if (terminateDisjunctiveDAAT(didCandidates)) { break; }

        // Consider the smallest did and slowly slide right so as not to skip any did
        did = *(std::min_element(didCandidates.begin(), didCandidates.end()));
        
        // Computer BM25 score
        float bm25Score = getBM25Score(allLps, did);
        
        // Keep top-K intersected dids in heap
        DocScore newDid(bm25Score, did);
        topDids.push(newDid);
        while (topDids.size() > K) { topDids.pop(); }
        resultCnt += 1;
        
        // Increase to search for next post
        did++;
    }
    for (size_t i = 0; i < allLps.size(); ++i) { allLps[i].closeList(); }
}
