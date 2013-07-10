#pragma once
#pragma once

#define SHARE                   (1000ll)               // used to position the decimal place
#define INIT_BLOCK_REWARD       (5000000ll)            // initial reward amount
#define BLOCK_INTERVAL          (5)                    // minutes between blocks
#define BLOCKS_PER_HOUR         (60/BLOCK_INTERVAL)           
#define BLOCKS_PER_DAY          (BLOCKS_PER_HOUR*24)
#define BLOCKS_PER_YEAR         (BLOCKS_PER_DAY*365)
#define COINBASE_WAIT_PERIOD    (BLOCKS_PER_HOUR*8) // blocks before a coinbase can be spent
#define DEFAULT_SERVER_PORT     (9876)
