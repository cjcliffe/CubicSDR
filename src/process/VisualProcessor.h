#pragma once

#include "CubicSDRDefs.h"
#include "ThreadQueue.h"

typedef ThreadQueue<ReferenceCounter *> VisualDataQueue;

class VisualProcessor {
public:
    void setInput(VisualDataQueue *vis_in) {
        // set input queue
    }
    
    void attachOutput(VisualDataQueue *vis_out) {
        // attach an output queue
    }
    
    void removeOutput(VisualDataQueue *vis_out) {
        // remove an output queue
    }
    
    void pushInput(ReferenceCounter *input) {
        // push input data
    }
    
    virtual void process() {
        // process input to output
        // distribute(output);
    }
    
protected:
    void distribute(ReferenceCounter *output) {
        // distribute outputs
    }
    
    VisualDataQueue * input;
    std::vector<VisualDataQueue *> outputs;
    
    std::mutex busy_update;
};

