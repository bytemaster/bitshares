
/**
 *  Caches all transactions that contain an input
 *  or output that references an address managed
 *  by this wallet. 
 */
struct public_wallet
{
   uint32_t                        last_block;
   std::vector<public_wallet_key>  addresses;
   std::vector<transaction>        transactions;
};

