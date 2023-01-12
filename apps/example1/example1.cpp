//
// Created by Nicolas Loira on 11-01-23.
//

#include <iostream>

#include "tubul.h"

int main(){
    std::cout << "Hello Tubul version: " << TU::getVersion() << ".\n";
	std::string hello("Hello world");
	auto tokens = TU::split(hello," " ) ;
	std::cout << "Splitting a string: " << hello << " -> ['" << tokens[0] << "','" << tokens[1] << "']" << std::endl;
}