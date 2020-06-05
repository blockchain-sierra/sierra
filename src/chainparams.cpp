// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Sierra Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include "arith_uint256.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

#define NEVER32 0xFFFFFFFF

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
        CMutableTransaction txNew;
        txNew.nVersion = 1;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = genesisReward;
        txNew.vout[0].scriptPubKey = genesisOutputScript;

        CBlock genesis;
        genesis.nTime    = nTime;
        genesis.nBits    = nBits;
        genesis.nNonce   = nNonce;
        genesis.nVersion = nVersion;
        genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
        genesis.hashPrevBlock.SetNull();
        genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
        return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
        assert(!devNetName.empty());

        CMutableTransaction txNew;
        txNew.nVersion = 1;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        // put height (BIP34) and devnet name into coinbase
        txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
        txNew.vout[0].nValue = genesisReward;
        txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

        CBlock genesis;
        genesis.nTime    = nTime;
        genesis.nBits    = nBits;
        genesis.nNonce   = nNonce;
        genesis.nVersion = 4;
        genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
        genesis.hashPrevBlock = prevBlockHash;
        genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
        return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
 *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
 *   vMerkleTree: e0028e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
        const char* pszTimestamp = "Shift To Renewables To Become A Growing Trend In Mining";
        const CScript genesisOutputScript = CScript() << ParseHex("04e48194deb3f8e731347dae547ecaa98043c58712a59b06022ecf62730d745cc61c78e399700a2355504dcedb8d676ab43b779feea3dc9d58d00771dd515b7b32") << OP_CHECKSIG;
        return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock FindDevNetGenesisBlock(const Consensus::Params& params, const CBlock &prevBlock, const CAmount& reward)
{
        std::string devNetName = GetDevNetName();
        assert(!devNetName.empty());

        CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName.c_str(), prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

        arith_uint256 bnTarget;
        bnTarget.SetCompact(block.nBits);

        for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
                block.nNonce = nNonce;

                uint256 hash = block.GetHash();
                if (UintToArith256(hash) <= bnTarget)
                        return block;
        }

        // This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
        // iteration of the above loop will give a result already
        error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
        assert(false);
}

// this one is for testing only
static Consensus::LLMQParams llmq10_60 = {
        .type = Consensus::LLMQ_10_60,
        .name = "llmq_10",
        .size = 10,
        .minSize = 6,
        .threshold = 6,

        .dkgInterval = 24, // one DKG per hour
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 18,
};

static Consensus::LLMQParams llmq50_60 = {
        .type = Consensus::LLMQ_50_60,
        .name = "llmq_10_60",
        .size = 10, // 50,
        .minSize = 8, // 40,
        .threshold = 6, // 30,

        .dkgInterval = 45, // one DKG per hour
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 18,
        .dkgBadVotesThreshold = 3, // 40,

        .signingActiveQuorumCount = 24, // a full day worth of LLMQs

        .keepOldConnections = 25,
};

static Consensus::LLMQParams llmq400_60 = {
        .type = Consensus::LLMQ_400_60,
        .name = "llmq_30_60",
        .size = 30, // 400,
        .minSize = 24, // 300,
        .threshold = 18, // 240,

        .dkgInterval = 45 * 12, // one DKG every 12 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 28,
        .dkgBadVotesThreshold = 8, // 300,

        .signingActiveQuorumCount = 4, // two days worth of LLMQs

        .keepOldConnections = 5,
};

// Used for deployment and min-proto-version signalling, so it needs a higher threshold
static Consensus::LLMQParams llmq400_85 = {
        .type = Consensus::LLMQ_400_85,
        .name = "llmq_30_85",
        .size = 30, // 400,
        .minSize = 26, // 350,
        .threshold = 24, // 340,

        .dkgInterval = 45 * 24, // one DKG every 24 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 48, // give it a larger mining window to make sure it is mined
        .dkgBadVotesThreshold = 8, // 300,

        .signingActiveQuorumCount = 4, // two days worth of LLMQs

        .keepOldConnections = 5,
};


/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */


class CMainParams : public CChainParams {
public:
    CMainParams() {
            strNetworkID = "main";

            consensus.nSubsidyHalvingInterval = 0; // undefined yet
            consensus.nMasternodePaymentsStartBlock = 0;
            consensus.nInstantSendConfirmationsRequired = 6;
            consensus.nInstantSendKeepLock = 24;
            consensus.nBudgetPaymentsStartBlock = 0;
            consensus.nBudgetPaymentsCycleBlocks = 1;
            consensus.nBudgetPaymentsWindowBlocks = 100;
            consensus.nSuperblockStartBlock = 1;
            consensus.nSuperblockCycle = 32400; // blocks per month
            consensus.nSuperblockStartHash = uint256S("0");
            consensus.nGovernanceMinQuorum = 10;
            consensus.nGovernanceFilterElements = 20000;
            consensus.nMasternodeMinimumConfirmations = 15;

            consensus.nLastPoWBlock = 1000;
            consensus.nFirstDevFeeBlock = consensus.nLastPoWBlock + 1;
            consensus.nFirstSpowBlock = 190001; //consensus.nLastPoWBlock + 101;

            consensus.BIP34Height = 1;
            consensus.BIP34Hash = uint256S("0");
            consensus.BIP65Height = 1;
            consensus.BIP66Height = 1;
            consensus.DIP0001Height = 1;
            consensus.DIP0003Height = consensus.nLastPoWBlock + 1;
            consensus.DIP0003EnforcementHeight = NEVER32;
            consensus.DIP0003EnforcementHash = uint256();
            consensus.powLimit = uint256S("000fffff00000000000000000000000000000000000000000000000000000000");
            consensus.nPowTargetTimespan = 24 * 60 * 60;
            consensus.nPowTargetSpacing = 80;
            consensus.fPowAllowMinDifficultyBlocks = false;
            consensus.fPowNoRetargeting = true;
            consensus.nPowKGWHeight = 0;
            consensus.nPowDGWHeight = 0;
            consensus.nMaxBlockSpacingFixDeploymentHeight = 10000;

            // Stake information
            consensus.nPosTargetSpacing = 80;
            consensus.nPosTargetTimespan = 60 * 40; // 40 blocks at max for difficulty adjustment
            consensus.nStakeMaxAge = 60 * 60 * 24; // near one day
            consensus.nWSTargetDiff = 0x1e0ffff0;
            consensus.nPoSDiffAdjustRange = 5;

            consensus.nRuleChangeActivationThreshold = 1026; // 95% of 1080
            consensus.nMinerConfirmationWindow = 1080; // nPowTargetTimespan / nPowTargetSpacing

            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1577836800; // 2020-01-01T00:00:00+00:00
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1609459200; // 2021-01-01T00:00:00+00:00

            // Deployment of BIP68, BIP112, and BIP113.
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1577836800;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1609459200;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nThreshold = 80;

            // Deployment of DIP0001
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1577836800;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1609459200;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 80;

            // Deployment of BIP147
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1577836800;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1609459200;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 80;

            // Deployment of DIP0003
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1577836800;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1609459200;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 80;

            // Deployment of DIP0008
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 1577836800;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 1609459200;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThreshold = 80;

            // The best chain should have at least this much work.
            consensus.nMinimumChainWork = uint256S("0"); // set to known chain work for block N in the next release
            // By default assume that the signatures in ancestors of this block are valid.
            consensus.defaultAssumeValid = uint256S("0");
            /**
             * The message start string is designed to be unlikely to occur in normal data.
             * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
             * a large 32-bit integer with any alignment.
             */
            pchMessageStart[0] = 0x53;
            pchMessageStart[1] = 0x76;
            pchMessageStart[2] = 0x33;
            pchMessageStart[3] = 0x00;
            vAlertPubKey = ParseHex("0410ded580f8d0b8ff05bc2c88ba3bc2d809403726df700c734d71fadb5c0ce92b9c2874d08fe325e8732b34072f4a1cbe70d97c37c702ef60e62f3699d2bfdcca");
            nDefaultPort = 19518;
            nPruneAfterHeight = 100000;

            genesis = CreateGenesisBlock(1591228800, 2463995, 0x1e0ffff0, 1, 0 * COIN);
            consensus.hashGenesisBlock = genesis.GetHash();
            assert(consensus.hashGenesisBlock == uint256S("00000f49021b9fc63a4d335f32ed8a32743737d065215fb81d467e9849d4bb86"));
            assert(genesis.hashMerkleRoot == uint256S("741e37ce97352889aba3b62d3bc284a70f5ddf32cbad3653dafbf5f115614a35"));

            vSeeds.push_back(CDNSSeedData("seed1.sierracoin.org", "seed1.sierracoin.org"));
            vSeeds.push_back(CDNSSeedData("seed2.sierracoin.org", "seed2.sierracoin.org"));
            vSeeds.push_back(CDNSSeedData("seed3.sierracoin.org", "seed3.sierracoin.org"));

            base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 63);
            base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 16);
            base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 163);
            base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
            base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

            // BIP44 coin type is '517'
            nExtCoinType = 517;

            //vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

            // long living quorum params
            consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
            consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
            consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
            consensus.llmqChainLocks = Consensus::LLMQ_400_60;
            consensus.llmqForInstantSend = Consensus::LLMQ_50_60;

            fMiningRequiresPeers = true;
            fDefaultConsistencyChecks = false;
            fRequireStandard = true;
            fMineBlocksOnDemand = false;
            fAllowMultipleAddressesFromGroup = false;
            fAllowMultiplePorts = false;

            nPoolMinParticipants = 3;
            nPoolMaxParticipants = 5;
            nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

            vSporkAddresses = {"SjV3uCRcwajtNsEJJoy4EdDX7tGFm29PzF"};
            nMinSporkKeys = 1;
            fBIP9CheckMasternodesUpgraded = true;
            consensus.fLLMQAllowDummyCommitments = false;

            checkpointData = (CCheckpointData) {
                boost::assign::map_list_of
                        ( 0, uint256S("00000f49021b9fc63a4d335f32ed8a32743737d065215fb81d467e9849d4bb86"))
            };
            chainTxData = ChainTxData {
                1591228800, // * UNIX timestamp of last checkpoint block
                0,          // * total number of transactions between genesis and last checkpoint
                            //   (the tx=... number in the SetBestChain debug.log lines)
                0.1         // * estimated number of transactions per day after checkpoint
            };

            devAddress = "SMX5CuiLSqkRQGKZC6359rQUz657NnmEeh";
            spowAddress = "SXSLH3G35oXzpdHU3JUZ34KBfm3ifw8fGs";
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
            strNetworkID = "test";
            consensus.nSubsidyHalvingInterval = NEVER32;
            consensus.nMasternodePaymentsStartBlock = 15; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
            consensus.nMasternodePaymentsIncreaseBlock = NEVER32;
            consensus.nMasternodePaymentsIncreasePeriod = NEVER32;
            consensus.nInstantSendConfirmationsRequired = 2;
            consensus.nInstantSendKeepLock = 6;
            consensus.nBudgetPaymentsStartBlock = 46;
            consensus.nBudgetPaymentsCycleBlocks = 24;
            consensus.nBudgetPaymentsWindowBlocks = 10;
            consensus.nSuperblockStartBlock = 3050; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPaymentsStartBlock
            consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on testnet
            consensus.nGovernanceMinQuorum = 1;
            consensus.nGovernanceFilterElements = 500;
            consensus.nMasternodeMinimumConfirmations = 1;

            consensus.nLastPoWBlock = 1000;
            consensus.nFirstDevFeeBlock = consensus.nLastPoWBlock + 1;
            consensus.nFirstSpowBlock = consensus.nLastPoWBlock + 101;

            consensus.BIP34Height = 1;
            consensus.BIP34Hash = uint256S("0");
            consensus.BIP65Height = 1;
            consensus.BIP66Height = 1;
            consensus.DIP0001Height = 1;
            consensus.DIP0003Height = consensus.nLastPoWBlock + 1;
            consensus.DIP0003EnforcementHeight = NEVER32;
            consensus.DIP0003EnforcementHash = uint256();
            consensus.powLimit = uint256S("0000fffff0000000000000000000000000000000000000000000000000000000");
            consensus.nPowTargetTimespan = 60 * 60 * 24;
            consensus.nPowTargetSpacing = 80;
            consensus.fPowAllowMinDifficultyBlocks = true;
            consensus.fPowNoRetargeting = false;
            consensus.nPowKGWHeight = 0; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
            consensus.nPowDGWHeight = 0;

            // Stake info
            consensus.nPosTargetSpacing = 80;
            consensus.nPosTargetTimespan = 60 * 40;
            consensus.nStakeMaxAge = 60 * 60 * 24 * 30;
            consensus.nPoSDiffAdjustRange = 1;
            consensus.nWSTargetDiff = 0x1f00ffff; // Genesis Difficulty
            consensus.nMaxBlockSpacingFixDeploymentHeight = -1;

            consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
            consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

            // Deployment of BIP68, BIP112, and BIP113.
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

            // Deployment of DIP0001
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 4032;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 3226;

            // Deployment of BIP147
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 4032;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 3226;

            // Deployment of DIP0003
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 4032;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 3226;

            // Deployment of DIP0008
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = NEVER32;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = NEVER32;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 4032;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThreshold = 3226;

            // The best chain should have at least this much work.
            consensus.nMinimumChainWork = uint256S("0000000000000000000000000000000000000000000000000000000000000000");
            // By default assume that the signatures in ancestors of this block are valid.
            consensus.defaultAssumeValid = uint256S("0000000000000000000000000000000000000000000000000000000000000000");

            pchMessageStart[0] = 0x53;
            pchMessageStart[1] = 0x76;
            pchMessageStart[2] = 0x33;
            pchMessageStart[3] = 0x01;
            vAlertPubKey = ParseHex("0479f4a582b76a8ba3215c972ec9f36e0c0fcc695884a1b2f5f6a40e96a418a55cf1d0961e8be89981c91ecbf9f98000d40dff401024f5ae67ad7bdfdfb3f6d3b3");
            nDefaultPort = 29518;
            nPruneAfterHeight = 1000;

            uint32_t nTime = 1591228801;
            uint32_t nNonce = 1108918;

            if (!nNonce) {
                while (UintToArith256(genesis.GetHash()) >
                       UintToArith256(consensus.powLimit))
                {
                    nNonce++;
                    genesis = CreateGenesisBlock(nTime, nNonce, 0x1f00ffff, 1, 0 * COIN);
                }
            }

            genesis = CreateGenesisBlock(nTime, nNonce, 0x1f00ffff, 1, 0 * COIN);
            consensus.hashGenesisBlock = genesis.GetHash();

            //vSeeds.push_back(CDNSSeedData("testnetseed1.sierracoin.org", "testnetseed.sierracoin.org"));
            //vSeeds.push_back(CDNSSeedData("testnetseed2.sierracoin.org", "testnetseed2.sierracoin.org"));

            // Testnet sierra addresses start with 'y'
            base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
            // Testnet sierra script addresses start with '8' or '9'
            base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
            // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
            base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
            // Testnet sierra BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
            base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
            // Testnet sierra BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
            base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

            // Testnet sierra BIP44 coin type is '1' (All coin's testnet default)
            nExtCoinType = 1;
    
            // long living quorum params
            consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
            consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
            consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
            consensus.llmqChainLocks = Consensus::LLMQ_50_60;
            consensus.llmqForInstantSend = Consensus::LLMQ_50_60;

            fMiningRequiresPeers = true;
            fDefaultConsistencyChecks = false;
            fRequireStandard = false;
            fMineBlocksOnDemand = false;
            fAllowMultipleAddressesFromGroup = false;
            fAllowMultiplePorts = false;

            nPoolMaxParticipants = 3;
            nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
    
            vSporkAddresses = {"SjV3uCRcwajtNsEJJoy4EdDX7tGFm29PzF"};
            nMinSporkKeys = 1;
            fBIP9CheckMasternodesUpgraded = true;
            consensus.fLLMQAllowDummyCommitments = true;

            checkpointData = (CCheckpointData) {
                    boost::assign::map_list_of
                            (   0, uint256S("0x"))


            };
            chainTxData = ChainTxData{
                    0, // * UNIX timestamp of last checkpoint block
                    0,       // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
                    0         // * estimated number of transactions per day after checkpoint

            };

            devAddress = "SiwF6oeXSpZ6WesPhMmFSfnzdvt3pPmJay";
            spowAddress = "SUBju14JMLqPxdHJAcb8NYxZQJJAaXUDP8";
    }
};
static CTestNetParams testNetParams;

/**
 * Devnet
 */
class CDevNetParams : public CChainParams {
public:
    CDevNetParams() {
            strNetworkID = "dev";
            consensus.nSubsidyHalvingInterval = 210240;
            consensus.nMasternodePaymentsStartBlock = 4010; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
            consensus.nMasternodePaymentsIncreaseBlock = 4030;
            consensus.nMasternodePaymentsIncreasePeriod = 10;
            consensus.nInstantSendConfirmationsRequired = 2;
            consensus.nInstantSendKeepLock = 6;
            consensus.nBudgetPaymentsStartBlock = 4100;
            consensus.nBudgetPaymentsCycleBlocks = 50;
            consensus.nBudgetPaymentsWindowBlocks = 10;
            consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
            consensus.nSuperblockStartHash = uint256(); // do not check this on devnet
            consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on devnet
            consensus.nGovernanceMinQuorum = 1;
            consensus.nGovernanceFilterElements = 500;
            consensus.nMasternodeMinimumConfirmations = 1;

            consensus.nLastPoWBlock = 1000;
            consensus.nFirstDevFeeBlock = consensus.nLastPoWBlock + 1;
            consensus.nFirstSpowBlock = consensus.nLastPoWBlock + 101;

            consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
            consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
            consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
            consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
            consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
            consensus.nPowTargetTimespan = 24 * 60 * 60;
            consensus.nPowTargetSpacing = 80;
            consensus.fPowAllowMinDifficultyBlocks = true;
            consensus.fPowNoRetargeting = false;
            consensus.nPowKGWHeight = 4001; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
            consensus.nPowDGWHeight = 4001;
            consensus.nMaxBlockSpacingFixDeploymentHeight = 700;

            consensus.nPosTargetSpacing = 80;
            consensus.nPosTargetTimespan = 60 * 40;
            consensus.nStakeMaxAge = 60 * 60 * 24; // one day

            consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
            consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing

            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008
    
            // Deployment of BIP68, BIP112, and BIP113.
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1506556800; // September 28th, 2017
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1538092800; // September 28th, 2018
    
            // Deployment of DIP0001
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1505692800; // Sep 18th, 2017
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1537228800; // Sep 18th, 2018
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100
    
            // Deployment of BIP147
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1517792400; // Feb 5th, 2018
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1549328400; // Feb 5th, 2019
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100
    
            // Deployment of DIP0003
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1535752800; // Sep 1st, 2018
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1567288800; // Sep 1st, 2019
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 50; // 50% of 100
    
            // Deployment of DIP0008
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 1553126400; // Mar 21st, 2019
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 1584748800; // Mar 21st, 2020
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 100;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThreshold = 50; // 50% of 100

            // The best chain should have at least this much work.
            consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

            // By default assume that the signatures in ancestors of this block are valid.
            consensus.defaultAssumeValid = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

            pchMessageStart[0] = 0x53;
            pchMessageStart[1] = 0x76;
            pchMessageStart[2] = 0x33;
            pchMessageStart[3] = 0x02;
            vAlertPubKey = ParseHex("0464dcb4a1dc747448a33b07b8cb71e6cf130e5889066a3c1b0cb268a89f29b54515411b9fbc0253ad1637386d7bd46a54624d700cefb9a6ab9ee3be63f3655f5f");
            nDefaultPort = 39518;
            nPruneAfterHeight = 1000;

            genesis = CreateGenesisBlock(1591228802, 1078333, 0x207fffff, 1, 0 * COIN);
            consensus.hashGenesisBlock = genesis.GetHash();
            assert(consensus.hashGenesisBlock == uint256S("0000074a2326761fd57ba13db6c67bfd20fba72b7f677676868f8685890b3257"));
            assert(genesis.hashMerkleRoot == uint256S("741e37ce97352889aba3b62d3bc284a70f5ddf32cbad3653dafbf5f115614a35"));

            devnetGenesis = FindDevNetGenesisBlock(consensus, genesis, 0 * COIN);
            consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

            vFixedSeeds.clear();
            vSeeds.clear();
            //vSeeds.push_back(CDNSSeedData("devnetseed1.sierracoin.org", "devnetseed1.sierracoin.org"));

            // Testnet Sierra addresses start with 'y'
            base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
            // Testnet Sierra script addresses start with '8' or '9'
            base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
            // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
            base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
            // Testnet Sierra BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
            base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
            // Testnet Sierra BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
            base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

            // Testnet Dash BIP44 coin type is '1' (All coin's testnet default)
            nExtCoinType = 1;
    
            // long living quorum params
            consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
            consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
            consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
            consensus.llmqChainLocks = Consensus::LLMQ_50_60;
            consensus.llmqForInstantSend = Consensus::LLMQ_50_60;

            fMiningRequiresPeers = true;
            fDefaultConsistencyChecks = false;
            fRequireStandard = false;
            fMineBlocksOnDemand = false;
            fAllowMultipleAddressesFromGroup = true;
            fAllowMultiplePorts = true;

            nPoolMaxParticipants = 3;
            nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
    
            vSporkAddresses = {"SjV3uCRcwajtNsEJJoy4EdDX7tGFm29PzF"};
            nMinSporkKeys = 1;
            // devnets are started with no blocks and no MN, so we can't check for upgraded MN (as there are none)
            fBIP9CheckMasternodesUpgraded = false;
            consensus.fLLMQAllowDummyCommitments = true;

            checkpointData = (CCheckpointData) {
                    boost::assign::map_list_of
                            (      0, uint256S("0000074a2326761fd57ba13db6c67bfd20fba72b7f677676868f8685890b3257"))
            };

            chainTxData = ChainTxData{
                    devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
                    2,                            // * we only have 2 coinbase transactions when a devnet is started up
                    0.01                          // * estimated number of transactions per second
            };

            devAddress = "SiwF6oeXSpZ6WesPhMmFSfnzdvt3pPmJay";
            spowAddress = "SUBju14JMLqPxdHJAcb8NYxZQJJAaXUDP8";
    }

    void UpdateSubsidyAndDiffParams(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
    {
        consensus.nMinimumDifficultyBlocks = nMinimumDifficultyBlocks;
        consensus.nHighSubsidyBlocks = nHighSubsidyBlocks;
        consensus.nHighSubsidyFactor = nHighSubsidyFactor;
    }
};
static CDevNetParams *devNetParams;


/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
            strNetworkID = "regtest";
            consensus.nSubsidyHalvingInterval = 150;
            consensus.nMasternodePaymentsStartBlock = 240;
            consensus.nMasternodePaymentsIncreaseBlock = 350;
            consensus.nMasternodePaymentsIncreasePeriod = 10;
            consensus.nInstantSendConfirmationsRequired = 2;
            consensus.nInstantSendKeepLock = 6;
            consensus.nBudgetPaymentsStartBlock = 25;
            consensus.nBudgetPaymentsCycleBlocks = 50;
            consensus.nBudgetPaymentsWindowBlocks = 10;
            consensus.nSuperblockStartBlock = 1500;
            consensus.nSuperblockStartHash = uint256(); // do not check this on regtest
            consensus.nSuperblockCycle = 10;
            consensus.nGovernanceMinQuorum = 1;
            consensus.nGovernanceFilterElements = 100;
            consensus.nMasternodeMinimumConfirmations = 1;

            consensus.nLastPoWBlock = 1000;
            consensus.nFirstDevFeeBlock = consensus.nLastPoWBlock + 1;
            consensus.nFirstSpowBlock = consensus.nLastPoWBlock + 101;

            consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
            consensus.BIP34Hash = uint256();
            consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
            consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
            consensus.DIP0001Height = 2000;
            consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
            consensus.nPowTargetTimespan = 24 * 60 * 60; // sierra: 1 day
            consensus.nPowTargetSpacing = 80;
            consensus.fPowAllowMinDifficultyBlocks = true;
            consensus.fPowNoRetargeting = true;
            consensus.nPowKGWHeight = 15200; // same as mainnet
            consensus.nPowDGWHeight = 34140; // same as mainnet
            consensus.nMaxBlockSpacingFixDeploymentHeight = 700;
            consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
            consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 999999999999ULL;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 0;
            consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 999999999999ULL;

            // Stake info
            consensus.nPosTargetSpacing = 80;
            consensus.nPosTargetTimespan = 60 * 40;
            consensus.nStakeMaxAge = 60 * 60 * 24; // one day

            // highest difficulty | 0x1e0ffff0 (?)
            // smallest difficulty | 0x008000
            consensus.nWSTargetDiff = 0x1e0ffff0; // Genesis Difficulty

            // The best chain should have at least this much work.
            consensus.nMinimumChainWork = uint256S("0x00");

            // By default assume that the signatures in ancestors of this block are valid.
            consensus.defaultAssumeValid = uint256S("0x00");

            pchMessageStart[0] = 0x53;
            pchMessageStart[1] = 0x76;
            pchMessageStart[2] = 0x33;
            pchMessageStart[3] = 0x03;
            nDefaultPort = 49518;
            nPruneAfterHeight = 1000;

            genesis = CreateGenesisBlock(1591228802, 1078333, 0x207fffff, 1, 0 * COIN);
            consensus.hashGenesisBlock = genesis.GetHash();
            assert(consensus.hashGenesisBlock == uint256S("0000074a2326761fd57ba13db6c67bfd20fba72b7f677676868f8685890b3257"));
            assert(genesis.hashMerkleRoot == uint256S("741e37ce97352889aba3b62d3bc284a70f5ddf32cbad3653dafbf5f115614a35"));


            vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
            vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

            fMiningRequiresPeers = false;
            fDefaultConsistencyChecks = true;
            fRequireStandard = false;
            fMineBlocksOnDemand = true;
            fAllowMultipleAddressesFromGroup = true;
            fAllowMultiplePorts = true;

            nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
    
            // privKey: cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK
            vSporkAddresses = {"SjV3uCRcwajtNsEJJoy4EdDX7tGFm29PzF"};
            nMinSporkKeys = 1;
            // regtest usually has no masternodes in most tests, so don't check for upgraged MNs
            fBIP9CheckMasternodesUpgraded = false;
            consensus.fLLMQAllowDummyCommitments = true;

            checkpointData = (CCheckpointData){
                    boost::assign::map_list_of
                            ( 0, uint256S("0000074a2326761fd57ba13db6c67bfd20fba72b7f677676868f8685890b3257"))
            };

            chainTxData = ChainTxData{
                    0,
                    0,
                    0
            };

            base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
            // Regtest sierra script addresses start with '8' or '9'
            base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
            // Regtest private keys start with '9' or 'c' (Bitcoin defaults)
            base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
            // Regtest sierra BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
            base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
            // Regtest sierra BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
            base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

            // Regtest Dash BIP44 coin type is '1' (All coin's testnet default)
            nExtCoinType = 1;
    
            // long living quorum params
            consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
            consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
            consensus.llmqChainLocks = Consensus::LLMQ_5_60;
            consensus.llmqForInstantSend = Consensus::LLMQ_5_60;
    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int64_t nWindowSize, int64_t nThreshold)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        if (nWindowSize != -1) {
            consensus.vDeployments[d].nWindowSize = nWindowSize;
        }
        if (nThreshold != -1) {
            consensus.vDeployments[d].nThreshold = nThreshold;
        }
    }

    void UpdateDIP3Parameters(int nActivationHeight, int nEnforcementHeight)
    {
        consensus.DIP0003Height = nActivationHeight;
        consensus.DIP0003EnforcementHeight = nEnforcementHeight;
    }

    void UpdateBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
    {
        consensus.nMasternodePaymentsStartBlock = nMasternodePaymentsStartBlock;
        consensus.nBudgetPaymentsStartBlock = nBudgetPaymentsStartBlock;
        consensus.nSuperblockStartBlock = nSuperblockStartBlock;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
        assert(pCurrentParams);
        return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
        if (chain == CBaseChainParams::MAIN)
                return mainParams;
        else if (chain == CBaseChainParams::TESTNET)
                return testNetParams;
        else if (chain == CBaseChainParams::DEVNET) {
                assert(devNetParams);
                return *devNetParams;
        } else if (chain == CBaseChainParams::REGTEST)
                return regTestParams;
        else
                throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
        if (network == CBaseChainParams::DEVNET) {
                devNetParams = new CDevNetParams();
        }

        SelectBaseParams(network);
        pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int64_t nWindowSize, int64_t nThreshold)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout, nWindowSize, nThreshold);
}

void UpdateRegtestDIP3Parameters(int nActivationHeight, int nEnforcementHeight)
{
    regTestParams.UpdateDIP3Parameters(nActivationHeight, nEnforcementHeight);
}

void UpdateRegtestBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
{
    regTestParams.UpdateBudgetParameters(nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
}

void UpdateDevnetSubsidyAndDiffParams(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
{
    assert(devNetParams);
    devNetParams->UpdateSubsidyAndDiffParams(nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
}
