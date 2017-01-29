// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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
    
    bool isInputEmpty() {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        return input->empty();
    }
    
    bool isOutputEmpty() {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        for (outputs_i = outputs.begin(); outputs_i != outputs.end(); outputs_i++) {
            if ((*outputs_i)->full()) {
                return false;
            }
        }
        return true;
    }
    
    bool isAnyOutputEmpty() {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        for (outputs_i = outputs.begin(); outputs_i != outputs.end(); outputs_i++) {
            if (!(*outputs_i)->full()) {
                return true;
            }
        }
        return false;
    }

    void setInput(ThreadQueue<InputDataType *> *vis_in) {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);
        input = vis_in;
        
    }
    
    void attachOutput(ThreadQueue<OutputDataType *> *vis_out) {
        // attach an output queue
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);
        outputs.push_back(vis_out);
       
    }
    
    void removeOutput(ThreadQueue<OutputDataType *> *vis_out) {
        // remove an output queue
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        typename std::vector<ThreadQueue<OutputDataType *> *>::iterator i = std::find(outputs.begin(), outputs.end(), vis_out);
        if (i != outputs.end()) {
            outputs.erase(i);
        }
      
    }
    
    void run() {
        
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        if (input && !input->empty()) {
            process();
        }
       
    }
    
protected:
    virtual void process() {
        // process inputs to output
        // distribute(output);
    }

    void distribute(OutputDataType *output) {
        // distribute outputs
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        output->setRefCount((int)outputs.size());
        for (outputs_i = outputs.begin(); outputs_i != outputs.end(); outputs_i++) {

        	if (!(*outputs_i)->push(output)) {
        		output->decRefCount();
        	} 
        }
    }

    ThreadQueue<InputDataType *> *input = nullptr;
    std::vector<ThreadQueue<OutputDataType *> *> outputs;
	typename std::vector<ThreadQueue<OutputDataType *> *>::iterator outputs_i;

    //protects input and outputs, must be recursive because ao reentrance
    std::recursive_mutex busy_update;
};


template<class OutputDataType = ReferenceCounter>
class VisualDataDistributor : public VisualProcessor<OutputDataType, OutputDataType> {
protected:
    void process() {
        OutputDataType *inp;
        while (VisualProcessor<OutputDataType, OutputDataType>::input->try_pop(inp)) {
            
            if (!VisualProcessor<OutputDataType, OutputDataType>::isAnyOutputEmpty()) {
                if (inp) {
            	    inp->decRefCount();
                }
                return;
            }
      
            if (inp) {
            	VisualProcessor<OutputDataType, OutputDataType>::distribute(inp);
            }
        }
    }
};


template<class OutputDataType = ReferenceCounter>
class VisualDataReDistributor : public VisualProcessor<OutputDataType, OutputDataType> {
protected:
    void process() {
        OutputDataType *inp;
        while (VisualProcessor<OutputDataType, OutputDataType>::input->try_pop(inp)) {
            
            if (!VisualProcessor<OutputDataType, OutputDataType>::isAnyOutputEmpty()) {
                if (inp) {
            	    inp->decRefCount();
                }
                return;
            }
            
            if (inp) {
                OutputDataType *outp = buffers.getBuffer();
                (*outp) = (*inp);
                inp->decRefCount();
                VisualProcessor<OutputDataType, OutputDataType>::distribute(outp);
            }
        }
    }
    ReBuffer<OutputDataType> buffers;
};
