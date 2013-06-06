#pragma once
#include "api.hpp"
#include <fc/thread/thread.hpp>


/**
 *  The miner will monitor the block chain for changes and
 *  attempt to solve new blocks as they come in.  When a new
 *  block is solved it will be 'added' to the chain.
 */
class miner
{
    public:
       miner( block_chain& bc );
       ~miner();

       void start( float effort = 1);
       void stop();

    private:
       fc::thread mining_thread;
};
