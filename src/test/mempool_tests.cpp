// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"
#include "util.h"

#include "test/test_sierra.h"

#include <boost/test/unit_test.hpp>
#include <list>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(mempool_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(MempoolRemoveTest)
{
    // Test CTxMemPool::remove functionality

    TestMemPoolEntryHelper entry;
    // Parent transaction with three children,
    // and three grand-children:
    CMutableTransaction txParent;
    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++)
    {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000LL;
    }
    CMutableTransaction txChild[3];
    for (int i = 0; i < 3; i++)
    {
        txChild[i].vin.resize(1);
        txChild[i].vin[0].scriptSig = CScript() << OP_11;
        txChild[i].vin[0].prevout.hash = txParent.GetHash();
        txChild[i].vin[0].prevout.n = i;
        txChild[i].vout.resize(1);
        txChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txChild[i].vout[0].nValue = 11000LL;
    }
    CMutableTransaction txGrandChild[3];
    for (int i = 0; i < 3; i++)
    {
        txGrandChild[i].vin.resize(1);
        txGrandChild[i].vin[0].scriptSig = CScript() << OP_11;
        txGrandChild[i].vin[0].prevout.hash = txChild[i].GetHash();
        txGrandChild[i].vin[0].prevout.n = 0;
        txGrandChild[i].vout.resize(1);
        txGrandChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txGrandChild[i].vout[0].nValue = 11000LL;
    }


    CTxMemPool testPool;

    // Nothing in pool, remove should do nothing:
    unsigned int poolSize = testPool.size();
    testPool.removeRecursive(txParent);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
}

BOOST_AUTO_TEST_SUITE_END()
