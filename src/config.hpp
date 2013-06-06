#pragma once

#define SHARE                       (100000000ll)  // used to position the decimal place
#define INIT_BLOCK_REWARD           (50ll) // initial reward amount
#define BLOCK_INTERVAL              (10) // minutes between blocks
#define REWARD_ADJUSTMENT_INTERVAL  ((60/BLOCK_INTERVAL)*24*365*4) // every 4 years
#define COINBASE_WAIT_PERIOD        (144) // blocks before a coinbase can be spent
