#pragma once

#include "CubicSDRDefs.h"
#include "ThreadQueue.h"
#include "IOThread.h"
#include <algorithm>

template<class InputDataType = ReferenceCounter, class OutputDataType = ReferenceCounter>
class VisualProcessor {
public:
	virtual ~VisualProcessor() {

	}

    void setInput(ThreadQueue<InputDataType *> *vis_in) {
        busy_update.lock();
        input = vis_in;
        busy_update.unlock();
    }
    
    void attachOutput(ThreadQueue<OutputDataType *> *vis_out) {
        // attach an output queue
        busy_update.lock();
        outputs.push_back(vis_out);
        busy_update.unlock();
    }
    
    void removeOutput(ThreadQueue<OutputDataType *> *vis_out) {
        // remove an output queue
        busy_update.lock();
        typename std::vector<ThreadQueue<OutputDataType *> *>::iterator i = std::find(outputs.begin(), outputs.end(), vis_out);
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

    void distribute(OutputDataType *output) {
        // distribute outputs
        output->setRefCount(outputs.size());
        for (outputs_i = outputs.begin(); outputs_i != outputs.end(); outputs_i++) {
        	if ((*outputs_i)->full()) {
        		output->decRefCount();
        	} else {
        		(*outputs_i)->push(output);
        	}
        }
    }
    
    bool isOutputEmpty() {
        for (outputs_i = outputs.begin(); outputs_i != outputs.end(); outputs_i++) {
            if (!(*outputs_i)->empty()) {
            	return false;
            }
        }
        return true;
    }

    bool isAnyOutputEmpty() {
        for (outputs_i = outputs.begin(); outputs_i != outputs.end(); outputs_i++) {
            if ((*outputs_i)->empty()) {
            	return true;
            }
        }
        return false;
    }

    ThreadQueue<InputDataType *> *input;
    std::vector<ThreadQueue<OutputDataType *> *> outputs;
	typename std::vector<ThreadQueue<OutputDataType *> *>::iterator outputs_i;
    std::mutex busy_update;
};


template<class OutputDataType = ReferenceCounter>
class VisualDataDistributor : public VisualProcessor<OutputDataType, OutputDataType> {
protected:
    void process() {
        if (!VisualProcessor<OutputDataType, OutputDataType>::isOutputEmpty()) {
            return;
        }
        while (!VisualProcessor<OutputDataType, OutputDataType>::input->empty()) {
        	OutputDataType *inp;
        	VisualProcessor<OutputDataType, OutputDataType>::input->pop(inp);
            if (inp) {
            	VisualProcessor<OutputDataType, OutputDataType>::distribute(inp);
            }
        }
    }
};

