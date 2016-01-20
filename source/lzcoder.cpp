#include <stdlib.h>
#include <string.h>
#include "bitops.h"
#include "lzcoder.h"


/* -----------------------------------------------
	modify only if you know what you're doing!
	----------------------------------------------- */

	
#define LZ_D_MAX	( 1 << LZ_D_LMAX ) // absolute maximum LZ distance
#define LZ_C_MIN	3 // minimum number of symbols matched
#define LZ_HBYTE	2 // 1 <= X <= 3 (increases memory requirement!) 
#define LZ_HFLUSH	( 1 << 20 ) // might decrease memory requirement, but will cost CPU


/* -----------------------------------------------
	constructor for lz reader class
	----------------------------------------------- */

lz_encoder::lz_encoder( iostream* source ) {
	lz_encoder( source, 256, 4096 );
}


/* -----------------------------------------------
	constructor for lz reader class
	----------------------------------------------- */
	
lz_encoder::lz_encoder( iostream* source, int lim_count, int lim_dist ) {
	// set up limits
	limc = lim_count;
	limd = lim_dist;
	limcd = limc + limd - 1;
	// calloc
	hash_table = ( htab_entry** ) calloc( 1 << ( 8 * LZ_HBYTE ), sizeof( htab_entry* ) );
	lz_buffer = ( unsigned char* ) calloc( limcd, 1 );
	// init
	lz_next = NULL;
	str_src = source;
	fpos = 0;
	for ( int i = 0; i < ( 1 << ( 8 * LZ_HBYTE ) ); i++ )
		hash_table[i] = NULL;
	// fill lz next/prev buffer
	remain = str_src->read( lz_buffer, 1, limc );
	lhash = gen_hash();
}


/* -----------------------------------------------
	destructor for lz reader class
	----------------------------------------------- */
	
lz_encoder::~lz_encoder( void ) {
	// clear hash table and linked lists
	for ( int hash = 0; hash < 1 << ( 8 * LZ_HBYTE ); hash++ )
		clear_ht_entry( &(hash_table[ hash ]) );
	// free memory
	if ( lz_next != NULL ) free( lz_next );
	free( hash_table );
	free( lz_buffer );
}


/* -----------------------------------------------
	lz encode (one lz code)
	----------------------------------------------- */
	
lz_code* lz_encoder::encode_lz( void ) {
	lz_code* lz_curr = NULL;
	unsigned char* data = NULL;
	int bc;
	
	
	// check if lz code already in buffer
	if ( lz_next != NULL ) {
		lz_curr = lz_next;
		lz_next = NULL;
	} else {
		// setup current LZ code
		lz_curr = ( lz_code* ) calloc( 1, sizeof( lz_code ) );
		if ( lz_curr == NULL ) return NULL; // out of memory
		lz_curr->data = NULL;
		find_best_match( lz_curr );
	}
	
	// main coding process
	if ( lz_curr->dist == 0 ) { // no match found - seek till next
		// setup next LZ code, data
		lz_next = ( lz_code* ) calloc( 1, sizeof( lz_code ) );
		data = ( unsigned char* ) calloc( limc-1, 1 );
		if ( ( lz_next == NULL ) || ( data == NULL ) ) { // out of memory
			free( lz_curr );
			if ( lz_next != NULL ) free( lz_next );
			if ( data != NULL ) free( data );
			return NULL;
		}  
		lz_next->data = NULL;
		// end of data check
		if ( !current_byte( data ) ) {
			free( lz_next ); lz_next = NULL;
			free( data );
			return lz_curr;
		}
		// seek till next match
		for ( bc = 1;; bc++ ) {
			advance_stream( 1 );
			if ( find_best_match( lz_next ) ) break;
			if ( bc == (limc-1) ) break;
			if ( !current_byte( data + bc ) ) break;
		}		
		// finish current LZ code
		lz_curr->count = bc;
		lz_curr->data = ( unsigned char* ) realloc( data, bc );
		if ( lz_curr->data == NULL ) {
			if ( data != NULL ) free( data );
			free( lz_curr );
			return NULL;
		}
	} else advance_stream( lz_curr->count );
	
	
	return lz_curr;
}


/* -----------------------------------------------
	find best match in stream
	----------------------------------------------- */
	
int lz_encoder::find_best_match( lz_code* lz ) {
	htab_entry* ht = NULL;
	int lpos = fpos - ( limd - 1 );
	int p, c;
	
	
	lz->count = 0;
	lz->dist = 0;
	
	if ( remain < LZ_HBYTE ) return 0;
	if ( hash_table[lhash] != NULL ) {
		if ( hash_table[lhash]->pos < lpos ) {
			clear_ht_entry( &hash_table[lhash] );
			return 0;
		}
	} else return 0;
	
	for ( ht = hash_table[lhash];; ht = ht->next ) {
		for ( c = LZ_HBYTE, p = ht->pos; ( c < (limc-1) ) && ( (p+c)-fpos < remain ); c++ )
			if ( stream( fpos + c ) != stream( p + c ) ) break;
		if ( ( c > lz->count ) && ( ( LZ_C_MIN <= LZ_HBYTE ) || ( c >= LZ_C_MIN ) ) ) {
			lz->count = c;
			lz->dist = fpos - p;
			if ( c == limc - 1 ) break;
		}
		if ( ht->next != NULL ) {
			if ( ht->next->pos < lpos ) {
				clear_ht_entry( &(ht->next) );
				break;
			}
		} else break;
	}
	
	if ( lz->count > remain )
		lz->count = remain;
	
	
	return lz->dist;
}


/* -----------------------------------------------
	access previous/next byte
	----------------------------------------------- */

int lz_encoder::current_byte( unsigned char* byte ) {
	if ( remain == 0 ) return 0;
	(*byte) = stream( fpos );
	return 1;
}


/* -----------------------------------------------
	advance stream by n byte
	----------------------------------------------- */
	
inline void lz_encoder::advance_stream( int n ) {
	htab_entry* ht;
		
	while ( ( n-- > 0 ) && ( remain > 0 ) ) {
		// insert last hash into table
		ht = ( htab_entry* ) calloc( 1, sizeof( htab_entry ) );
		ht->pos = fpos;
		ht->next = hash_table[ lhash ];
		hash_table[ lhash ] = ht;
		// move on
		fpos++;
		lhash = gen_hash();
		if ( !str_src->read( lz_buffer + ( (fpos+limc-1) % limcd ), 1, 1 ) ) remain--;
		if ( fpos % LZ_HFLUSH == 0 ) clear_ht();
	}	
}


/* -----------------------------------------------
	access byte in absolute position
	----------------------------------------------- */
	
inline unsigned char lz_encoder::stream( unsigned int apos ) {
	return lz_buffer[ apos % limcd ];
}


/* -----------------------------------------------
	generate hash and add to hash-table
	----------------------------------------------- */
	
inline int lz_encoder::gen_hash( void ) {
	int hash = 0;
	for ( int i = 0; i < LZ_HBYTE; i++ )
		hash |= stream( fpos + i ) << (i*8);
	return hash;
}


/* -----------------------------------------------
	remove old entries from hash table
	----------------------------------------------- */
	
inline void lz_encoder::clear_ht( void ) {
	htab_entry* ht = NULL;
	int lpos = fpos - ( limd - 1 );
	
	for ( int hash = 0; hash < ( 1 << ( 8 * LZ_HBYTE ) ); hash++ ) {
		if ( hash_table[ hash ] != NULL ) {
			if ( hash_table[ hash ]->pos < lpos )
				clear_ht_entry( &(hash_table[hash]) );
			else for ( ht = hash_table[ hash ]; ht->next != NULL; ht = ht->next ) {
				if( ht->next->pos < lpos ) {
					clear_ht_entry( &(ht->next) );
					break;
				}
			}
		}
	}
}


/* -----------------------------------------------
	clear entries from hash table
	----------------------------------------------- */

inline void lz_encoder::clear_ht_entry( htab_entry** start ) {
	for ( htab_entry* ht = *start; ht != NULL; ht = *start ) {
		*start = ht->next;
		free( ht );
	}
}


/* -----------------------------------------------
	constructor for lz reader class
	----------------------------------------------- */
	
lz_decoder::lz_decoder( iostream* dest ) {
	// calloc
	lz_buffer = ( unsigned char* ) calloc( LZ_D_MAX, 1 );
	// init
	str_dst = dest;
	fpos = 0;
}


/* -----------------------------------------------
	destructor for lz reader class
	----------------------------------------------- */
	
lz_decoder::~lz_decoder( void ) {
	// free memory
	free( lz_buffer );
}


/* -----------------------------------------------
	lz decode (one lz code)
	----------------------------------------------- */
	
void lz_decoder::decode_lz( lz_code* lz ) {
	if ( lz->dist > 0 ) write_lz_code( lz );
	else write_nbyte( lz->data, lz->count );
}


/* -----------------------------------------------
	decode lz code to stream
	----------------------------------------------- */
	
void lz_decoder::write_lz_code( lz_code* lz ) {
	unsigned char byte;
	int i, p;
	for ( i = 0, p = fpos - lz->dist; i < lz->count; i++, fpos++, p++ ) {
		byte = lz_buffer[ p % LZ_D_MAX ];
		lz_buffer[ fpos % LZ_D_MAX ] = byte;
		str_dst->write( &byte, 1, 1 );
	}
}


/* -----------------------------------------------
	write n byte to stream
	----------------------------------------------- */
	
void lz_decoder::write_nbyte( unsigned char* data, int n ) {
	str_dst->write( data, 1, n );
	while ( n-- > 0 )
		lz_buffer[ (fpos++) % LZ_D_MAX ] = *(data++);
}
