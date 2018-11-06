//
//  main.cpp
//  QueryProcessor
//
//  Created by Hieu Do on 10/29/18.
//  Copyright © 2018 Hieu Do. All rights reserved.
//

#include <iostream>

#include "QueryProcessor.hpp"

int main(int argc, const char * argv[]) {
    if (argc != 4) {
        std::cerr << "Argument error: you need to provide pathToUrlTable, pathToLexicon, and pathToInvertedIndex\n";
        exit(1);
    }
    
    QueryProcessor qp(argv[1], argv[2], argv[3]);
    
    // Main loop: prompt user for input
    std::string query;
    while(std::cout << "\n====================\n\nPlease enter your query: " && std::getline(std::cin, query)) {
        std::cout << "\nSearching '" << query << "'...\n\n";
        qp.search(query);
    }
    
    return 0;
}
