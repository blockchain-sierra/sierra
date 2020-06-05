SIERRA Core integration/staging tree
=====================================

https://www.sierracoin.org

- Copyright (c) 2009-2014 Bitcoin Developers
- Copyright (c) 2014-2018 Dash Developers
- Copyright (c) 2018-2020 Sierra Developers

What's New?
-----------

Sierra 2.x:
 - based on unsupported code
 - reached end of life

Sierra 3.x:
 - based on stable Dash 0.14 codebase
 - skein PoW used for bootstrap
 - PoS minting implemented (based on few PoS coins)
 - fixed few PoS code bugs (miner crashes and deadlocks)
 - added orphan PoS block automatic recovery (no more lost coins)
 - DevFee and SPOW payments added
 - PrivateSend is currently disabled
 - Governance payments are currently disabled
 - DIP3 blockchain-based deterministic masternode lists
 - LLMQ (long-living masternode quorums) support
 - LLMQ-based InstantSend (instant confirmation of almost all transactions)
 - LLMQ-based ChainLocks (51% attack blockchain protection)
 - Dash API compatible, lot of applications can be easily ported
 - binary releases for Windows (32bit/64bit, both ZIP and Installer)
 - binary releases for Mac (64bit DMG)
 - binary releases for Linux variants (Intel i686, Intel x86_64, ARM gnueabihf, ARM aarch64)

What is Sierra?
---------------

Sierra is an experimental digital currency that enables anonymous, instant
payments to anyone, anywhere in the world. Sierra uses peer-to-peer technology
to operate with no central authority: managing transactions and issuing money
are carried out collectively by the network. Sierra Core is the name of the open
source software which enables the use of this currency.

Sierra is a blockchain based cryptocurrency that was designed to accelerate
the use and creation of solar energy. Like other cryptocurrencies such as Bitcoin,
the SIERRA network is truly global and decentralized.

Now we combine two important areas in the world economy, solar energy and the
rapidly developing cryptocurrency market! In our network, we combine Solar Proof
of Work to reward PV system owners and offer SIERRA Marketplace to purchase solar
panels using SIERRACOIN. The ecosystem of the project is justified and has
fundamental features from other cryptocurrencies!

For more information, as well as an immediately useable, binary version of
the Sierra Core software, see https://www.sierracoin.org/Find-sierra/.


License
-------

Sierra Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is meant to be stable. Development is normally done in separate branches.
[Tags](https://github.com/blockchain-sierra/sierra/tags) are created to indicate new official,
stable release versions of Sierra Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/qa) of the RPC interface, written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/qa) are installed) with: `qa/pull-tester/rpc-tests.py`

The Travis CI system makes sure that every pull request is built for Windows, Linux, and OS X, and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Sierra Core's Transifex page](https://www.transifex.com/projects/p/sierra/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

Translators should also follow the [forum](https://www.sierracoin.org/forum/topic/sierra-worldwide-collaboration.88/).
