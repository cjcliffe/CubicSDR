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

    //Set a (new) 'input' queue for incoming data.
    void setInput(ThreadQueue<InputDataType *> *vis_in) {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);
        input = vis_in;
        
    }
    
    //Add a vis_out queue where to consumed 'input' data will be
    //dispatched by distribute(). 
    void attachOutput(ThreadQueue<OutputDataType *> *vis_out) {
        // attach an output queue
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);
        outputs.push_back(vis_out);
    }
    
    //reverse of attachOutput(), removed an existing attached vis_out.
    void removeOutput(ThreadQueue<OutputDataType *> *vis_out) {
        // remove an output queue
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        typename std::vector<ThreadQueue<OutputDataType *> *>::iterator i = std::find(outputs.begin(), outputs.end(), vis_out);
        if (i != outputs.end()) {
            outputs.erase(i);
        }
    }
    
    //Call process() repeateadly until all available 'input' data is consumed.
    void run() {
        
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        if (input && !input->empty()) {
            process();
        }
       
    }
    
protected:
    // derived class must implement a  process() interface
    //where typically 'input' data is consummed, procerssed, and then dispatched
    //with distribute() to all 'outputs'.
    virtual void process() = 0;

    //To be used by derived classes implementing 
    //process() : will dispatch 'item' into as many 
    //available outputs, previously set by attachOutput().
    void distribute(OutputDataType *item) {
      
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);
        //We will try to distribute 'output' among all 'outputs',
        //so 'output' will a-priori be shared among all 'outputs' so set its ref count to this 
        //amount.
        item->setRefCount((int)outputs.size());
        for (outputs_i = outputs.begin(); outputs_i != outputs.end(); outputs_i++) {
            //if 'output' failed to be given to an outputs_i, dec its ref count accordingly.
        	if (!(*outputs_i)->push(item)) {
                item->decRefCount();
        	} 
        }

        // Now 'item' refcount matches the times 'item' has been successfully distributed,
        //i.e shared among the outputs.
    }

    //the incoming data queue 
    ThreadQueue<InputDataType *> *input = nullptr;
    
    //the n-outputs where to process()-ed data is distribute()-ed.
    std::vector<ThreadQueue<OutputDataType *> *> outputs;
	
    typename std::vector<ThreadQueue<OutputDataType *> *>::iterator outputs_i;

    //protects input and outputs, must be recursive because of re-entrance
    std::recursive_mutex busy_update;
};

//Specialization much like VisualDataReDistributor, except 
//the input (pointer) is directly re-dispatched
//to outputs, so that all output indeed SHARE the same instance. 
template<class OutputDataType = ReferenceCounter>
class VisualDataDistributor : public VisualProcessor<OutputDataType, OutputDataType> {
protected:
    virtual void process() {
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

//specialization class which process() take an input item and re-dispatch
//A COPY to every outputs, without further processing. This is a 1-to-n dispatcher. 
template<class OutputDataType = ReferenceCounter>
class VisualDataReDistributor : public VisualProcessor<OutputDataType, OutputDataType> {
protected:
    virtual void process() {
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
