//
// Created by Nicolas Loira on 16-03-23.
//

#include "tubul_engine_factory.h"

namespace TU {

    TubulEngine &getInstance() {
        static TubulEngine engine;
        return engine;
    }

}