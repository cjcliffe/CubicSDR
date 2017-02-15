// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "CubicSDRDefs.h"
#include "ThreadBlockingQueue.h"
#include "IOThread.h"
#include <algorithm>
#include <vector>

template<typename InputDataType = ReferenceCounter, typename OutputDataType = ReferenceCounter>
class VisualProcessor {
    //
    typedef  ThreadBlockingQueue<InputDataType*> VisualInputQueueType;
    typedef  ThreadBlockingQueue<OutputDataType*> VisualOutputQueueType;
    typedef typename std::vector< VisualOutputQueueType *>::iterator outputs_i;
public:
	virtual ~VisualProcessor() {

	}
    
    bool isInputEmpty() {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        return input->empty();
    }
    
    bool isOutputEmpty() {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        for (outputs_i it = outputs.begin(); it != outputs.end(); it++) {
            if ((*it)->full()) {
                return false;
            }
        }
        return true;
    }
    
    bool isAnyOutputEmpty() {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        for (outputs_i it = outputs.begin();  it != outputs.end(); it++) {
            if (!(*it)->full()) {
                return true;
            }
        }
        return false;
    }

    //Set a (new) 'input' queue for incoming data.
    void setInput(VisualInputQueueType *vis_in) {
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);
        input = vis_in;
        
    }
    
    //Add a vis_out queue where to consumed 'input' data will be
    //dispatched by distribute(). 
    void attachOutput(VisualOutputQueueType *vis_out) {
        // attach an output queue
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);
        outputs.push_back(vis_out);
    }
    
    //reverse of attachOutput(), removed an existing attached vis_out.
    void removeOutput(VisualOutputQueueType *vis_out) {
        // remove an output queue
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);

        outputs_i i = std::find(outputs.begin(), outputs.end(), vis_out);
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
    //* \param[in] timeout The number of microseconds to wait to push an item in each one of the outputs, 0(default) means indefinite wait.
    //* \param[in] errorMessage an error message written on std::cout in case pf push timeout.
    void distribute(OutputDataType *item, std::uint64_t timeout = BLOCKING_INFINITE_TIMEOUT, const char* errorMessage = "") {
      
        std::lock_guard < std::recursive_mutex > busy_lock(busy_update);
        //We will try to distribute 'output' among all 'outputs',
        //so 'output' will a-priori be shared among all 'outputs' so set its ref count to this 
        //amount.
        item->setRefCount((int)outputs.size());
        for (outputs_i it = outputs.begin(); it != outputs.end(); it++) {
            //if 'output' failed to be given to an outputs_i, dec its ref count accordingly.
            //blocking push, with a timeout
        	if (!(*it)->push(item, timeout, errorMessage)) {
                item->decRefCount();
        	} 
        }
        // Now 'item' refcount matches the times 'item' has been successfully distributed,
        //i.e shared among the outputs.
    }

    //the incoming data queue 
    VisualInputQueueType *input = nullptr;
    
    //the n-outputs where to process()-ed data is distribute()-ed.
    std::vector<VisualOutputQueueType *> outputs;

    //protects input and outputs, must be recursive because of re-entrance
    std::recursive_mutex busy_update;
};

//Specialization much like VisualDataReDistributor, except 
//the input (pointer) is directly re-dispatched
//to outputs, so that all output indeed SHARE the same instance. 
template<typename OutputDataType = ReferenceCounter>
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
                int previousRefCount = inp->getRefCount();
            	VisualProcessor<OutputDataType, OutputDataType>::distribute(inp);
                //inp is now shared through the distribute(), which overwrite the previous ref count,
                //so increment it properly.
                int distributeRefCount = inp->getRefCount();
                inp->setRefCount(previousRefCount + distributeRefCount);
            }
        }
    }
};

//specialization class which process() take an input item and re-dispatch
//A COPY to every outputs, without further processing. This is a 1-to-n dispatcher. 
template<typename OutputDataType = ReferenceCounter>
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
