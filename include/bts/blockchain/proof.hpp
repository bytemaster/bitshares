#pragma once
#include <fc/crypto/sha224.hpp>
#include <fc/reflect/reflect.hpp>

namespace bts
{

  /**
   *  @brief Data that is paired with 
   */
  struct proof
  {
     proof()
     :branch_path(-1),nonce(0){}

     /**
      *  Given the branch_path and branch_states calculates the
      *  merkel root. This root, combined with the nonce and the
      *  hash function is how the proof of work can be validated.
      */
     fc::sha224              merkel_root( const fc::sha224& leaf_hash )const;

     /** 
      *  Each bit from the LSF represents whether or not
      *  to the branch state is left or right branch of the
      *  merkel tree and is used to determine how to 
      *  reconstruct the merkel_root.
      *
      *  @note -1 indicates no merged mining
      */
     uint16_t                branch_path; 

     /**
      *  Does not include the leaf_hash which must be passed to the
      *  merkel_root() method to properly reconstruct the merkel root.
      *
      *  The leaf is not included in this struct because it can always be
      *  calculated from the header and it is helpful to avoid an extra
      *  28 bytes.
      */
     std::vector<fc::sha224> branch_states;
     uint64_t                nonce;
  };

  /**
   *  Manages merged mining by creating the full merkel tree
   *  for a set of leaf nodes.
   */
  struct merkel_tree
  {
     fc::sha224              merkel_root()const;
     proof                   get_proof( uint32_t index );
     std::vector<fc::sha224> leaf_hashes;
  };
}

FC_REFLECT( bts::proof, (branch_path)(branch_states)(nonce) )
FC_REFLECT( merkel_tree, (leaf_nodes) )
