//
// Created by Nicolas Loira on 11-01-23.
//

#include <iostream>

#include "tubul.h"

int main(){
    std::cout << "Hello Tubul version: " << TU::getVersion() << ".\n";
	std::string hello("Hello world 1 2 3");
	auto tokens = TU::split(hello ) ;
	std::cout << "I can split and join strings: " << hello << " -> '" << TU::join(tokens,"->") << "'" << std::endl;
	std::string_view hello_view  = hello;
	auto tokens_from_view = TU::split(hello_view);
	std::cout << "Also works with string_views: " << hello_view << " -> '" << TU::join(tokens_from_view,"->") << "'" << std::endl;

}