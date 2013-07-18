#pragma once

#define SHARE                        (1000ll)               // used to position the decimal place
#define INIT_BLOCK_REWARD            (5000000ll)            // initial reward amount
#define BLOCK_INTERVAL               (5)                    // minutes between blocks
#define BLOCKS_PER_HOUR              (60/BLOCK_INTERVAL)           
#define BLOCKS_PER_DAY               (BLOCKS_PER_HOUR*24)
#define BLOCKS_PER_YEAR              (BLOCKS_PER_DAY*365)
#define COINBASE_WAIT_PERIOD         (BLOCKS_PER_HOUR*8) // blocks before a coinbase can be spent
#define DEFAULT_SERVER_PORT          (9876)
#define DESIRED_PEER_COUNT           (8)                 // number of nodes to connect to
#define MAX_MESSAGE_SIZE             (8*1024*1024)
#define BITCHAT_TARGET_BPS           (128*1024)          // 128 kbit / sec target data rate
#define BITCHAT_BANDWIDTH_WINDOW_US  (5*60*1000*1000ll)  // 5 minutes
#define BITCHAT_INVENTORY_WINDOW_SEC (60)                // seconds to keep inventory items around
