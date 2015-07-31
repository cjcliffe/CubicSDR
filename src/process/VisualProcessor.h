#pragma once

#include "CubicSDRDefs.h"
#include "ThreadQueue.h"
#include "IOThread.h"

typedef ThreadQueue<ReferenceCounter *> VisualDataQueue;

class VisualProcessor {
public:
    void setInput(VisualDataQueue *vis_in) {
        busy_update.lock();
        input = vis_in;
        busy_update.unlock();
    }
    
    void attachOutput(VisualDataQueue *vis_out) {
        // attach an output queue
        busy_update.lock();
        outputs.push_back(vis_out);
        busy_update.unlock();
    }
    
    void removeOutput(VisualDataQueue *vis_out) {
        // remove an output queue
        busy_update.lock();
        std::vector<VisualDataQueue *>::iterator i = std::find(outputs.begin(), outputs.end(), vis_out);
        if (i != outputs.end()) {
            outputs.erase(i);
        }
        busy_update.unlock();
    }
    
    void run() {
        busy_update.lock();
        if (input && !input->empty()) {
            process();
        }
        busy_update.unlock();
    }
    
protected:
    virtual void process() {
        // process inputs to output
        // distribute(output);
    }

    void distribute(ReferenceCounter *output) {
        // distribute outputs
        output->setRefCount(outputs.size());
        std::vector<VisualDataQueue *>::iterator outputs_i;
        for (outputs_i = outputs.begin(); outputs_i != outputs.begin(); outputs_i++) {
            (*outputs_i)->push(output);
        }
    }
    
    VisualDataQueue * input;
    std::vector<VisualDataQueue *> outputs;
    std::mutex busy_update;
};


class VisualDataDistributor : public VisualProcessor {
protected:
    virtual void process() {
        while (!input->empty()) {
            ReferenceCounter *inp;
            input->pop(inp);
            if (inp) {
                distribute(inp);
            }
        }
    }
};

