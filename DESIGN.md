Account:
  - contains a set of addresses and all transactions that reference them.
  - contains a wallet that may have some or all of the private keys for the account.

Wallet: 
  - contains a set of private keys and the matching public keys.  Manages Encryption 
  and security of the private keys.

BlockChain:
  - contains all known blocks + the current longest block.
  - validates all transactions / transfers.
  - applies validated blocks.
  - builds new blocks.

Market:
  - used by the BlockChain, organizes / manages all transactions
    associated with a buy/sell pair.

Miner:
  - given a block will attempt to solve the proof-of-work.

Node:
  - Provides communication with other nodes, receives transactions / blocks,
    and will broadcast transactions / blocks.
