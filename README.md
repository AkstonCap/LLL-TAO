# Lower Level Library / TAO Framework

Series of Templates for developing Crypto, Database, or Protocol. Base templates for the TAO framework, which will inherit these templates and create higher level functionality. This TAO framework is the foundation to Nexus as an integrated series of core upgrades for a live blockchain to upgrade into a multi-dimensional chain. This is named the acronym TAO standing for Tritium, Amine, and Obsidian.


## TAO Framework

The core base upgrades utilizing the LLL as base templates for Tritium, Amine, and Obsidian feature sets.

### Tritium

The first of the three updates in the TAO Framework. Tritium will include the following feature sets:

* Tritium Transactions / Signature Chains (Key Suites)
* Advanced Contracts (Phase 1)
* LISP Integration (Optional)
* L1 Locking Groups (LISP IP Multicast)
* Twin Blocks (No more Orphans)

### Amine

The second of the three updates in the TAO Framework. Amine will include the following feature sets:

* L1 Ledger Sharding Channels
* L2 Ledger Sharding Groups
* Advanced Contracts (Phase 2)
* IP Multicast Groups over LISP (L1, L2, and L3)
* L1 and L2 Trust Locking and Reputation

### Obsidian

The third of the three updates in the TAO Framework. Obsidian will include the following feature sets:

* L3 Ledger Sharding Cubes
* L3 Distributed Mining Pool
* L3 Trust Locking and Reputation
* Distributed Autonomous Community
* Ambassador / Developer Contracts
* Advanced Contracts (Phase 3)


## LISP (Locator / Identifier Separation Protocol)

Tritium provides support to run over the LISP overlay. Included in this repository are docker build files for deploying in a docker container (recommended), and of course native support for LISP, if you decide to build and run yourself. If you would like to learn more about LISP, it is open source and available here:

https://github.com/farinacci/lispers.net


## Lower Level Library

Following will include descriptions of the core components of the Lower Level Library. These base templates lay the foundation for any higher inheritance as a series of base classes for Crypto, Database, and Protocol.

### Lower Level Crypto

Set of Operations for handling Crypto including:

* Digital Signatures (ECDSA, Hash Based)
* Hashing (SHA3 / Notable Secure Algorithms)
* Encryption (Symmetric / Asymmetric)
* Post-Quantum Cryptography (Experimental)

Currently Implemented:

* SK Hashing (Skein and Keccak)
* OpenSSL wrapping functions (EC_KEY, BIGNUM)

To Implement:

* Experimental Lattice Based Signature Schemes


### Lower Level Database

Set of Templates for designing high efficiency database systems. Core templates can be expanded into higher level database types.

* Keychain Database
* Sector Database

Keychains Included:

* Binary File Map
* Binary Hash Map

We welcome any contributions of new keychains to provide different indexing data structures of the sector data.

### Lower Level Protocol

Set of Client / Server templates for efficient data handling. Inherit and create custom packet types to write a new protocol with ease and no network programming required.

* Data Server
* Listening Server
* Connection Types
* Packet Styles
* Event Triggers
* DDOS Throttling

LLP Protocols Implemented:

* Legacy
* Tritium
* HTTP


### Utilities

Set of useful tools for developing any program such as:

* Serialization
* Runtime
* Debug
* Json
* Arguments
* Containers
* Configuration
* Sorting
* Allocators
* Filesystem


## License

Nexus is released under the terms of the MIT license. See [COPYING](COPYING.MD) for more
information or see https://opensource.org/licenses/MIT.


## Contributing
If you would like to contribute as always submit a pull request. This library development is expected to be on-going, with new higher level templates created for any types of use in the web.


## Applications
This is a foundational piece of Nexus that can be rebased over a Nexus branch to incorporate new LLL features into Nexus. It could also be useful for:

* Custom MySQL Servers
* Custom WebServers
* Custom Protocols
* Scaleable Databases

These core templates can be expanded in any way by inheriting the base templates and creating any type of new backend that one would like.


## Why?
A lot of software that we use today for databases, or protocols, or cryptography was created back in the 1990's as open source software. Since then the industry has expanded and bloated this code causing performance degradation. The aim of these templates is performance in simplicity. Include only what is needed, no more, and no less. This allows extremely high performance and scaleability necessary for the new distributed systems that will continue to evolve over the next few decades.
