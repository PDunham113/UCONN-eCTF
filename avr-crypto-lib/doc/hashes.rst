Hash functions
==============
A hash function is an algorithm to map an arbitrary long message (in the form
of a bit string) to a fixed length message digest or hash value.
The hash function aims to be collision free, which means that it is not 
practicable to find two messages with the same hash value (although this 
collision must exist). Also it should not be practicable to construct a 
message which maps to a given hash value.
 
List of available hash functions
--------------------------------
The following hash functions are currently implemented:

* Blake
* BlueMidnightWish
* CubeHash
* Echo
* Grøstl
* Keccak
* MD5
* SHA-256
* SHA-1
* Shabal
* Skein 

High frequent parameters
------------------------
+-----------------+------------------------------------------------------------+
| block size      | 512 bits                                                   |
+-----------------+------------------------------------------------------------+
| hash value size | 128 bits, 160 bits, 224 bits, 256 bits, 384 bits, 512 bits |
+-----------------+------------------------------------------------------------+

Parts of a hash function
------------------------
* initialization function 
* compression algorithm
* finalization function

hash function API
-----------------
The API is not always consistent due to the fact that we tried to optimize the
code for size (flash, heap and stack) and speed (runtime of the different 
components).

Generally the API of the implemented block ciphers consists of:

+--------------+--------------------------------------------------------------+
| \*_init      | function, which implements the initialization of the context |
+--------------+--------------------------------------------------------------+
| \*_nextBlock | function, which implements the compression algorithm         |
+--------------+--------------------------------------------------------------+
| \*_lastBlock | function, which implements the padding algorithm             |
+--------------+--------------------------------------------------------------+
| \*_ctx2hash  | function, which turns a context into an actual hash value    |
+--------------+--------------------------------------------------------------+
| \*_ctx_t     | context type, which can contains the state of a hashing      |
|              | process                                                      |
+--------------+--------------------------------------------------------------+

``*_init`` function
~~~~~~~~~~~~~~~~~~~
The ``*_init`` function generally takes a pointer to the context as parameter.
This function initializes the context with algorithm specific values.
 
``*_nextBlock`` function
~~~~~~~~~~~~~~~~~~~~~~~~
The ``*_nextBlock`` function is the core of each hash function. It updates the
hash state with a given message block. So this function uses a context pointer
and a message pointer as parameters. The size of a message block is fixed for
each hash function (mostly 512 bit). For the last block of a messages which may
be smaller than the blocksize you have to use the ``*_lastBlock`` function
described below.
 
``*_lastBlock`` function
~~~~~~~~~~~~~~~~~~~~~~~~
The ``*_lastBlock`` function finalizes the context with the last bits of a 
message. Since the last block is not required to have the blocksize you have
to specify the length of the last block (normally in bits). This function
performs the padding and final processing.

``*_ctx2hash`` function
~~~~~~~~~~~~~~~~~~~~~~~
The ``*_ctx2hash`` function turns a given hash context into an actual hash
value. If multiple sized hash value may be created from a context it is
necessary to give the the size of the hash value as parameter. 
 

Hash function abstraction layer (HFAL)
======================================
The HashFunctionAbstractionLayer (BCAL) is an abstraction layer which allows
usage of all implemented hash functions in a simple way. It abstracts specific
function details and is suitable for implementations which want to be flexible
in the choosing of specific hash functions. Another important aspect is that
this abstraction layer enables the implementation of hash function operating
modes independently from concrete hash function. It is very simple to use and
reassembles the API used to implement individual hash functions.

The main component is a hash function descriptor which contains the details of
the individual hash functions.

Parts of HFAL
-------------
The HFAL is split up in different parts:

* HFAL declaration for HFAL descriptors
* algorithm specific definitions of HFAL descriptors
* HFAL basic context type
* HFAL basic functions  

HFAL declaration for HFAL descriptors
-------------------------------------
The HFAL descriptor is a structure which is usually placed in FLASH or ROM since
modification is unnecessary. It contains all information required to use the
according hash function.

::

   typedef struct {
   	uint8_t  type; /* 2 == hashfunction */
   	uint8_t  flags;
   	PGM_P    name;
   	uint16_t ctxsize_B;
   	uint16_t blocksize_b;
   	uint16_t hashsize_b;
   	hf_init_fpt init;
   	hf_nextBlock_fpt  nextBlock;
   	hf_lastBlock_fpt  lastBlock;
   	hf_ctx2hash_fpt   ctx2hash;
   	hf_free_fpt free;
   	hf_mem_fpt mem;
   } hfdesc_t; /* hashfunction descriptor type */

+-------------+----------------------------------------------------------------+
| type        | should be set to ``2`` to indicate that this descriptor is for |
|             | a hash function.                                               |
+-------------+----------------------------------------------------------------+
| flags       | currently unused, should be set to zero.                       |
+-------------+----------------------------------------------------------------+
| name        | is a pointer to a zero terminated ASCII string giving the name |
|             | of the implemented primitive. On targets with                  |
|             | Harvard-architecture the string resides in code memory         |
|             | (FLASH, ROM, ...).                                             |
+-------------+----------------------------------------------------------------+
| ctxsize_B   | is the number of bytes which should be allocated for the       |
|             | context variable.                                              |
+-------------+----------------------------------------------------------------+
| blocksize_b | is the number of bits on which are hashed by one iteration of  |
|             | the nextBlock function.                                        |
+-------------+----------------------------------------------------------------+
| hashsize_b  | is the number of bits on which are output as final hash value. |
+-------------+----------------------------------------------------------------+
| init        | is a pointer to the init function.                             |
+-------------+----------------------------------------------------------------+
| nextBlock   | is a pointer to the algorithm specific nextBlock function.     |
+-------------+----------------------------------------------------------------+
| lastBlock   | is a pointer to the algorithm specific lastBlock function.     |
+-------------+----------------------------------------------------------------+
| ctx2hash    | is a pointer to the algorithm specific ctx2hash function.      |
+-------------+----------------------------------------------------------------+
| free        | is a pointer to the free function or NULL if there is no free  |
|             | function.                                                      |
+-------------+----------------------------------------------------------------+
| mem         | is a pointer to the algorithm specific mem function. This      |
|             | function hashes a complete message which has to reside         |
|             | entirely in RAM. This value may be NULL if there is no such    |
|             | function.                                                      |
+-------------+----------------------------------------------------------------+

HFAL-Basic context
------------------
Besides the context types for individual hash functions there is a generic context
type for HFAL. This is the context to use when using HFAL based functions.
The HFAL context has the following structure:

::

   typedef struct{
   	hfdesc_t* desc_ptr;
   	void*     ctx;
   } hfgen_ctx_t;

+----------+-----------------------------------------------+
| desc_ptr | a pointer to the HFAL descriptor              |
+----------+-----------------------------------------------+
| ctx      | pointer to the hash function specific context |
+----------+-----------------------------------------------+

HFAL-Basic
----------
HFAL-Basic provides the basic features of an hash function on top of the
HFAL. To use it you simply have to include the algorithms you want to use,
the HFAL descriptor file and of course the HFAL-Basic implementation.

The following functions are provided:

``hfal_hash_init``
~~~~~~~~~~~~~~~~~~
::

   uint8_t hfal_hash_init(const hfdesc_t* hash_descriptor, hfgen_ctx_t* ctx)

this function initializes a HFAL context based on the given HFAL descriptor
pointer (first parameter). The context to initialize is designated by the 
pointer passed as second parameter.

If everything works fine ``0`` is returned. In the case something fails the 
following codes are returned:

+---+-------------------------------------------------------------------+
| 3 | It was not possible to allocate enough memory to hold the context |
|   | variable for the selected hash function.                          |
+---+-------------------------------------------------------------------+

``hfal_hash_nextBlock``
~~~~~~~~~~~~~~~~~~~~~~~
::

   void hfal_hash_nextBlock(hfgen_ctx_t* ctx, const void* block)

this function hashes a block of memory (of algorithm specific length) and 
updates the context accordingly.

``hfal_hash_lastBlock``
~~~~~~~~~~~~~~~~~~~~~~~
::

   void hfal_hash_lastBlock(hfgen_ctx_t* ctx, const void* block, uint16_t length_b)
   
this function is used to hash the last block of a message. Since messages are
not required to consist of full blocks (or even full bytes) the length of the
block must be given in bits. The context is updated accordingly. This function
already performs padding and related stuff.

``hfal_hash_ctx2hash``
~~~~~~~~~~~~~~~~~~~~~~
::

   void hfal_hash_ctx2hash(void* dest, hfgen_ctx_t* ctx)

this function converts a context into an actual hash value which is stored in
``dest``. The application is responsible for allocating enough room.

``hfal_hash_free``
~~~~~~~~~~~~~~~~~~
::

   void hfal_hash_free(hfgen_ctx_t* ctx)
   
this function differs from the individual hash functions ``free`` function
in that it is always provided and must be called to avoid memory holes.
This function also automatically calls the implementation specific ``free``
function if one is provided.

``hfal_hash_mem``
~~~~~~~~~~~~~~~~~
::

   void hfal_hash_mem(const hfdesc_t* hash_descriptor, void* dest, const void* msg, uint32_t length_b)
   
this function is always provided (even if the actual algorithm does not 
specify a ``mem`` function. It hashes an entire message which resides in
RAM and stores the hash value in ``dest``. ``msg`` is the pointer to the
message and ``length_b`` is the message length in bits.

``hfal_hash_getBlocksize``
~~~~~~~~~~~~~~~~~~~~~~~~~~
::

   uint16_t hfal_hash_getBlocksize(const hfdesc_t* hash_descriptor)

returns the blocksize of the described (``hash_descriptor``) hash function.

``hfal_hash_getHashsize``
~~~~~~~~~~~~~~~~~~~~~~~~~
::

   uint16_t hfal_hash_getHashsize(const hfdesc_t* hash_descriptor)
   
returns the hash value size of the described (``hash_descriptor``) hash
function.

``hfal_hash_getCtxsize``
~~~~~~~~~~~~~~~~~~~~~~~~
::

   uint16_t hfal_hash_getCtxsize_B(const hfdesc_t* hash_descriptor)

returns the size of a context variable of the described (``hash_descriptor``)
hash function.



