//
// Created by Nicolas Loira on 11-01-23.
//

#include "tubul.h"
#include "tubul_engine.h"

namespace TU {

std::unique_ptr<TubulEngine> Tubul::engine_ = nullptr;

Tubul::Tubul(){
	engine_ = std::make_unique<TubulEngine>();
}

TubulEngine &Tubul::getInstance(){

	if(engine_)
		return *engine_;

	throw std::runtime_error("Tubul not created.");
}

    int getVersion() { return 0; }
}