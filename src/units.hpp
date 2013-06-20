#pragma once
#include <fc/reflect/reflect.hpp>

/**
 *  All balanes are annotated with a unit, each BitShare 
 *  chain supports 8 basic units
 */
enum unit_type 
{
    BitShares   = 0,
    OneOzGold   = 1,
    OneOzSilver = 2,
    USD         = 3,
    EUR         = 4,
    YUAN        = 5,
    GBP         = 6,
    BitCoin     = 7,
    NumUnits    
};
FC_REFLECT_ENUM( unit_type, 
      (BitShares)
      (OneOzGold)
      (OneOzSilver)
      (USD)
      (EUR)
      (YUAN)
      (GBP)
      (BitCoin)
      (NumUnits) 
)

struct bond_type
{
   bond_type():issue_type(BitShares),backing_type(BitShares){}
   unit_type  issue_type;
   unit_type  backing_type;
};

FC_REFLECT( bond_type, (issue_type)(backing_type) )

// TODO: define raw pack to 1 byte

