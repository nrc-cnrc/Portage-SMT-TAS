// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
#ifndef __ugEncodingSchemes_hh
#define __ugEncodingSchemes_hh
#include <vector>
#include <stdint.h>

namespace ugdiss
{
  using namespace std;

  class Escheme
  {
  public:
    class formula;
    class formula1;
    class formula2;

    static formula const* const f1, *const f2;

    vector<uint32_t> blockSizes; // sizes of bit blocks
    formula const* f;
    uint32_t fid;

    Escheme(vector<uint32_t> const& blocks);
    ~Escheme();
    // Escheme(Escheme const& other);

    /** cost: total number of bits needed to encode the distribution
     *  given in /cnt/ (/cnt/ maps from a bit block size i to the
     *  number of numbers in the underlying distribution needing
     *  exactly i bits) */
    uint64_t cost;
    bool operator<(Escheme const& other) const;

    // uint64_t calcCost(vector<uint32_t> const& cnt) const;
    // void optmize(vector<uint32_t> const& cnt);

    // find optimal
  };

  class
  Escheme::
  formula
  {
  public:
    uint32_t const id;
    formula(uint32_t _id);
    virtual uint64_t operator()(vector<uint32_t> const& blk,
                                vector<uint32_t> const& cnt) const = 0;
  };

  class
  Escheme::formula1 : public formula
  {
  public:
    formula1();
    uint64_t operator()(vector<uint32_t> const& blk,
                        vector<uint32_t> const& cnt) const;
  };

  class
  Escheme::formula2 : public Escheme::formula
  {
  public:
    formula2();
    uint64_t operator()(vector<uint32_t> const& blk,
                        vector<uint32_t> const& cnt) const;
  };

  vector<Escheme>
  enumerate_schemes(vector<uint32_t> const& D, uint32_t max_blocks);

  Escheme
  best_scheme(vector<uint32_t> const& D, uint32_t max_blocks);

}
#endif
