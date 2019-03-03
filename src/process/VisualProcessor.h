// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "CubicSDRDefs.h"
#include "ThreadBlockingQueue.h"
#include "IOThread.h"
#include <algorithm>
#include <vector>
#include <typeinfo>
#include "SpinMutex.h"

template<typename InputDataType, typename OutputDataType>
class VisualProcessor {

public:
    //
    typedef typename std::shared_ptr<InputDataType> InputDataTypePtr;
    typedef typename std::shared_ptr<OutputDataType> OutputDataTypePtr;

    typedef  ThreadBlockingQueue<InputDataTypePtr> VisualInputQueueType;
    typedef  ThreadBlockingQueue<OutputDataTypePtr> VisualOutputQueueType;

    typedef  std::shared_ptr<VisualInputQueueType> VisualInputQueueTypePtr;
    typedef  std::shared_ptr<VisualOutputQueueType> VisualOutputQueueTypePtr;

	virtual ~VisualProcessor() {
	}
    
    bool isInputEmpty() {
        std::lock_guard < SpinMutex > busy_lock(busy_update);
		
		if (input) {
			return input->empty();
		}

		return true;
    }
    
    bool isOutputEmpty() {
        std::lock_guard < SpinMutex > busy_lock(busy_update);

        for (VisualOutputQueueTypePtr single_output :  outputs) {
            if (single_output->full()) {
                return false;
            }
        }
        return true;
    }
    
    bool isAnyOutputEmpty() {
        std::lock_guard < SpinMutex > busy_lock(busy_update);

        for (VisualOutputQueueTypePtr single_output : outputs) {
            if (!(single_output)->full()) {
                return true;
            }
        }
        return false;
    }

    //Set a (new) 'input' queue for incoming data.
    void setInput(VisualInputQueueTypePtr vis_in) {
        std::lock_guard < SpinMutex > busy_lock(busy_update);
        input = vis_in;
        
    }
    
    //Add a vis_out queue where to consumed 'input' data will be
    //dispatched by distribute(). 
    void attachOutput(VisualOutputQueueTypePtr vis_out) {
        // attach an output queue
        std::lock_guard < SpinMutex > busy_lock(busy_update);
        outputs.push_back(vis_out);
    }
    
    //reverse of attachOutput(), removed an existing attached vis_out.
    void removeOutput(VisualOutputQueueTypePtr vis_out) {
        // remove an output queue
        std::lock_guard < SpinMutex > busy_lock(busy_update);

        auto it = std::find(outputs.begin(), outputs.end(), vis_out);
        if (it != outputs.end()) {
            outputs.erase(it);
        }
    }
    //Flush all queues, either input or outputs clearing their accumulated messages.
    //this is purposefully (almost) non-blocking call.
    void flushQueues() {
       
		//capture a local copy atomically, so we don't need to protect input.
		VisualInputQueueTypePtr localInput = input;

		if (localInput) {
			localInput->flush();
		}

		//scoped-lock: create a local copy of outputs, and work with it.
		std::vector<VisualOutputQueueTypePtr> local_outputs;
		{
			std::lock_guard < SpinMutex > busy_lock(busy_update);
			local_outputs = outputs;
		}

        for (auto single_output : local_outputs) {

            single_output->flush();
        }
    }
    
    //Call process() repeateadly until all available 'input' data is consumed.
    void run() {
		
		//capture a local copy atomically, so we don't need to protect input.
		VisualInputQueueTypePtr localInput = input;

        if (localInput && !localInput->empty()) {
            process();
		}
    }
    
protected:
    // derived class must implement a  process() interface
    //where typically 'input' data is consummed, processed, and then dispatched
    //with distribute() to all 'outputs'.
    virtual void process() = 0;

    //To be used by derived classes implementing 
    //process() : will dispatch 'item' into as many 
    //available outputs, previously set by attachOutput().
    //* \param[in] timeout The number of microseconds to wait to push an item in each one of the outputs, 0(default) means indefinite wait.
    //* \param[in] errorMessage an error message written on std::cout in case pf push timeout.
    void distribute(OutputDataTypePtr item, std::uint64_t timeout = BLOCKING_INFINITE_TIMEOUT, const char* errorMessage = nullptr) {
      
        //scoped-lock: create a local copy of outputs, and work with it.
        std::vector<VisualOutputQueueTypePtr> local_outputs;
        {
            std::lock_guard < SpinMutex > busy_lock(busy_update);
            local_outputs = outputs;
        }

        //We will try to distribute 'output' among all 'local_outputs',
        //so 'output' will a-priori be shared among all 'local_outputs'.
        
        for (VisualOutputQueueTypePtr single_output : local_outputs) {
            //'output' can fail to be given to an single_output,
            //using a blocking push, with a timeout
        	if (!(single_output)->push(item, timeout, errorMessage)) {
                //trace  will be std::output if timeout != 0 is set and errorMessage != null.
        	} 
        }
    }

    //the incoming data queue 
    VisualInputQueueTypePtr input;
    
    //the n-outputs where to process()-ed data is distribute()-ed.
    std::vector<VisualOutputQueueTypePtr> outputs;

    //protects input and outputs
    SpinMutex busy_update;
};

//Specialization much like VisualDataReDistributor, except 
//the input (pointer) is directly re-dispatched
//to outputs, so that all output indeed SHARE the same instance. 
template<typename OutputDataType>
class VisualDataDistributor : public VisualProcessor<OutputDataType, OutputDataType> {
protected:

    virtual void process() {

        typename VisualProcessor<OutputDataType, OutputDataType>::OutputDataTypePtr inp;

        while (VisualProcessor<OutputDataType, OutputDataType>::input->try_pop(inp)) {
            
			//do not try to distribute if all outputs are already full.
            if (!VisualProcessor<OutputDataType, OutputDataType>::isAnyOutputEmpty()) {
               
                return;
            }
      
            if (inp) {
            	VisualProcessor<OutputDataType, OutputDataType>::distribute(inp);
                //inp is now shared through the distribute() call.
            }
        }
    }
};

//specialization class which process() take an input item and re-dispatch
//A COPY to every outputs, without further processing. This is a 1-to-n dispatcher. 
template<typename OutputDataType>
class VisualDataReDistributor : public VisualProcessor<OutputDataType, OutputDataType> {

protected:

    VisualDataReDistributor() : buffers (std::string(typeid(*this).name())) {

    }

    ReBuffer<OutputDataType> buffers;

    virtual void process() {

        typename VisualProcessor<OutputDataType, OutputDataType>::OutputDataTypePtr inp;

        while (VisualProcessor<OutputDataType, OutputDataType>::input->try_pop(inp)) {
            
			//do not try to distribute if all outputs are already full.
            if (!VisualProcessor<OutputDataType, OutputDataType>::isAnyOutputEmpty()) {
               
                return;
            }
            
            if (inp) {

                typename VisualProcessor<OutputDataType, OutputDataType>::OutputDataTypePtr outp = buffers.getBuffer();

                //'deep copy' of the contents 
                (*outp) = (*inp);
  
                VisualProcessor<OutputDataType, OutputDataType>::distribute(outp);
            }
        }
    }
    
};
