/*
 * Copyright (c) 2001-2003 Regents of the University of California.
 * All rights reserved.
 *
 * See the file LICENSE included in this distribution for details.
 */

enum bamboo_stat {
  BAMBOO_OK = 0,
  BAMBOO_CAP = 1,
  BAMBOO_AGAIN = 2
};

typedef opaque bamboo_key[20];
typedef opaque bamboo_value<1024>; /* may be increased to 8192 eventually */
typedef opaque bamboo_placemark<100>;
typedef opaque bamboo_value_hash[20];

struct bamboo_hash {
    string algorithm<20>; /* only "" and "SHA1" are currently supported */
    opaque hash<40>;      /* "" means can't be removed, set hash to []  */
};

struct bamboo_put_arguments {
  string application<255>;
  string client_library<255>;
  bamboo_key key;
  bamboo_value value;
  bamboo_hash secret_hash;
  int ttl_sec;
};

struct bamboo_get_args {
  string application<255>;
  string client_library<255>;
  bamboo_key key;
  int maxvals;
  bamboo_placemark placemark;
};

struct bamboo_get_value {
    bamboo_value value;
    int ttl_sec_rem;
    bamboo_hash secret_hash; /* To distinguish between same key, same */
};                           /* value, different secret. */

struct bamboo_get_result {
    bamboo_get_value values<>;
    bamboo_placemark placemark;
};

struct bamboo_rm_arguments {
  string application<255>;
  string client_library<255>;
  bamboo_key key;
  bamboo_hash value_hash;    /* the triple of (key, value_hash, secret_hash) */
  string secret_hash_alg<20>;/*   identifies which put we're talking about */
  opaque secret<40>;         /* such that hash(secret) = secret_hash */
  int ttl_sec;               /* must be >= removed put's TTL remaining */
};

/********************************************************************
                               VERSION 2 TYPES
        The following types are used only in the version 2 interface.
 ********************************************************************/

struct bamboo_put_args {
  string application<255>;
  string client_library<255>;
  bamboo_key key;
  bamboo_value value;
  int ttl_sec;
};

struct bamboo_get_res {
    bamboo_value values<>;
    bamboo_placemark placemark;
};

/* NO LONGER SUPPORTED */
struct bamboo_rm_args {
  string application<255>;
  string client_library<255>;
  bamboo_key key;
  bamboo_value_hash value_hash;
  int ttl_sec;
};

/********************************************************************
                            END OF VERSION 2 TYPES
 ********************************************************************/

program BAMBOO_DHT_GATEWAY_PROGRAM {
	version BAMBOO_DHT_GATEWAY_VERSION {
		void 
		BAMBOO_DHT_PROC_NULL (void) = 1;

	        bamboo_stat
		BAMBOO_DHT_PROC_PUT (bamboo_put_args) = 2;

                bamboo_get_res
		BAMBOO_DHT_PROC_GET (bamboo_get_args) = 3;
                
                /* This remove is no longer supported at all. 
                bamboo_stat
                BAMBOO_DHT_PROC_RM (bamboo_rm_args) = 4;
                */
	} = 2;
	version BAMBOO_DHT_GATEWAY_VERSION_3 {
		void 
		BAMBOO_DHT_PROC_NULL (void) = 1;

	        bamboo_stat
		BAMBOO_DHT_PROC_PUT (bamboo_put_arguments) = 2;

                bamboo_get_result
		BAMBOO_DHT_PROC_GET (bamboo_get_args) = 3;
                
                bamboo_stat
                BAMBOO_DHT_PROC_RM (bamboo_rm_arguments) = 4;
	} = 3;
} = 708655600;

